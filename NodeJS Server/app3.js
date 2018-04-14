var mqtt = require('mqtt')
var client = mqtt.connect('mqtt://35.153.164.108')

var AWS = require('aws-sdk')

AWS.config.update({
    accessKeyId: 'AKIAJU66CVAJDHDFYX2A',
    secretAccessKey: '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',
    region: 'us-east-1'
});

var lambda = new AWS.Lambda()


//MQTT Broker Handlers & Directives
client.on('connect', function () {
    console.log('Connecting & Subscribing')
    client.subscribe('localgateway_to_awsiot')
    client.subscribe('awsiot_to_localgateway')
    console.log('Done Connecting & Subscribing')
})

client.on('message', function (topic, message) {

    switch (topic) {
        case 'localgateway_to_awsiot':
            console.log('Ping From localgateway_to_awsiot');
            break;
        case 'awsiot_to_localgateway':
            console.log('Ping From awsiot_to_localgateway');
            decodeMsg(message);
            break;
    }
})


function decodeMsg(str) {

    var obj;

    //Attempt to parse the JSON String, Error if invalid JSON Format
    try {
        obj = JSON.parse(str);
    }
    catch (error) {
        console.log('Message was not in proper JSON Format');
        return;
    }

    if (obj.Op === 3)//Command to Invoke Optimization Angle Function
    {
        retData = invokeSolarOpt();
        client.publish('awsiot_to_localgateway', JSON.stringify({
            'Op': 4,
            'Dist': retData
        }));
        return retData;
    }
}



function invokeSolarOpt() {

    var returnData;
    var obj;

    var params = {
        FunctionName: 'getSolarAltitude',

        Payload: JSON.stringify({
               'latitude': 28.5899,
               'longitude': -81.211795,
            })
    };

    lambda.invoke(params, function (error, data) {
        if (error) {
            console.log('Error Invoking Lambda Function: ' + error);
        }
        else {
            console.log('Success... Data is: ');
            console.log(data);
            returnData = data;
        }
    });

    obj = JSON.parse(returnData);
    returnData = obj.Payload;
    return returnData;
}



//topic awsiot_to_localgateway
//topic localgateway_to_awsiot

/*
//Verify Modules are present
var http = require('http');
var awsIoT = require('aws-iot-device-sdk');
var name = 'ArduinoRev3';

var app = awsIoT.device({
    certPath: 'certs/certificate.pem.crt',
    keyPath: 'certs/private.pem.key',
    caPath: 'certs/root-ca.pem.crt',
    clientId: 'NodeServer',
    host: 'a19bzzqi5jpbs0.iot.us-east-1.amazonaws.com',
    region: 'us-east-1'
})

/*
$aws/things/ArduinoRev3/shadow/update
$aws/things/ArduinoRev3/shadow/update/accepted
$aws/things/ArduinoRev3/shadow/update/documents
$aws/things/ArduinoRev3/shadow/update/rejected
$aws/things/ArduinoRev3/shadow/get
$aws/things/ArduinoRev3/shadow/get/accepted
$aws/things/ArduinoRev3/shadow/get/rejected 

app.subscribe(awsiot_to_localgateway);
app.subscribe(localgateway_to_awsiot);

app.on('message', function (topic, payload) {
    console.log('got message', topic, payload.toString());
})
*/

