/*************************************************************
  This is a DEMO sketch which works with Blynk myPlant app and
  showcases how your app made with Blynk can work

  You can download free app here:

  iOS:     TODO
  Android: http://play.google.com/store/apps/details?id=cc.blynk.appexport.demo

  If you would like to add these features to your product,
  please contact Blynk for Businesses:

                   http://www.blynk.io/

 *************************************************************/

//#define USE_SPARKFUN_BLYNK_BOARD    // Uncomment the board you are using
//#define USE_NODE_MCU_BOARD        // Comment out the boards you are not using
//#define USE_WITTY_CLOUD_BOARD
#define USE_CUSTOM_BOARD          // For all other ESP8266-based boards -
                                    // see "Custom board configuration" in Settings.h

#define APP_DEBUG        // Comment this out to disable debug prints

#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include "BlynkProvisioning.h"
#include "Adafruit_Si7021.h"

static int power = 14;

static int sleepTimeS = 10;
static int wateringThreshold = 20;

static bool isNotificationSent = false;
static bool isRefreshRateReceived = false;
static bool isWateringThresholdReceived = false;

Adafruit_Si7021 sensor = Adafruit_Si7021();

WidgetMap myMap(V0);

void setup() {
  Serial.begin(115200);

  sensor_init();
  sensor_read();


  /**************************************************************
   *
   * Workflow to connect the device to WiFi network.
   * Here is how it works:
   * 1. At the first start hardware acts as an Access Point and
   *    broadcasts it's own WiFi.
   * 2. myPlant smartphone app connects to this Access Point
   * 3. myPlant smartphone app request new Auth Token and passes
   *    it together with user's WiFi SSID and password
   * 4. Hardware saves this information to EEPROM
   * 5. Hardware reboots and now connects to user's WiFi Network
   * 6. Hardware connects to Blynk Cloud and is ready to work with app
   *
   * Next time the hardware reboots, it will use the same configuration
   * to connect. User can RESET the board and re-initiate provisioning
   *
   * Explore the Settings.h for parameters
   * Read the documentation for more info: http://
   *
   **************************************************************/

  BlynkProvisioning.begin();

  // If you want to remove all points:
  //myMap.clear();
}

void loop() {
  // This handles the network and cloud connection
  BlynkProvisioning.run();

  // Send sensors value to cloud when ready
  if (isRefreshRateReceived && isWateringThresholdReceived && BlynkState::is(MODE_RUNNING)) {
    send_sensor_value();

    int index = 1;
    float lat = 45.791011;
    float lon = -73.393825;
    myMap.location(index, lat, lon, "Plant 1 : OK");

    if (isNotificationSent) {
      BlynkState::set(MODE_THIRSTY);
      WiFi.forceSleepBegin();
    } 
    else {
      digitalWrite(power, LOW);
      DEBUG_PRINT("Device in sleep mode");
      ESP.deepSleep(sleepTimeS * 1000000);
    } 
  }

  if (BlynkState::is(MODE_THIRSTY)) {
    check_soil_moisture();
  }
}


/**************************************************************
 *
 *              myPlant App code
 *
 * The following code manage plant monitoring system
 *
 **************************************************************/

static int s1 = 12;
static int s2 = 13;
static int clk = 15;

static int sensorLight = 0;
static int sensorBattery = 0;
static int sensorSoilMoisture = 0;
static float sensorAirHumidity = 0.0;
static float sensorAirTemperature = 0.0;

// Calibration factors
const int soilMoistureOffset = -191;
const float soilMoistureMultiplier = 1.15;

// Getting data from "Refresh Rate" step setting
BLYNK_WRITE(V10) {
  sleepTimeS = param.asInt() * 60;
  if (sleepTimeS == 0) {
    sleepTimeS = 10;
  } 
  DEBUG_PRINT(String("Refresh rate: ") + sleepTimeS);
  isRefreshRateReceived = true;
}

// Getting data from "Watering threshold" step setting
BLYNK_WRITE(V11) {
  wateringThreshold = param.asInt();
  DEBUG_PRINT(String("Watering threshold: ") + wateringThreshold);
  isWateringThresholdReceived = true;
}

