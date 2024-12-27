#ifndef CONFIG_H
#define CONFIG_H

#define SENSORNAME "DeskLight" //change this to whatever you want to call your device

//Ports for the ledstrip to control
#define BLUEPIN D6
#define REDPIN D5 
#define GREENPIN D7
#define WHITE1PIN 0
#define WHITE2PIN 0

//Wifi configuration
#define STASSID0 "H369A6B77CF";
#define STAPSK0  "Spanning!";

const char* ssid0 = STASSID0;
const char* password0 = STAPSK0;

//Mqtt server
const char* mqtt_serverName = "192.168.2.231";
const char* mqtt_username = "mqtt_user";
const char* mqtt_password = "mqtT_HoMe";

//NTP server
const char* NTPServerName = "192.168.2.231";

const uint32_t connectTimeoutMs = 5000;

// InfluxDB server url, e.g. http://192.168.1.48:8086 (don't use localhost, always server name or ip address)
#define INFLUXDB_URL "http://192.168.2.231:8086"
// InfluxDB database name
#define INFLUXDB_DB_NAME "Temperatures"

#endif //CONFIG_H



