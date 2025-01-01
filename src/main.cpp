#include <string>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <RGBWWLedControl.h>
#include <MQTTDebugger.h>
#include <MqttIdentify.h>


#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
//#include "mqttledstripcontrol.h"

//============================================================================
// Multiwifi definition
//============================================================================
#if defined (ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#endif

#if defined (ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#endif
//============================================================================

WiFiClient espClient;
PubSubClient pubSubClient(espClient);

MqttIdentify mqttIdentify(SENSORNAME);

RGBWWLedControl rgbwwLedControl(REDPIN, GREENPIN, BLUEPIN, WHITE1PIN, WHITE2PIN);
//MqttLedstripControl mqttLedstripControl(espClient, SENSORNAME);

/******************************************************************************
 * WIFI
 * 
 * Setup the WIFI function
 ******************************************************************************/
void connectWifi(){
  if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
    // Serial.print("WiFi connected: ");
    // Serial.print(WiFi.SSID());
    // Serial.print(" ");
    // Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected!");
  }
}

/******************************************************************************
 * OTA
 * 
 * Setup the OTA function
 ******************************************************************************/
void OTA(){

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.println("Ready via OTA");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void publishGenericState() {
    StaticJsonDocument<200> stateDoc;
    
    stateDoc["state"] = rgbwwLedControl.getLedStatus() ? "ON" : "OFF";
    stateDoc["brightness"] = rgbwwLedControl.getBrightnessRGB();
    
    // JsonObject color = stateDoc.createNestedObject("color");
    // color["r"] = rgbwwLedControl.getRed() / 4;
    // color["g"] = rgbwwLedControl.getGreen() / 4;
    // color["b"] = rgbwwLedControl.getBlue() / 4;

    String output;
    serializeJson(stateDoc, output);
    pubSubClient.publish(SENSORNAME "/state", output.c_str());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  //===========================================
  if (strcmp(topic,SENSORNAME "/Control")==0) {
    StaticJsonDocument<200> jsonBuffer;
    char s[length];
    for (int i = 0; i < length; i++) {
      //Serial.print((char)payload[i]);
      s[i]=payload[i];
    }
    Serial.print("Received JSON: ");
    Serial.println(s);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, s);
    if (error){
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
    } 
    else {
      if (doc["state"].is<const char*>()) {
        const char* state = doc["state"];
        Serial.printf("Value state = %s\n", state);
        if(strcmp(state, "ON") == 0) {
          rgbwwLedControl.on();
        } else {
          rgbwwLedControl.off();
        }
      }
      if (doc["brightness"].is<signed int>()) {
        u_int8_t brightness = doc["brightness"];
        Serial.printf("Value brightness = %d\n", brightness);
        rgbwwLedControl.setBrightnessRGB(brightness);
      }
      if (doc["color"].is<JsonObjectConst>())  {
        JsonObject color = doc["color"];
        int color_r = color["r"];
        int color_g = color["g"];
        int color_b = color["b"];
        Serial.printf("Value r=%d g=%d b=%d\n", color_r, color_g, color_b);
        rgbwwLedControl.setRGB(color_r * 4, color_g * 4, color_b * 4);
      }

      publishGenericState();
    }
  }
}



void reconnect() {
  // Loop until we're reconnected

  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (pubSubClient.connect(SENSORNAME, mqtt_username, mqtt_password)) {
     Serial.println("connected");

     Serial.print(pubSubClient.publish("MQTTIdentify", SENSORNAME));

      mqttIdentify.init(&pubSubClient,&WiFi);
      mqttIdentify.report();
      
      std::string controlChannel;
      controlChannel = SENSORNAME + std::string("/Control");

      pubSubClient.subscribe(controlChannel.c_str());
      Serial.print(controlChannel.c_str());

      controlChannel = SENSORNAME + std::string("/State");

      pubSubClient.publish(controlChannel.c_str(),"Alive");

    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void reportState() {
  Serial.println("Report state");
  if( rgbwwLedControl.getLedStatus() == true){
    Serial.println(pubSubClient.publish(SENSORNAME "/state", "{\"state\": \"ON\"}"));
  } 
  else {
    Serial.println(pubSubClient.publish(SENSORNAME "/state", "{\"state\": \"OFF\"}"));
  }
  
}

/******************************************************************************
 * 
 * Callback functions RGBWWLedControl
 * 
 *****************************************************************************/
int CallbackRGBWW(String parameter, int value) {
    Serial.println("\CallbackRGBWW: parameter=" + parameter + "/value=" + String(value));

    if (parameter == "state") {
      Serial.println("State change");
      if( value == 1){
        Serial.println("State ON");
         Serial.println(pubSubClient.publish(SENSORNAME "/fiets", "{\"state\": \"ON\"}"));
      } 
      else {
        Serial.println("State OFF");
         Serial.println(pubSubClient.publish(SENSORNAME "/fiets", "{\"state\": \"OFF\"}"));
      }
    }
    return 32000;
}

void setup() {
  WiFi.persistent(false);
  Serial.begin(115200);

  std::string deviceName;
  deviceName = SENSORNAME + std::string(" Booting...");
  
  Serial.println(deviceName.c_str());

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(STASSID0, STAPSK0);
  
  // connectWifi();

  // Serial.print("WiFi connected: ");
  // Serial.print(WiFi.SSID());
  // Serial.print(" ");
  // Serial.println(WiFi.localIP());

  pubSubClient.setServer(mqtt_serverName, 1883);
  pubSubClient.setCallback(callback);
 
  rgbwwLedControl.init();
  //rgbwwLedControl.setCallback(CallbackRGBWW);

// Doe we realy set the light here or do wee need to reload the latest settings
  rgbwwLedControl.off();

  rgbwwLedControl.setBrightnessRGB(255);
  rgbwwLedControl.setRGB(1023,512,255);
  rgbwwLedControl.on();
}


int reconnectCounter = 0;
void loop() {
  connectWifi();
  ArduinoOTA.handle();
  if (!pubSubClient.connected()) {
    reconnect();
    reportState();
  }


  pubSubClient.loop();
  rgbwwLedControl.loop();
  mqttIdentify.loop();  
}
