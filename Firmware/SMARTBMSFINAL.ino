#include <WiFi.h>
#include <PubSubClient.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

// --- CONFIGURATION ---
const char* ssid = "Airtel_mode_0746";
const char* password = "air71632";
const char* mqtt_server = "broker.hivemq.com";

#define MOSFET_PIN 4    
#define NTC_PIN 34      
#define BUILTIN_LED 2
#define I2C_SDA 21
#define I2C_SCL 22

// --- KALMAN FILTER (EKF) ---
class KalmanFilter {
  public:
    KalmanFilter(float q, float r, float p, float initial_value) {
      _q = q; _r = r; _p = p; _x = initial_value;
    }
    float update(float measurement) {
      _p = _p + _q;
      _k = _p / (_p + _r);
      _x = _x + _k * (measurement - _x);
      _p = (1 - _k) * _p;
      return _x;
    }
  private:
    float _q, _r, _p, _x, _k;
};

KalmanFilter voltFilter(0.01, 0.1, 1.0, 4.0);
KalmanFilter currFilter(0.1, 0.5, 1.0, 0.0);

// --- OBJECTS ---
Adafruit_SSD1306 display(128, 64, &Wire, -1); // -1 means no hardware reset pin
Adafruit_INA219 ina219;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebSocketsServer webSocket = WebSocketsServer(81);

// --- GLOBALS ---
volatile float shared_V = 0;
volatile float shared_mA = 0;
volatile float shared_Temp = 0;
int idle_counter = 0;

// --- TASK 1: SAFETY & DISPLAY ---
void BMS_Safety_Task(void * pvParameters) {
  for(;;) {
    // Read Sensors
    shared_V = voltFilter.update(ina219.getBusVoltage_V());
    shared_mA = currFilter.update(ina219.getCurrent_mA());
    
    int analogVal = analogRead(NTC_PIN);
    float resistance = 10000.0 / (4095.0 / (float)analogVal - 1.0);
    shared_Temp = 1.0 / (log(resistance / 10000.0) / 3950.0 + 1.0 / 298.15) - 273.15;

    // Logic: Inverted PWM (0 = Full ON, 255 = Full OFF due to NPN Transistor)
    int throttle = 100;
    if (shared_Temp >= 35.0) {
      throttle = map(constrain(shared_Temp, 35, 45), 35, 45, 100, 0);
    }
    if (shared_V < 3.2) throttle = 0; 

    int pwmValue = map(throttle, 0, 100, 255, 0); 
    analogWrite(MOSFET_PIN, pwmValue);

    // OLED Update
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    display.print("BMS: "); display.println(throttle < 100 ? "THROTTLING" : "OPTIMAL");
    
    display.setCursor(0, 15);
    display.setTextSize(2);
    display.print(shared_V, 1); display.print("V ");
    
    display.setTextSize(1);
    display.setCursor(70, 20);
    display.print(shared_Temp, 1); display.print("C");
    
    // Progress bar for throttle
    display.drawRect(0, 45, 128, 15, SSD1306_WHITE);
    display.fillRect(2, 47, map(throttle, 0, 100, 0, 124), 11, SSD1306_WHITE);
    display.display();

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

// --- TASK 2: IOT & LOGGING ---
void IoT_Comm_Task(void * pvParameters) {
  for(;;) {
    if (WiFi.status() == WL_CONNECTED) {
      ArduinoOTA.handle();
      if (!mqttClient.connected()) mqttClient.connect("SmartBMS_ESP32");
      mqttClient.loop();

      char buffer[128];
      snprintf(buffer, 128, "{\"v\":%.2f,\"i\":%.1f,\"t\":%.1f}", shared_V, shared_mA, shared_Temp);
      mqttClient.publish("smartBMS/live", buffer);
      webSocket.broadcastTXT(buffer);
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH); // Start OFF (Inverted logic)

  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Robust Display Init
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed!");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 25);
    display.setTextSize(1);
    display.print("INITIALIZING BMS...");
    display.display();
  }

  ina219.begin();
  WiFi.begin(ssid, password);
  ArduinoOTA.begin();

  xTaskCreatePinnedToCore(BMS_Safety_Task, "BMS", 10000, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(IoT_Comm_Task, "IoT", 10000, NULL, 1, NULL, 0);
}

void loop() {}