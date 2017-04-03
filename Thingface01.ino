/********************************************************************/
// Arduino Internet of Things
// - control/monitor arduino over internet
// - example for home automation system

/********************************************************************/
// INSTALL
// - UIPEthernet for ENC28J60
// - OneWire, DallasTemperature for DS18B20 temperature sensor
// - DHT sensor library for DHT sensor
// - PubSubClient
// - ArduinoJson

/********************************************************************/
// WIRE UP
//
//	ENC28J60 - Arduino
//	----------------------
//	SS			- 10
//	MOSI(SI)	- 11
//	MISO (SO)	- 12
//	SCK			- 13
//	----------------------
//
//	DS18B20 - Arduino
//	----------------------
//	1.pin - +5V
//	2.pin - 2
//	3.pin - GND
//
//	DHT11 - Arduino
//	----------------------
//	1.pin - 2
//  2.pin - +5V
//	3.pin - GND

/********************************************************************/
// SETUP
// 
// 1. Set MQTT_MAX_PACKET_SIZE in PubSubClient.h to 200
// 2. create account at http://thingface.io/account/new
// 3. register your device and get ID and secret key


// uncomment for UIP Ethernet module
//#define UIP 1
// uncomment for DHT11
#define DHT11 1
// uncomment for LM35
//#define LM35 1
// uncomment for DS18B20
//#define DS 1

#ifdef UIP
#include <UIPEthernet.h>
#else
#include <SPI.h>
#include <Ethernet.h>
#endif

#ifdef DHT11
#include <DHT.h>
#endif

#ifdef DS
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DEVICE_ID "83785b2afa664877"
#define SECRET_KEY "H3YBhON2Y29hU5iNU4MjzHGd2coXgt"
#define ONE_WIRE_BUS 2

const char* sensorTopicPrefix = "d/d/"DEVICE_ID"/";

bool ledState;
// interval for sending sensor values
unsigned long previousMillis = 0;
const long interval = 6000; // must be >=5 seconds

byte mac[] = {
	0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

//52.174.88.226 dev-app.thingface.io
IPAddress serverIp(52, 174, 88, 226);
EthernetClient ethClient;

#ifdef LM35
int adcValue = 0;
#endif

#ifdef DHT11
DHT dht(2, DHT11);
#endif // DHT11

#ifdef DS
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#endif // DS

void callback(char* topic, byte* payload, unsigned int length)
{
	if(strContains(topic, "u/c/")){
		char json[length];
		for(int i = 0; i < length; i++){
			json[i] = (char) payload[i];
		}
		StaticJsonBuffer<200> jsonBuffer;
		JsonObject& commandPayload = jsonBuffer.parseObject(json);
		if (!commandPayload.success())
		{
			//Serial.println(json);
			//Serial.println("parseObject() failed");
			return;
		}
		if(commandPayload["c"] == "led")
		{
			byte ledPin = commandPayload["a"][0];
			if (ledPin == 9)
			{
				ledState = !ledState;
				digitalWrite(ledPin, ledState ? HIGH : LOW);
			}			
		}
		
		//clear
		memset(json, 0, sizeof(json));
		memset(&commandPayload, 0, sizeof(commandPayload));
		memset(&topic, 0, sizeof(topic));
		memset(&payload, 0, sizeof(payload));
		memset(&jsonBuffer, 0, sizeof(jsonBuffer));
	}
}

PubSubClient client(serverIp, 1883, callback, ethClient);

void setup()
{
	/* LEDs */
	pinMode(9, OUTPUT);
	
#ifdef DHT11
	dht.begin();
#endif

#ifdef DS
	sensors.begin();
#endif // DS

	/* ethernet */
	Ethernet.begin(mac);
	
	/* pubsub client */
	// connect to thingface server
	if (client.connect(DEVICE_ID, DEVICE_ID, SECRET_KEY))
	{
		// subscribe for commands from users
		client.subscribe("u/c/+/"DEVICE_ID);
	}

	//Serial.begin(9600);
}

void loop()
{
	client.loop();

	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= interval)
	{		
		previousMillis = currentMillis;

		float t = getTemperature();
		sendSensorValue("temp", t);
	}
}

float getTemperature()
{

	#ifdef DHT11
	return dht.readTemperature();
	#endif // DHT11

	#ifdef LM35
	adcValue = analogRead(A0);
	// (ADC * AREF / 10mV) / 1024
	return ((adcValue * 5000.0) / 10.0) / 1024;
	#endif // LM35

	#ifdef DS
	sensors.requestTemperatures();
	return sensors.getTempCByIndex(0);
	#endif // DS
}

void sendSensorValue(char* sensorId, float sensorValue)
{	
	char topicBuffer[50];
	String topicBuilder = sensorTopicPrefix;
	topicBuilder += sensorId;	
	topicBuilder.toCharArray(topicBuffer, 50);

	char payloadBuffer[30];
	String payloadBuilder = "{\"v\":";
	payloadBuilder += sensorValue;
	payloadBuilder += "}";
	payloadBuilder.toCharArray(payloadBuffer, 50);

	client.publish(topicBuffer, payloadBuffer);
}

char strContains(char *str, char *sfind)
{
	char found = 0;
	char index = 0;
	char len;

	len = strlen(str);
	
	if (strlen(sfind) > len) {
		return 0;
	}
	while (index < len) {
		if (str[index] == sfind[found]) {
			found++;
			if (strlen(sfind) == found) {
				return 1;
			}
		}
		else {
			found = 0;
		}
		index++;
	}

	return 0;
}
