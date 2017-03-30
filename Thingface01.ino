#include <DHT.h>
// 1. install libraries
// PubSubClient
// ArduinoJson
// 2. Set MQTT_MAX_PACKET_SIZE in PubSubClient.h to 200
// 3. create account at http://thingface.io
// 4. register your device and get ID and secret key

// optional
// for DHT11 
// install "Adafruit Unified Sensor"
// install "DHT sensor library"
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DEVICE_ID ""
#define SECRET_KEY ""

const char* sensorTopicPrefix = "d/d/"DEVICE_ID"/";

bool ledState1;
bool ledState2;
DHT dht(2, DHT11);
// interval for sending sensor values
unsigned long previousMillis = 0;
const long interval = 6000; // must be >=5 seconds

byte mac[] = {
	0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

//52.174.88.226 dev-app.thingface.io
IPAddress server(52, 174, 88, 226);
EthernetClient ethClient;

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
				ledState1 = !ledState1;
				digitalWrite(ledPin, ledState1 ? HIGH : LOW);
			}
			if (ledPin == 6)
			{
				ledState2 = !ledState2;
				digitalWrite(ledPin, ledState2 ? HIGH : LOW);
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

PubSubClient client(server, 1883, callback, ethClient);

void setup()
{
	/* LEDs */
	pinMode(9, OUTPUT);
	pinMode(6, OUTPUT);

	dht.begin();

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

		float t = dht.readTemperature();
		sendSensorValue("temp", t);

		//sendSensorValue("temp1", 10.45);
	}
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
