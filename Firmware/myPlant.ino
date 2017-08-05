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

#define APP_DEBUG        // Comment this out to disable debug prints

#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include "BlynkProvisioning.h"
#include "Adafruit_Si7021.h"

// Alert maeesages
#define URGENT_WATER ": URGENT! Water me! :O "
#define WATER ": Water me please. :( "
#define THANK_YOU ": Thank you for watering me! :) "
#define OVER_WATERED ": You over watered me. "
#define UNDER_WATERED ": You didn't water me enough. "

//tracks the state to avoid erroneously repeated tweets
#define URGENT_SENT 3
#define WATER_SENT 2
#define MOISTURE_OK 1

#define HYSTERESIS 25 // stabilization value http://en.wikipedia.org/wiki/Hysteresis

static int state = MOISTURE_OK; // tracks which messages have been sent
static int counter = 0;
static int power = 14;
static int sleepTimeS = 10;
static int wateringThreshold = 20;
static int isParametersReceived = 0;

static bool isNotificationSent = true;


Adafruit_Si7021 sensor = Adafruit_Si7021();


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
}

void loop() {
  // This handles the network and cloud connection
  BlynkProvisioning.run();

  // Send sensors value to cloud when ready
  if ((isParametersReceived == 5) && BlynkState::is(MODE_RUNNING)) {
    send_sensor_value();
    send_alert();
    send_map_value();

    if (state > MOISTURE_OK) {
      BlynkState::set(MODE_THIRSTY);
    }
    else {
      digitalWrite(power, LOW);
      DEBUG_PRINT("Device in sleep mode");
      ESP.deepSleep(sleepTimeS * 1000000);
    } 
  }

  if (BlynkState::is(MODE_THIRSTY)) {
    delay(10000);
    sensor_init();
    sensor_read();
    send_sensor_value();
    send_alert();
    send_map_value(); 

    if (state == MOISTURE_OK) {
      BlynkState::set(MODE_WATER);
      delay(5000);
      digitalWrite(power, LOW);
      DEBUG_PRINT("Device in sleep mode");
      ESP.deepSleep(sleepTimeS * 1000000);
    }
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

WidgetMap myMap(V0);
int mapIndex = 1;

String alertMessage = "OK";

// Calibration factors
static int soilMoistureOffset = 0;
static float soilMoistureGain = 1.0;
const int lightOffset = -46;
const float lightGain = 0.104;
const int batteryOffset = -630;
const float batteryGain = 0.4;

// Getting data from "soil offset" step setting
BLYNK_WRITE(V5) {
  soilMoistureOffset = param.asInt() * -1;
  DEBUG_PRINT(String("Soil moisture offset: ") + soilMoistureOffset);
  isParametersReceived += 1;
}

// Getting data from "soil gain" step setting
BLYNK_WRITE(V6) {
  soilMoistureGain = param.asInt() / 100.0;
  DEBUG_PRINT(String("Soil moisture Gain: ") + soilMoistureGain);
  isParametersReceived += 1;
}

// Getting data from "Refresh Rate" step setting
BLYNK_WRITE(V10) {
  sleepTimeS = param.asInt() * 60;
  if (sleepTimeS == 0) {
    sleepTimeS = 10;
  } 
  DEBUG_PRINT(String("Refresh rate: ") + sleepTimeS);
  isParametersReceived += 1;
}

// Getting data from "Watering threshold" step setting
BLYNK_WRITE(V11) {
  wateringThreshold = param.asInt();
  DEBUG_PRINT(String("Watering threshold: ") + wateringThreshold);
  isParametersReceived += 1;
}

// Getting data from "map index" step setting
BLYNK_WRITE(V12) {
  mapIndex = param.asInt();
  DEBUG_PRINT(String("Map index: ") + mapIndex);
  isParametersReceived += 1;
}

// When device starts ->
//   sync app parameters
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V5, V6, V10, V11, V12);
}