// When device starts ->
//   sync refresh rate and watering threshold
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V10, V11);
}


void sensor_init() {
  DEBUG_PRINT("Init sensors");
  pinMode(power, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  
  digitalWrite(power, HIGH);

  analogWriteFreq(40000);
  analogWrite(clk, 100);

  delay(500);

  sensor.begin();
}


void sensor_read() {
  DEBUG_PRINT("Read sensor");
  
  // Soil Moisture
  digitalWrite(s1, HIGH);
  digitalWrite(s2, LOW);
  delay(10);
  sensorSoilMoisture = 1023 - analogRead(A0);
  DEBUG_PRINT("Soil Moisture RAW: " + String(sensorSoilMoisture));
  sensorSoilMoisture = (sensorSoilMoisture + soilMoistureOffset)*soilMoistureMultiplier;
  DEBUG_PRINT("Soil Moisture: " + String(sensorSoilMoisture));
   
  // Light level
  digitalWrite(s1, LOW);
  digitalWrite(s2, HIGH);
  delay(10);
  sensorLight = analogRead(A0);
  DEBUG_PRINT("Light level: " + String(sensorLight));

  // Battery level
  digitalWrite(s1, HIGH);
  digitalWrite(s2, HIGH);
  delay(10);
  sensorBattery = analogRead(A0);
  DEBUG_PRINT("Battery level: " + String(sensorBattery));

  // Air humidity
  sensorAirHumidity = sensor.readHumidity();
  DEBUG_PRINT("Air humidity: " + String(sensorAirHumidity));

  // Air temperature
  sensorAirTemperature = sensor.readTemperature();
  DEBUG_PRINT("Air temperature: " + String(sensorAirTemperature));

  // Disable i2c interface
  pinMode(4, INPUT);
  pinMode(5, INPUT);

  // Init rgb led
  indicator.initLED();

  // Power off sensors
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
}


void send_sensor_value() {
  DEBUG_PRINT("Send sensors values to cloud");

  Blynk.virtualWrite(V3, sensorAirTemperature);
  Blynk.virtualWrite(V4, sensorAirHumidity);

  // Soil moisture
  if (sensorSoilMoisture < 30) {
    Blynk.virtualWrite(V1, "DRY");
  } else if (sensorSoilMoisture > 80) {
    Blynk.virtualWrite(V1, "WET");
    isNotificationSent = false;
  } else {
    Blynk.virtualWrite(V1, "MOIST");
  }
  Blynk.virtualWrite(V7, sensorSoilMoisture);

  // Light level
  if (sensorLight < 100) {
    Blynk.virtualWrite(V2, "LOW");
  } else if (sensorLight > 800) {
    Blynk.virtualWrite(V2, "HIGH");
  } else {
    Blynk.virtualWrite(V2, "MED");
  }
  Blynk.virtualWrite(V8, sensorLight);
  
  Blynk.virtualWrite(V9, sensorBattery);

  // Manage notifications
  if (sensorSoilMoisture < wateringThreshold) {
    if (isNotificationSent == false) {
      Blynk.notify("Your plant is thirsty!");
      isNotificationSent = true;
      DEBUG_PRINT("Notification sent");
    }
  }
}


void check_soil_moisture() {
  DEBUG_PRINT("Check soil moisture");
  
  // Soil Moisture
  digitalWrite(s1, HIGH);
  digitalWrite(s2, LOW);
  delay(10);
  sensorSoilMoisture = 1023 - analogRead(A0);
  DEBUG_PRINT("Soil Moisture RAW: " + String(sensorSoilMoisture));
  sensorSoilMoisture = (sensorSoilMoisture + soilMoistureOffset)*soilMoistureMultiplier;
  DEBUG_PRINT("Soil Moisture: " + String(sensorSoilMoisture));
   
  // Power off sensors
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);

  if (sensorSoilMoisture > wateringThreshold) {
    digitalWrite(power, LOW);
    DEBUG_PRINT("Device in sleep mode");
    ESP.deepSleep(10 * 1000000);
  }

  delay(5000);
}

