/*
   ไลบรารี่ที่ใช้งาน
   1.Blnyk จาก library manager
   2.ClosedCube OPT3001 จาก library manager
   3.DHT จาก library manager


   ใช้งาน
   1. ใส่ token blynk
   2. ใส่ ssid pass wifi
   3. แก้ไข server ที่ใช้งาน


   ต่อสาย
   -DHT
   OUT -> 4
   vcc -> 3v
   GND -> GND

   -OPT3001 light sensor
   SDA -> 21
   SCL -> 22
   vcc -> 3v
   GND -> GND

   -Moisture sensor
   A0 -> 34
   vcc -> 3v
   GND -> GND

   -Dimmer
   vcc   -> 3v
   gnd   -> gnd
   zero  -> 27
   tring -> 32

   Blynk Virtual pin
   V0 -> temp
   V1 -> humid
   V2 -> light
   V3 -> moisture
   V4 -> dimmer
*/

#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
//#define BLYNK_TEMPLATE_ID           "TMPxxxxxx"
//#define BLYNK_TEMPLATE_NAME         "Device"
#define BLYNK_AUTH_TOKEN            "HgpRAq3mLjnyIgBjO9spjnvKerAZoXJe"   // <======= 1.ใส่ Token

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <ClosedCube_OPT3001.h>
#include <DHT.h>

#define TRIGPIN 32                 //    <======= ขาควบคุม DIMMER
#define ZEROPIN 27

hw_timer_t * timer = NULL;
uint16_t trigTime = 9000;

// ตั้งค่าสำหรับเซนเซอร์ความชื้นในดิน
#define MOISTURE_PIN 34

// ตั้งค่าสำหรับ OPT3001 sensor
ClosedCube_OPT3001 opt3001;
#define OPT3001_ADDRESS 0x44  // ถ้าไม่ได้เปลี่ยนเป็น 0x45

// ตั้งค่าสำหรับ DHT11
#define DHTPIN 4          // DHT11 data pin
#define DHTTYPE DHT11     // DHT11 temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

// ใส่ชื่อไวไฟกับรหัสผ่าน                     <======= 2.ใส่ ssid pass
char ssid[] = "ssid";
char pass[] = "pass";

unsigned long previousMillis = 0;

void IRAM_ATTR onZero() {

  if (trigTime == 0) {
    digitalWrite(TRIGPIN, HIGH);
  } else if (trigTime == 10000) {
    digitalWrite(TRIGPIN, LOW);
  } else {
    digitalWrite(TRIGPIN, LOW);
    timerRestart(timer);
    timerAlarmWrite(timer, trigTime, true); // set the alarm to trigger every 1 microsecond
    timerAlarmEnable(timer); // enable the alarm
  }
}

void IRAM_ATTR onTrig() {
  digitalWrite(TRIGPIN, HIGH);
  timerAlarmDisable(timer); // disable the alarm
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
}

BLYNK_WRITE(V4)
{ /* This function gets called each time something changes on the widget */
  int value = param.asInt();  /* This gets the 'value' of the Widget as an integer */
  Serial.print("Dimer update to " + String(value) + "%");
  trigTime = (100 - value) * 100;
}

void setup()
{
  // Debug console
  Serial.begin(115200);

  // เรื่มต้นทำงาน opt3001
  Serial.println("ClosedCube OPT3001 Arduino Test");
  opt3001.begin(OPT3001_ADDRESS);
  Serial.print("OPT3001 Manufacturer ID");
  //  Serial.println(opt3001.readManufacturerID());
  Serial.print("OPT3001 Device ID");
  //  Serial.println(opt3001.readDeviceID());
  configureSensor();
  //  printResult("High-Limit", opt3001.readHighLimit());
  //  printResult("Low-Limit", opt3001.readLowLimit());
  Serial.println("----");

  // DHT11 Setup
  dht.begin();

  // Soil Moisture Sensor Setup
  pinMode(MOISTURE_PIN, INPUT);

  // เชื่อมต่อไวไฟและblynk              <======= 3.แก้ไข server ที่ใช้งาน
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "elec.cmtc.ac.th", 8080);

  // Initialize timer
  timer = timerBegin(0, 80, true); // timer_id = 0, prescaler = 80, countUp = true
  timerAttachInterrupt(timer, &onTrig, true); // attach the interrupt function

  // ขาคอนไทรล Dimmer
  digitalWrite(TRIGPIN, LOW);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ZEROPIN, INPUT);
  attachInterrupt(ZEROPIN, onZero, RISING);

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
    moisture = ( 100 - ( (moisture / 4095.00) * 100 ) );   // คำนวณค่าเป็นเปอร์เซน
    Serial.println("Moisture: " + String(moisture));

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
