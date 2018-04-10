/*
 * https://pubsubclient.knolleary.net/api.html#PubSubClient for documentation on using MQTT PubSub
 * 
 * 
 * 
 */

//Arduino Libraries
#include <SPI.h>
#include <Ethernet.h>

//Contributed Libraries
#include <PubSubClient.h>
#include <SRAM.h>

//Connection Information
byte mac[]    = {0x90, 0xA2, 0xDA, 0x11, 0x29, 0x5A};
IPAddress ip(192, 168, 0, 31);
IPAddress server(172, 16, 0, 2);

//Initialize EthernetClient
EthernetClient ethernetClient;

//AWS IOT Config
const char aws_server[]    = "a19bzzqi5jpbs0.iot.us-east-1.amazonaws.com";
const char aws_key[]         = "AKIAI7SA3CEMZGOWRJ5A";
const char aws_secret[]      = "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";
const char aws_region[]      = "us-east-1";
const char* aws_topic  = "$aws/things/ArduinoRev3/shadow/update";
int aws_port = 443; //AWS IoT Port for MQTT with Websockets (Port 8883 MQTT w/o WebSockets)

//Callback to receive messages from topic
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i=0;i<length;i++) 
  {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();
}

SRAM sram(4, SRAM_1024);

//PubSubClient (server, port, [callback], client, [stream])
PubSubClient client(aws_server, aws_port, callback, ethernetClient, sram);
//client.setServer(aws_server, aws_port);
//client.setCallback(callback);

//Handle Reconnection
void reconnect() 
{
  //Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("arduinoClient")) 
    {
      Serial.println("connected");
      
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      
      // ... and resubscribe
      client.subscribe("inTopic");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      //Wait 5s, then try again
      delay(5000);
    }
  }
}

//Upon Arduino Startup
void setup() 
{
  Serial.begin(57600);

  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);
  // Allow the hardware to sort itself out
  delay(1500);

}

//Main Loop 
void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  
  client.loop();

}
