//#include <SPI.h>
//#include <Wire.h>
#include <ArduinoJson.h>
//#include <elapsedMillis.h>
//#include <Bounce2.h>

#include <Ethernet.h>
#include <SoftReset.h>
#include <PubSubClient.h>


#define falsepulseDelay 2
#define Zero 10000

volatile char FwdRev = 'S' ;
volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
volatile unsigned int Pulse_Counter = Zero;   // counter for the number of pulses

StaticJsonBuffer<35> jsonBuffer;

#define OPEN 1
#define CLOSE 2


/************************* Initialize Pin Setup *****************************/
//Actuator One(1) Output
int ENA1 = 9;
int IN1 = 8;
int IN2 = 7;
//------------------------------------
//Actuator Two(2) Output
int ENA2 = 5;
int IN3 = 3;
int IN4 = 12;

int OPTICAL_Pin = 2;
//------------------------------------


/************************* Ethernet Client Setup *****************************/
byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0x1F, 0x48};
IPAddress ip;



/************************* MQTT Client Config ********************************/

const char* mqttserver = "35.153.164.108";               // Local Broker
const int mqttport = 1883;                             // MQTT port
String subscriptionTopic = "awsiot_to_localgateway";   // Get messages from AWS
String publishTopic = "localgateway_to_awsiot";        // Send messages to AWS


/************************* MQTT Connection Monitoring ************************/

long connecionMonitoringFrequency = 10000UL;  // (1000 = 1 sec)
unsigned long lastConnectionCheck = 0;
int connectionAttempt = 0;
const int maxconnectionAttempt = 5;           // Reset after x connection attempt
const int maxConnectionCycles = 100;//100;    // Security Reset (Soft Reet)
const int maxCiclosDiarios = 5000;//500;      // Full Reset
int ConnectionCyclesCount = 0;
int totalCyclesCount = 0;

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.println("Callback Occured & Processing Payload");
  //char* chararray = (char*)payload;
  
  //String msgStr = String((char*)payload);

  JsonObject& root = jsonBuffer.parseObject((char*)payload);

  // Test if parsing succeeds.
  if (!root.success()) 
  {
    Serial.println("parseObject() failed");
    delay(5000);
    return;
  }

  int op = root.get<int>("Op");
  int dist = root.get<int>("Dist");
  Serial.println("Payload is:");
  Serial.print("Op Code: '");Serial.print(op);Serial.print("'");Serial.println();
  Serial.print("Dist: '");Serial.print(dist);Serial.print("'");Serial.println();
  delay(5000);
  
  if (op == OPEN) //Received OPEN Command
    open(dist);
  else if (op == CLOSE)//Received CLOSE Command
    close(dist);

    jsonBuffer.clear();

}

/************************** PubSubClient Declaration *******************************/


EthernetClient client;
PubSubClient mqttclient(mqttserver, mqttport, callback, client); //Callback function must be declared before this line


/***************************** Publish_To_Topic Function ************************/


bool Publicar(String topic, String value , bool retain)
{
  Serial.println("Publish to MQTT");
  delay(500);

  bool success = false;
  char cpytopic [50];
  char message [50];
  value.toCharArray(message, value.length() + 1);
  topic.toCharArray(cpytopic, topic.length() + 1);
  success = mqttclient.publish(cpytopic, message);

  return success;
}

/***************************** Broker Connection Functions ************************/


void CheckConnection() {
  ConnectionCyclesCount++;
  totalCyclesCount++;

  //Restart Ethernet Shield after x connection cycles
  if (ConnectionCyclesCount > maxConnectionCycles)
  {
    Serial.println("<CheckConnection> : Restarting Ethernet");
    client.stop();
    Ethernet.begin(mac);
    ConnectionCyclesCount = 0;
  }
  else
  {
    // Daily Softreset
    if (totalCyclesCount > maxCiclosDiarios) {
      Serial.println("<CheckConnection> : Resetting Device");
      totalCyclesCount = 0;
      delay(1000);
      soft_restart();
    }
    else
    {
      //Check MQTT Connection
      if (!mqttclient.connected()) {
        if (!reconnect()) {
          Serial.println("<CheckConnection> : Disconnected ~~ Connection Attempt #: " + (String)connectionAttempt);
          connectionAttempt++;
          if (connectionAttempt > maxconnectionAttempt)
          {

            connectionAttempt = 0;
            Serial.println("<CheckConnection> : Restarting Ethernet");
            client.stop();
            Ethernet.begin(mac);
            delay(1000);
          }
        }
        else
        {
          connectionAttempt = 0;
          Serial.println("<CheckConnection> : Reconnected");
        }
      }
      else
      {
        Serial.println("<CheckConnection> : Connected");
      }
    }
  }
}

