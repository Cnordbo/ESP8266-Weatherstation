#include <ThingSpeak.h>
#include <Adafruit_BME280.h>      //BME280 sensor bibliotek
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define DEFAULT_SEALEVELPRESSURE_HPA (1014) // define a default pressure at sealevel
#define LED_BUILTIN 2

// YR URL
// URL Format: /sted/<Land>/<Fylke>/<Kommune>/<Sted>/varsel.xml
#define YR_URL "/sted/Norge/Rogaland/Stavanger/Hundvaag/varsel.xml"

WiFiManager wifiManager; // initialisering av WiFi Manager

Adafruit_BME280 bme; // I2C initialisering av BME sensor

ADC_MODE(ADC_VCC); // Initialiser avlesing av volt

unsigned long delayTime = 1000;             // delay variable
const char* server = "api.thingspeak.com";  // ThingSpeak server
unsigned long myChannelNumber = 000000;     // ThingSpeak Channel number
const char * apiKey = "";   // Write API key
unsigned long deepSleepForMinutes = 20;     // Go to deep sleep for 20 minutes


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);               // Serial communication for debugging
    digitalWrite(LED_BUILTIN, HIGH);
    wifiManager.setTimeout(120);
    wifiManager.autoConnect();        // Tries to connect to WiFi, if it fails it sets up a hotspot you can connect to for conifuguration
    digitalWrite(LED_BUILTIN,LOW);
    blink(2);
    
    Serial.println(F("BME280 test")); // Starting sensor test
    
    bool status;
    status = bme.begin();  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        blink(5);
        while (1);
    }
    Serial.println("-- Default Test --");
    Serial.println();
    delay(100); // let sensor boot up
    blink(1);
    //
}

void blink(int numberOfTimes) {
  for (int i = 0; numberOfTimes >= i; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN,LOW);
    delay(500);
  }
}


// Connect and return the current hPa value from YR.no
double getHPAFromYr() {
  Serial.println();
  Serial.println("==== YR ====");
  WiFiClient client;
  
  const char * host = "www.yr.no"; 
  Serial.print("Connecting to: ");
  Serial.println(host);

  if (!client.connect(host,80)) {
    Serial.println("Connection failed");
    delay(5000);
    return DEFAULT_SEALEVELPRESSURE_HPA;
  }

  client.print(String("GET ") + YR_URL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return DEFAULT_SEALEVELPRESSURE_HPA;
    }
  }

  String hPaNodeFromXml;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    // Yield() == delay(0) -- This allows the ESP8266 do its maintenance stuff if needed to keep the connection alive. 
    yield();
    // We "Cheat" and just take the first occurance of any hPa value. This usually indicates the "current" hPa value
    // as YR just gives us a forecast, and not data from the past. 
    if (line.indexOf("hPa") > -1) {
      hPaNodeFromXml.concat(line);
      break;
    }
  }
  Serial.println("Closing client connection");
  client.stop();

  //If we fail to find a value, lets use the default one. 
  if (hPaNodeFromXml.length() < 0) {
    return DEFAULT_SEALEVELPRESSURE_HPA;
  }
  
  int location = hPaNodeFromXml.indexOf("value=\"")+7;
  String tmpStringValue = hPaNodeFromXml.substring(location,location+6);
  const char * cValue = tmpStringValue.c_str();
  double dValue = strtod(cValue,NULL);
  Serial.print("YR: ");
  Serial.print(dValue);
  Serial.print(" hPa");
  Serial.println();
  Serial.println("==== /YR ====");
  Serial.println();
  return dValue;
}

void printValues(double hPa) {
    WiFiClient client;       // initialisering av WiFi Client
    ThingSpeak.begin(client);       // starter ThingSpeak klient

    // * Get Temperature
    float temp = (bme.readTemperature()); // temp = temperature from sensor
    Serial.print("Temperature = "); // print Type
    Serial.print(temp);             // print temperature
    Serial.println(" ªC");          // print suffix with line break
    ThingSpeak.setField(1, temp);   // set temp to field 1 in thingspeak

    // * Get Pressure
    float press = (bme.readPressure() / 100.0F);  // press = pressure from sensor
    Serial.print("Pressure = ");            // print Type
    Serial.print(press);                    // print pressure
    Serial.println(" hPa");                 // print suffix with linebreak
    ThingSpeak.setField(3, press);

    // * Approx altitude 
    float alt = (bme.readAltitude(hPa)); // alt = en eller annen altitude :-P
    Serial.print("Altitude = ");
    Serial.print(alt);
    Serial.println(" m");
    ThingSpeak.setField(5, alt);

    // * Get Humidity
    float hum = (bme.readHumidity());       // hum = humidity from sensor
    Serial.print("Humidity = ");      // print Type
    Serial.print(hum); // print humidity
    Serial.println(" %");             // print suffix with line break
    ThingSpeak.setField(2, hum);      // set hum to field 2 in thingspeak 

    // * Get battery voltage
    float vcc = (ESP.getVcc() / 1024.00f);  // vcc = voltage
    Serial.print("Battery = ");     // print Type
    Serial.print(vcc);              // print voltage
    Serial.println(" V");           // print suffix with line break
    ThingSpeak.setField(4, vcc);    // set vcc to field 4 in thingspeak
    
    // * Write all data to Thingspeak
    ThingSpeak.writeFields(myChannelNumber, apiKey);

    Serial.println();               // line break
   
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  printValues(getHPAFromYr());
  delay(delayTime);
  digitalWrite(LED_BUILTIN, LOW);
  ESP.deepSleep(deepSleepForMinutes  * 60 * 1000000); // go to deep sleep for 20 minutes
} 
