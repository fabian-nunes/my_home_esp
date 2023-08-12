#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h> //Pré-instalada com o Arduino IDE
#include <TimeLib.h>
#include <ArduinoJson.h> 

#define DHTPIN D1 // Pin Digital onde está ligado o sensor
#define DHTTYPE DHT11 // Tipo de sensor DHT
DHT dht(DHTPIN, DHTTYPE); // Instanciar e declarar a class DHT

WiFiUDP clienteUDP;
const char NTP_SERVER[] = "0.pool.ntp.org";
NTPClient clienteNTP(clienteUDP, NTP_SERVER, 3600);

const char SSID[] = "MEO-CF2AC3_EXT";
const char PASS_WIFI[] = "4D1FBE680D"; 

WiFiClient clienteWifi;
HTTPClient clienteHTTP;
const char URL_API_POST[] = "http://192.168.1.200:5000/api/sensor/data";
const char CONTENT_TYPE[] = "application/json";

const char JWT_TOKEN[] = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbWFpbCI6ImVzcEBtYWlsLmNvbSJ9.OUXvO51sKYVcst8TcBkbF82hkfRlwFo7fzGW-BFXfM8";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // espera ate o serial estar ligado
  while (!Serial);
  delay(250);
  Serial.print("Connecting to the WiFi...");
  WiFi.begin(SSID, PASS_WIFI);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Network Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Default Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Signal Potency: ");
  Serial.println(WiFi.RSSI());

  pinMode(LED_BUILTIN, OUTPUT);
  dht.begin();
}

void update_time(char *datehour, char *chour){
  clienteNTP.update();
  unsigned long epochTime = clienteNTP.getEpochTime();
  sprintf(datehour, "%02d-%02d-%02d %02d:%02d", year(epochTime), month(epochTime), day(epochTime), hour(epochTime), minute(epochTime));
  sprintf(chour, "%02d", hour(epochTime));
}

void post2API(String name, String value, String hour){

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["name"] = name;
  jsonDoc["value"] = value;
  jsonDoc["update"] = hour;
  jsonDoc["token"] = JWT_TOKEN;

  // Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  clienteHTTP.begin(clienteWifi, URL_API_POST);
  clienteHTTP.addHeader("Content-Type", CONTENT_TYPE);
  int status_code = clienteHTTP.POST(jsonString);

  if (status_code == 200){
    Serial.println(clienteHTTP.getString());
  }else{
    Serial.print("Error when executing the POST request: ");
    Serial.println(status_code);
    Serial.println(clienteHTTP.getString());
  }

  clienteHTTP.end();
}

void loop() {
  char datehour[20];
  char hour[2];
  update_time(datehour, hour);
  Serial.print("Actual Date: ");
  Serial.println(datehour);

  int currentHour;
  sscanf(hour, "%d", &currentHour);

  // Check if the current hour is between 9 and 18 (inclusive)
  if (currentHour >= 9 && currentHour <= 18) {

    //Read humidity as %
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    Serial.println(humidity);
    Serial.println(temperature);

    String sendNameTemp = "Temperature";
    String sendValueTemp = String(temperature);
    String sendHourTemp = datehour;

    post2API(sendNameTemp, sendValueTemp, sendHourTemp);

    String sendNameHum = "Humidity";
    String sendValueHum = String(humidity);
    String sendHourHum = datehour;

    post2API(sendNameHum, sendValueHum, sendHourHum);
  } else {
    Serial.println(currentHour);
    Serial.println("Work day ended go to sleep!");
  }

  Serial.println("Going to sleep for 15 minutes to not kill dht");
  delay (900000);
}