// Functon to reconnect to MQTT Broker
boolean reconnect() {
  Serial.println("Reconnecting: ");
  delay(1000);
  if (mqttclient.connect("arduinoClient")) {

    char topicConnection [50];

    subscriptionTopic.toCharArray(topicConnection, 25);
    mqttclient.subscribe(topicConnection);
  }

  return mqttclient.connected();
}



/************************** Sketch Code ********************************************/

void setup()
{
  Serial.begin(9600);
  delay(500);

  Serial.println("Serial Initialized Successfully");
  delay(500);

  Serial.println("Initializing Setup Procedures..."); Serial.println("Attempting Pin Set");
  delay(500);

  pinMode(OPTICAL_Pin, INPUT_PULLUP); // initialize the Optical Sensor pin as a input:
  attachInterrupt(0, count_ISR, RISING); // optical sensor output to Arduino pin2 interrupt

  //Set Pins for Actuator 1
  pinMode(ENA1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(A1, INPUT);

  //Set Pins for Actuator 2
  pinMode(ENA2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(A2, INPUT);

  Serial.println("Successfully Set Pins");Serial.println("Configuring MQTTClient & Ethernet Connection");
  delay(500);

  //Start Ethernet connection:
  Ethernet.begin(mac);
  ip = Ethernet.localIP();

  //Configure mqttClient
  mqttclient.setServer(mqttserver, mqttport);
  mqttclient.setCallback(callback);


  Serial.println("Successfully Configured MQTT & Ethernet");
  Serial.print("Arduino IPv4 IP: "); Serial.print(ip); Serial.println();
  Serial.println("Starting Connection to MQTTBroker");
  delay(500);

  //Connect to MQTTBroker
  if (reconnect())
  {
    Serial.println("Successfully Connected to Broker");
    delay(500);
  }
  else
  {
    Serial.println("Initial Connection Failed, trying again momentarily");
    delay(500);
  }


  //fullSpeedReverse(1);
  //fullSpeedReverse(2);
  //delay(52000);

  Serial.println("Resetting Actuator Position to Home");
  //close(30);
  Pulse_Counter = Zero; //reset counter

  Serial.println("Setup Procedures Concluded"); Serial.println("Entering Main Loop...");
  delay(500);

  //open();
}

void loop()
{
  String msg;

  //Check Connection Status
  if (millis() - lastConnectionCheck > connecionMonitoringFrequency)
  {
    lastConnectionCheck = millis();
    CheckConnection();
  }

  mqttclient.loop(); //End Cycle
}

void open(int dist) //Open the Actuators Concurrently
{
  Serial.println("Opening Now");

  int closeTime = (2*dist);closeTime = closeTime/0.5;
  int count = 0;
  int pulseDist = dist*100; //100 pulses an inch

  int initialPulseCount = Pulse_Counter;
  FwdRev = 'F';
  
  Serial.print("Initial Pulse Count: ");Serial.print(initialPulseCount);Serial.println();
  Serial.print("Pulse Dist: ");Serial.print(pulseDist);Serial.println();
  Serial.print("Comparision is: ");Serial.print(initialPulseCount - Pulse_Counter);Serial.println();
  Serial.print("Close Time is: ");Serial.print(closeTime);Serial.println();

  while((Pulse_Counter - initialPulseCount) < pulseDist) //Move specified dist (100 pulses is an inch)
  {
    fullSpeedForward(1); //Engage Actuator 1
    fullSpeedForward(2); //Engage Actuator 2
    delay(250);
    
    Serial.print("Pulse Counter is: "); Serial.print((Pulse_Counter - initialPulseCount));Serial.println(); //Display Current Sensor Reading for Actuator 1
        delay(250);

        count++;


      if(count == closeTime) //Force a stop after 20sec (40 loops @ 500ms/ea = 20 sec)
      {
        brake(1); //Engage Brake Actuator 1
        brake(2); //Engage Brake Actuator 2
        delay(150);
        FwdRev = 'S';
        return;

      }
   }

        brake(1);
        brake(2);
        delay(150);
        FwdRev = 'S';
        return;
      }

void close(int dist) //Close the Actuators Concurrently
{
  Serial.println("Closing Now");

  int closeTime = (2*dist);closeTime = closeTime/0.5;
  int count = 0;
  int pulseDist = dist*100; //100 pulses an inch

  int initialPulseCount = Pulse_Counter;
  FwdRev = 'R';

  Serial.print("Initial Pulse Count: ");Serial.print(initialPulseCount);Serial.println();
  Serial.print("Pulse Dist: ");Serial.print(pulseDist);Serial.println();
  Serial.print("Comparision is: ");Serial.print(initialPulseCount - Pulse_Counter);Serial.println();
  Serial.print("Close Time is: ");Serial.print(closeTime);Serial.println();

 while((initialPulseCount - Pulse_Counter) < pulseDist)
 {
    fullSpeedReverse(1); //Engage Actuator 1
    fullSpeedReverse(2); //Engage Actuator 2
    delay(250);
    
    Serial.print("Pulse Counter is: "); Serial.print((initialPulseCount - Pulse_Counter));Serial.println(); //Display Current Sensor Reading for Actuator 1
    delay(250);

    count++;

    
   if(count == closeTime) //Force a stop after 20sec (40 loops @ 500ms/ea = 20 sec)
   {
   Serial.println("Breaking Now");

   brake(1); //Engage Brake Actuator 1
   brake(2); //Engage Brake Actuator 2
   delay(150);

   Serial.println("Breaking Complete");
   FwdRev = 'S';
   return;
   }

  }
  brake(1);
  brake(2);
  delay(150);
  FwdRev = 'S';
  return;
}



  // actuator is 1 or 2
  void brake(int actuator) {
  if (actuator == 1) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA1, HIGH);
  }
        else { // if (actuator == 2)
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        digitalWrite(ENA2, HIGH);
      }
      }

        //Acuator is 1 || 2
        void fullSpeedForward(int actuator) //Engage Actuators to Open
      {
        if (actuator == 1)
      {
        digitalWrite(IN2, HIGH);
        digitalWrite(IN1, LOW);
        digitalWrite(ENA1, HIGH);
      }
        else if(actuator == 2)
      {
        digitalWrite(IN4, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(ENA2, HIGH);
      }
      }

        //Acuator is 1 || 2
        void fullSpeedReverse(int actuator) //Engage Actuators to Close
      {
        if (actuator == 1)
      {
        digitalWrite(IN2, LOW);
        digitalWrite(IN1, HIGH);
        digitalWrite(ENA1, HIGH);
      }
        else if(actuator == 2)
      {
        digitalWrite(IN4, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(ENA2, HIGH);
      }
      }

        void count_ISR()
      {
        // This function is called by the interrupt
        if ((millis() - lastDebounceTime) > falsepulseDelay) { //if current time minus the last trigger time is greater than the delay (pulse) time, pulse is valid.
        lastDebounceTime = millis();
        //received valid pulse
        if ( FwdRev=='F'){
        Pulse_Counter++;
      }
        if ( FwdRev=='R'){
        Pulse_Counter--;
      }
      }
      } // End of ISR

        int length(int actuator)//Returns Sensor Value
      {
        int ret = -1;

        //100 pulses per inch, 900 pulses for 9 inches


        if (actuator == 1)
      {
        ret = analogRead(A1);delay(200);
        Serial.print("Using A1: ");Serial.print(ret);Serial.println();

        ret = analogRead(1);delay(200);
        Serial.print("Using 1: ");Serial.print(ret);Serial.println();

        return ret;
      }
        else if(actuator == 3)
      {
        ret = analogRead(A2);delay(200);
        Serial.print("Using A2: ");Serial.print(ret);Serial.println();

        ret = analogRead(2);delay(200);
        Serial.print("Using 2: ");Serial.print(ret);Serial.println();

        return ret;
      }
        else
      {
        return ret; //Returns -1, invalid argument
      }
      }

        /****************************UNUSED FUNCTIONS****************************/
        /*
        //Acuator is 1 || 2
        //dutyCycle is 0-255
        void reverse(int actuator, int dutyCycle) {
        if (actuator == 1) {
        digitalWrite(IN2, LOW);
        digitalWrite(IN1, HIGH);
        analogWrite(ENA1, dutyCycle);
      }
        else { // if (actuator == 2)
        digitalWrite(IN4, LOW);
        digitalWrite(IN3, HIGH);
        analogWrite(ENA2, dutyCycle);
      }
      }

        //Acuator is 1 || 2
        //dutyCycle is 0-255
        void forward(int actuator, int dutyCycle) {
        if (actuator == 1) {
        digitalWrite(IN2, HIGH);
        digitalWrite(IN1, LOW);
        analogWrite(ENA1, dutyCycle);
      }
        else { // if (actuator == 2)
        digitalWrite(IN4, HIGH);
        digitalWrite(IN3, LOW);
        analogWrite(ENA2, dutyCycle);
      }
      }

        //Acuator is 1 || 2
        void floating(int actuator) {
        if (actuator == 1) {
        digitalWrite(IN2, HIGH);
        digitalWrite(IN1, HIGH);
        digitalWrite(ENA1, HIGH);
      }
        else { // if (actuator == 2)
        digitalWrite(IN4, HIGH);
        digitalWrite(IN3, HIGH);
        digitalWrite(ENA2, HIGH);
      }
      }
        */
