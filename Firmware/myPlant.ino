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

// Time to sleep (in seconds):
const int sleepTimeS = 10;

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
  if (BlynkState::is(MODE_RUNNING)) {
    send_sensor_value();

    // Sleep
    DEBUG_PRINT("Device in sleep mode");
    ESP.deepSleep(sleepTimeS * 1000000);
  } 
}


/**************************************************************
 *
 *              myPlant App code
 *
 * The following code simulates plant watering system
 *
 **************************************************************/

static int power = 14;
static int s1 = 12;
static int s2 = 13;
static int clk = 15;

static int sensorLight = 0;
static int sensorBattery = 0;
static int sensorSoilMoisture = 0;
static int sensorAirHumidity = 0;
static int sensorAirTemperature = 0;
static bool isNotificationSent = false;


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
  delay(1);
  sensorSoilMoisture = 1023 - analogRead(A0);
  DEBUG_PRINT("Soil Moisture: " + String(sensorSoilMoisture));
   
  // Light level
  digitalWrite(s1, LOW);
  digitalWrite(s2, HIGH);
  delay(1);
  sensorLight = analogRead(A0);
  DEBUG_PRINT("Light level: " + String(sensorLight));

  // Battery level
  digitalWrite(s1, HIGH);
  digitalWrite(s2, HIGH);
  delay(1);
  sensorBattery = analogRead(A0);
  DEBUG_PRINT("Battery level: " + String(sensorBattery));

  // Air humidity
  sensorAirHumidity = sensor.readHumidity();
  DEBUG_PRINT("Air humidity: " + String(sensorAirHumidity));

  // Air temperature
  sensorAirTemperature = sensor.readTemperature();
  DEBUG_PRINT("Air temperature: " + String(sensorAirTemperature));

  // Power off sensors
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(power, LOW);
}


void send_sensor_value() {
  DEBUG_PRINT("Send sensors values to cloud");

  Blynk.virtualWrite(V3, sensorAirTemperature);
  Blynk.virtualWrite(V4, sensorAirHumidity);
  
  Blynk.virtualWrite(V7, sensorSoilMoisture);
  Blynk.virtualWrite(V8, sensorLight);
  Blynk.virtualWrite(V9, sensorBattery);
}


