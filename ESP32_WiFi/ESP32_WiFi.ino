#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
//#define BLYNK_TEMPLATE_ID           "TMPxxxxxx"
//#define BLYNK_TEMPLATE_NAME         "Device"
#define BLYNK_AUTH_TOKEN            "YourAuthToken"   // ใส่ Token


#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <ClosedCube_OPT3001.h>
#include <DHT.h>

// ตั้งค่าสำหรับเซนเซอร์ความชื้นในดิน
#define MOISTURE_PIN 34

// ตั้งค่าสำหรับ OPT3001 sensor
ClosedCube_OPT3001 opt3001;
#define OPT3001_ADDRESS 0x44  // ถ้าไม่ได้เปลี่ยนเป็น 0x45

// ตั้งค่าสำหรับ DHT11
#define DHTPIN 4          // DHT11 data pin
#define DHTTYPE DHT11     // DHT11 temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

// ใส่ชื่อไวไฟกับรหัสผ่าน
char ssid[] = "YourNetworkName";
char pass[] = "YourPassword";

unsigned long previousMillis = 0;

void setup()
{
  // Debug console
  Serial.begin(115200);

  // เรื่มต้นทำงาน opt3001
  Serial.println("ClosedCube OPT3001 Arduino Test");
  opt3001.begin(OPT3001_ADDRESS);
  Serial.print("OPT3001 Manufacturer ID");
  Serial.println(opt3001.readManufacturerID());
  Serial.print("OPT3001 Device ID");
  Serial.println(opt3001.readDeviceID());
  configureSensor();
  printResult("High-Limit", opt3001.readHighLimit());
  printResult("Low-Limit", opt3001.readLowLimit());
  Serial.println("----");

  // DHT11 Setup
  dht.begin();

  // Soil Moisture Sensor Setup
  pinMode(MOISTURE_PIN, INPUT);

  // เชื่อมต่อไวไฟและblynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "elec.cmtc.ac.th", 8080);


}

void loop()
{
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 2000) {   // ทำงานทุก 2 วินาที
    previousMillis = currentMillis;

    // อ่านค่าแสงจากเซนเซอร์แสง
    OPT3001 result = opt3001.readResult();
    printResult("OPT3001", result);

    // อ่านค่าเซนเซอร์ DHT11
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.println(F("°C "));

    // อ่านค่าเซนเซอร์ความชื้นในดิน
    int moisture = analogRead(MOISTURE_PIN);

    // Send data to Blynk app
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
    Blynk.virtualWrite(V2, result.lux);
    Blynk.virtualWrite(V3, moisture);
  }
}


void configureSensor() {
  OPT3001_Config newConfig;

  newConfig.RangeNumber = B1100;
  newConfig.ConvertionTime = B0;
  newConfig.Latch = B1;
  newConfig.ModeOfConversionOperation = B11;

  OPT3001_ErrorCode errorConfig = opt3001.writeConfig(newConfig);
  if (errorConfig != NO_ERROR)
    printError("OPT3001 configuration", errorConfig);
  else {
    OPT3001_Config sensorConfig = opt3001.readConfig();
    Serial.println("OPT3001 Current Config:");
    Serial.println("------------------------------");

    Serial.print("Conversion ready (R):");
    Serial.println(sensorConfig.ConversionReady, HEX);

    Serial.print("Conversion time (R/W):");
    Serial.println(sensorConfig.ConvertionTime, HEX);

    Serial.print("Fault count field (R/W):");
    Serial.println(sensorConfig.FaultCount, HEX);

    Serial.print("Flag high field (R-only):");
    Serial.println(sensorConfig.FlagHigh, HEX);

    Serial.print("Flag low field (R-only):");
    Serial.println(sensorConfig.FlagLow, HEX);

    Serial.print("Latch field (R/W):");
    Serial.println(sensorConfig.Latch, HEX);

    Serial.print("Mask exponent field (R/W):");
    Serial.println(sensorConfig.MaskExponent, HEX);

    Serial.print("Mode of conversion operation (R/W):");
    Serial.println(sensorConfig.ModeOfConversionOperation, HEX);

    Serial.print("Polarity field (R/W):");
    Serial.println(sensorConfig.Polarity, HEX);

    Serial.print("Overflow flag (R-only):");
    Serial.println(sensorConfig.OverflowFlag, HEX);

    Serial.print("Range number (R/W):");
    Serial.println(sensorConfig.RangeNumber, HEX);

    Serial.println("------------------------------");
  }

}

void printResult(String text, OPT3001 result) {
  if (result.error == NO_ERROR) {
    Serial.print(text);
    Serial.print(": ");
    Serial.print(result.lux);
    Serial.println(" lux");
  }
  else {
    printError(text, result.error);
  }
}

void printError(String text, OPT3001_ErrorCode error) {
  Serial.print(text);
  Serial.print(": [ERROR] Code #");
  Serial.println(error);
}