void sensor_init() {
  DEBUG_PRINT("Init sensors");
  pinMode(power, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  
  digitalWrite(power, HIGH);

  analogWriteFreq(40000);
  analogWrite(clk, 100);

  delay(200);

  sensor.begin();
}


void sensor_read() {
  DEBUG_PRINT("Read sensor");
  
  // Soil Moisture
  digitalWrite(s1, HIGH);
  digitalWrite(s2, LOW);
  delay(100);
  sensorSoilMoisture = 1023 - analogRead(A0);
  DEBUG_PRINT("Soil Moisture RAW: " + String(sensorSoilMoisture));
  //sensorSoilMoisture = (sensorSoilMoisture + soilMoistureOffset)*soilMoistureGain;
  //DEBUG_PRINT("Soil Moisture: " + String(sensorSoilMoisture));
   
  // Light level
  digitalWrite(s1, LOW);
  digitalWrite(s2, HIGH);
  delay(100);
  sensorLight = analogRead(A0);
  DEBUG_PRINT("Light level RAW: " + String(sensorLight));
  sensorLight = (sensorLight + lightOffset)*lightGain;
  DEBUG_PRINT("Light level: " + String(sensorSoilMoisture));

  // Battery level
  digitalWrite(s1, HIGH);
  digitalWrite(s2, HIGH);
  delay(100);
  sensorBattery = analogRead(A0);
  DEBUG_PRINT("Battery level RAW: " + String(sensorBattery));
  sensorBattery = (sensorBattery + batteryOffset)*batteryGain;
  DEBUG_PRINT("Battery level: " + String(sensorBattery));

  // Air humidity
  sensorAirHumidity = sensor.readHumidity();
  DEBUG_PRINT("Air humidity: " + String(sensorAirHumidity));

  // Air temperature
  sensorAirTemperature = sensor.readTemperature();
  DEBUG_PRINT("Air temperature: " + String(sensorAirTemperature));

  // Init rgb led
  indicator.initLED();

  // Unselect sensors
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
}


void send_sensor_value() {
  DEBUG_PRINT("Send sensors values to cloud");

  Blynk.virtualWrite(V3, sensorAirTemperature);
  Blynk.virtualWrite(V4, sensorAirHumidity);

  // Soil moisture
  sensorSoilMoisture = (sensorSoilMoisture + soilMoistureOffset)*soilMoistureGain;
  if (sensorSoilMoisture < 30) {
    Blynk.virtualWrite(V1, "DRY");
  } else if (sensorSoilMoisture > 80) {
    Blynk.virtualWrite(V1, "WET");
  } else {
    Blynk.virtualWrite(V1, "MOIST");
  }
  Blynk.virtualWrite(V7, sensorSoilMoisture);

  // Light level
  if (sensorLight < 10) {
    Blynk.virtualWrite(V2, "LOW");
  } else if (sensorLight > 80) {
    Blynk.virtualWrite(V2, "HIGH");
  } else {
    Blynk.virtualWrite(V2, "MED");
  }
  Blynk.virtualWrite(V8, sensorLight);

  if (sensorBattery > 100) {
    sensorBattery = 100;
  }
  Blynk.virtualWrite(V9, sensorBattery); 
}


void send_map_value() {
  String latVal = getValue(configStore.plantCoordinate, ',', 0);
  String lonVal = getValue(configStore.plantCoordinate, ',', 1);
  float lat = latVal.toFloat();
  float lon = lonVal.toFloat();
  myMap.location(mapIndex, lat, lon, String(configStore.plantName) + ": " + alertMessage); 
}


void send_alert() {
  state = configStore.state;
  counter = configStore.counter;
  DEBUG_PRINT("State: " + String(state) + " & Counter: " + String(counter));
  
  if ((sensorSoilMoisture  < (wateringThreshold - 10)) && (state < URGENT_SENT)) {
    DEBUG_PRINT("URGENT tweet");
    alertMessage = URGENT_WATER;
    state = URGENT_SENT; // remember this message
    isNotificationSent = false;
  }
  else if  ((sensorSoilMoisture < wateringThreshold) && (state < WATER_SENT)) {
    DEBUG_PRINT("WATER tweet");
    alertMessage = WATER;
    state = WATER_SENT; // remember this message
    isNotificationSent = false;
  }
  else if ((sensorSoilMoisture > (wateringThreshold + HYSTERESIS)) && (state > MOISTURE_OK)) {
    DEBUG_PRINT("OK tweet");
    alertMessage = THANK_YOU;
    state = MOISTURE_OK; // reset to messages not yet sent state
    isNotificationSent = false;
  }

  if (isNotificationSent == false) {
    Blynk.tweet(String(configStore.plantName) + String(alertMessage) + " [ID-" + String(counter) + "] ");
    Blynk.notify(String(configStore.plantName) + String(alertMessage));
    isNotificationSent = true;
    DEBUG_PRINT("Notification sent");
    counter += 1;
    configStore.state = state;
    configStore.counter = counter;
    config_save();
  }

  if ((sensorSoilMoisture  < (wateringThreshold - 10)) && (state < URGENT_SENT)) {
    DEBUG_PRINT("URGENT tweet");
    alertMessage = URGENT_WATER;
    state = URGENT_SENT; // remember this message
    isNotificationSent = false;
  }
}


String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


