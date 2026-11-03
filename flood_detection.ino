#define BLYNK_TEMPLATE_ID "BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "BLYNK_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// Wi-Fi credentials
char ssid[] = "userwifi";
char pass[] = "password";

// Sensor pins
#define WATER_SENSOR_PIN 34
#define DHT_PIN 4
#define DHT_TYPE DHT11

// LED and buzzer pins
#define RED_LED 18
#define YELLOW_LED 19    
#define GREEN_LED 21
#define BUZZER_PIN 23

// DHT11 setup
DHT dht(DHT_PIN, DHT_TYPE);

// Example thresholds: calibrate using actual sensor readings
#define WARNING_THRESHOLD 10
#define FLOOD_THRESHOLD  25

BlynkTimer timer;

// Previous flood status
int previousStatus = -1;

// Status values:
// 0 = NORMAL
// 1 = WARNING
// 2 = FLOOD DANGER

void readSensors()
{
  // Read water level
 int waterRaw = analogRead(WATER_SENSOR_PIN);

 // Convert ESP32 sensor reading to 0–30 cm
 int waterLevel = map(waterRaw, 0, 2700, 0, 30);

 // Keep value within 0–30
 waterLevel = constrain(waterLevel, 0, 30);

 Serial.print("Raw Water Value: ");
 Serial.println(waterRaw);

 Serial.print("Water Level in cm: ");
 Serial.println(waterLevel);  

    // V0: Water level gauge
 // Send only 0–30 value to Blynk
 Blynk.virtualWrite(V0, waterLevel);
  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.println("\n-------------------------");

  Serial.print("Water Level: ");
  Serial.println(waterLevel);

  // V1: Temperature gauge
  // V2: Humidity gauge
  if (!isnan(temperature) && !isnan(humidity))
  {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, humidity);
  }
  else
  {
    Serial.println("DHT11 reading failed!");
  }

  // Determine flood status
  int currentStatus;

  if (waterLevel >= FLOOD_THRESHOLD)
  {
    currentStatus = 2;
  }
  else if (waterLevel >= WARNING_THRESHOLD)
  {
    currentStatus = 1;
  }
  else
  {
    currentStatus = 0;
  }

  // Turn off all physical outputs first
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // V3: Blynk LED widget
  // ON only during flood danger
  Blynk.virtualWrite(V3, currentStatus == 2 ? 1 : 0);

  // V4: Blynk label for flood condition
  if (currentStatus == 0)
  {
    digitalWrite(GREEN_LED, HIGH);

    Blynk.virtualWrite(V4, "NORMAL");

    Serial.println("STATUS: NORMAL");
  }
  else if (currentStatus == 1)
  {
    digitalWrite(YELLOW_LED, HIGH);

    Blynk.virtualWrite(V4, "WARNING");

    Serial.println("STATUS: WARNING - WATER LEVEL RISING");
  }
  else
  {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    Blynk.virtualWrite(V4, "FLOOD DANGER");

    Serial.println("STATUS: FLOOD DANGER!");
  }

  // Notify the owner when flood danger begins
  if (currentStatus == 2 && previousStatus != 2)
  {
    Blynk.logEvent(
      "flood_alert",
      "FLOOD WARNING! High water level detected. "
      "Please check the location immediately."
    );
  }

  previousStatus = currentStatus;

  Serial.println("-------------------------");
}

void setup()
{
  Serial.begin(115200);

  // Configure output pins
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Configure analog sensor resolution
  analogReadResolution(12);

  // Start DHT11
  dht.begin();

  Serial.println("IoT Flood Detection System Starting...");

  // Connect to Wi-Fi and Blynk
  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );

  // Read sensors every 2 seconds
  timer.setInterval(2000L, readSensors);
}

void loop()
{
  Blynk.run();
  timer.run();
}