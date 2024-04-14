#include <WiFiNINA.h>
#include "secret.h"
#include <Wire.h>
#include <BH1750.h>

char ssid[] = SECRET_SSID; // Wi-Fi network SSID
char pass[] = SECRET_PASS; // Wi-Fi network password

WiFiClient client; // Wi-Fi client object for making HTTP requests

const char* HOST_NAME = "maker.ifttt.com"; // IFTTT server hostname
String PATH_NAME = "/trigger/Light/with/key/" + String(SECRET_KEY); // IFTTT webhook path with API key

BH1750 lightMeter; // BH1750 light sensor object

bool inSunlight = false; // Flag to track if terrarium is in sunlight
unsigned long sunlightStart = 0; // Time when sunlight exposure started (in milliseconds)
const unsigned long sunlightDuration = 3 * 60 * 1000; // 3 minutes in milliseconds
bool dayDone = false; // Flag to track if sunlight exposure for the day is completed

void setup() {
  WiFi.begin(ssid, pass); // Connect to Wi-Fi network using provided SSID and password

  Serial.begin(9600); // Initialize serial communication
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi");

  Wire.begin(); // Initialize I2C communication
  lightMeter.begin(); // Initialize BH1750 light sensor

  Serial.println("BH1750 Test begin"); // Print message to serial monitor
}

float readLightLevel() {
  return lightMeter.readLightLevel(); // Read light level from BH1750 sensor
}

void sendWebhook(String message) {
  if (client.connect(HOST_NAME, 80)) { // Connect to IFTTT server
    Serial.println("Connected to server");

    String getRequest = "GET " + PATH_NAME + "?value1=" + message + " HTTP/1.1\r\n" +
                        "Host: " + String(HOST_NAME) + "\r\n" +
                        "Connection: close\r\n\r\n";

    client.print(getRequest); // Send HTTP GET request with webhook parameters

    unsigned long timeout = millis() + 5000; // Timeout in 5 seconds
    while (!client.available() && millis() < timeout) {
      delay(100);
    }

    if (client.available()) {
      while (client.available()) {
        char c = client.read();
        Serial.print(c); // Print response from server to serial monitor
      }
      Serial.println();
    } else {
      Serial.println("No response from server");
    }

    client.stop(); // Disconnect from server
    Serial.println("Disconnected from server");
  } else {
    Serial.println("Connection to server failed"); // Print error message if connection fails
  }
}

void loop() {
  float lux = readLightLevel(); // Read current light level
  
  if (lux <= 2 && dayDone) {
    dayDone = false; // Reset dayDone flag if light level drops below threshold
  }

  if (lux > 5 && !inSunlight && !dayDone) {
    inSunlight = true; // Set inSunlight flag to true
    sunlightStart = millis(); // Record start time of sunlight exposure
    sendWebhook("Terrarium in sunlight, 3 minutes remaining"); // Send webhook notification
  }

  if (inSunlight) {
    unsigned long currentTime = millis(); // Get current time
    unsigned long elapsedTime = currentTime - sunlightStart; // Calculate elapsed time

    if (elapsedTime >= sunlightDuration) { // If sunlight duration has elapsed
      sendWebhook("3 minutes reached, cover terrarium"); // Send webhook notification
      inSunlight = false; // Reset inSunlight flag
      dayDone = true; // Set dayDone flag to true
    }
  }

  delay(1000); // Short delay for stability 
}
