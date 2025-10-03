#define _WIFIMGR_LOGLEVEL_ 4
#include <WiFiManager.h>              
#include <WiFiUdp.h>
#include <FirebaseESP32.h>            
#include <DHT.h>                      
#include <NTPClient.h>                

// ---- REPLACE WITH YOUR OWN INFO ----
#define API_KEY "Firebase_API_Key"
#define DATABASE_URL "Database_URL"  // e.g., "your-project-id.firebaseio.com"

// ---- DHT11 setup ----
#define DHTPIN 23   
#define DHTTYPE DHT22 //Change to DHT11 if you use DHT11 
DHT dht(DHTPIN, DHTTYPE);

// --- Declare WiFiManager object ---
WiFiManager wifiManager;
// --- Configure NTP Client to get time ---
WiFiUDP ntpUDP;
// Viet Nam timezone is GMT+7 (7 * 3600(s/hr) = 25200 (s))
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 7 * 3600, 60000); 

// ---- Declare Firebase objects ----
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//  Function to create a valid path for Firebase (Example: "2025-09-30/16-45-02")
String getFirebasePath() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm t;
  gmtime_r(&rawTime, &t); // Convert to tm structure in UTC

  char date_buf[20], time_buf[20];
  sprintf(date_buf, "%04d-%02d-%02d", 
                    t.tm_year + 1900, // tm_year is years since 1900
                    t.tm_mon + 1,     // tm_mon is 0-11
                    t.tm_mday);
  sprintf(time_buf, "%02d-%02d-%02d", 
                    t.tm_hour, 
                    t.tm_min, t.tm_sec);
  return String(date_buf) + "/" + String(time_buf);
}

void setup() {
    Serial.begin(115200);
    dht.begin();     

    // Starting WiFi Manager
    Serial.println("Starting WiFiManager...");
    // Check if already connected to WiFi
    if (WiFi.status() != WL_CONNECTED) {
        // If not, open config portal and wait until user enters correct info
        wifiManager.startConfigPortal("ESP32_WifiConfig");
    }
    
    Serial.println("");
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    timeClient.begin();
    // Wait for time to be synchronized
    Serial.print("Synchronizing time");
    while (!timeClient.update()) {
      timeClient.forceUpdate();
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nTime synchronized!");
    Serial.println("Current time: " + timeClient.getFormattedTime());

    // --- Configure Firebase ---
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // Assign callback function for token creation
    Firebase.begin(&config, &auth);
    Firebase.signUp(&config, &auth, "", "");  
    Firebase.reconnectWiFi(true);

    Serial.println("Setup complete. Starting loop in 1 seconds...");
    delay(1000);
}

void loop() {
    int humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
    } else {
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" *C");
    }

    // Only run if Firebase connection is successful
    if (Firebase.ready()) {
        // Get current path based on date and time
        String dataPath = "/DHT11_Data/" + getFirebasePath();

        // Create JSON object to send
        FirebaseJson json;
        json.set("temperature", temperature);
        json.set("humidity", humidity);

        Serial.println("Pushing data to Firebase...");
        Serial.println("Path: " + dataPath);

        // Send JSON data to Firebase Realtime Database
        if (Firebase.setJSON(fbdo, dataPath.c_str(), json)) {
            Serial.println(">> SUCCESS");
        } else {
            Serial.println(">> FAILED");
            Serial.println("REASON: " + fbdo.errorReason());
        }
    }
    // Wait 10 seconds for the next send
    delay(10000);
}