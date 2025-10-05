#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <time.h>

// ---- REPLACE WITH YOUR CREDENTIALS ----
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"
#define API_KEY "Firebase_API_Key"
#define DATABASE_URL "Database_URL" 
// e.g., "https://<project-id>-default-rtdb.asia-southeast1.firebasedatabase.app"

// ========== DHT22 ==========
#define DHTPIN   23
#define DHTTYPE  DHT22
DHT dht(DHTPIN, DHTTYPE);
#define tempThreshold  30.0
float temp = NAN;
float hum  = NAN;

// ========== I/O PINS ==========
#define BUZZER_PIN 4
#define LED_RED    16
#define LED_GREEN  18
#define LED_BLUE   19
#define BUTTON_PIN 22

// ========== NTP ==========
const char* ntpServer = "asia.pool.ntp.org";
const long  gmtOffset_sec = 25200;   // GMT+7
const int   daylightOffset_sec = 0;

// ========== Firebase (legacy FirebaseESP32.h library) ==========
FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

// ========== SYSTEM VARIABLES ==========
const unsigned long DEBOUNCE_MS = 50;
unsigned long lastPressMs = 0;

const unsigned long READ_INTERVAL_MS = 5000;
unsigned long lastReadMs = 0;

enum Mode { MODE_NORMAL = 0, MODE_ALERT = 1 };
Mode currentMode = MODE_NORMAL;

// ------------ UTILITIES ------------
void setRGB(bool r, bool g, bool b) {
  digitalWrite(LED_RED,   r ? HIGH : LOW);
  digitalWrite(LED_GREEN, g ? HIGH : LOW);
  digitalWrite(LED_BLUE,  b ? HIGH : LOW);
}

void showMode(Mode m) {
  if (m == MODE_NORMAL) {
    setRGB(false, false, false);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Mode: NORMAL");
    return;
  }
  // MODE_ALERT
  if (!isnan(temp)) {
    if (temp > tempThreshold) {
      setRGB(true, false, false);
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.printf("Mode: ALERT - WARNING! Temp: %.2f\n", temp);
    } else {
      setRGB(false, true, false);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.printf("Mode: ALERT - Temp normal. Temp: %.2f\n", temp);
    }
  } else {
    setRGB(false, false, false);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("ERROR: Cannot read from DHT sensor!");
  }
}

// ------------ STREAM CALLBACK ------------
void streamCallback(StreamData data) {
  Serial.printf("Stream: %s %s %s\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str());

  // /Config/mode → string
  if (data.dataPath() == "/mode" && data.dataType() == "string") {
    String newMode = data.stringData();
    Serial.print("Firebase mode changed to: "); Serial.println(newMode);
    currentMode = (newMode == "alert") ? MODE_ALERT : MODE_NORMAL;
    showMode(currentMode);
  }

  // /Config/threshold → int/float/double
  if (data.dataPath() == "/threshold" &&
      (data.dataType() == "int" || data.dataType() == "float" || data.dataType() == "double")) {
    tempThreshold = data.floatData();
    Serial.printf("Firebase threshold updated to: %.2f\n", tempThreshold);
    if (currentMode == MODE_ALERT) showMode(currentMode);
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Stream timeout, resuming...");
}

// ------------ BUTTON ------------
void handleButtonPress() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long now = millis();
    if (now - lastPressMs > DEBOUNCE_MS) {
      currentMode = (currentMode == MODE_NORMAL) ? MODE_ALERT : MODE_NORMAL;

      String modeStr = (currentMode == MODE_NORMAL) ? "normal" : "alert";
      Firebase.setString(fbdo, "/Config/mode", modeStr);

      showMode(currentMode);
      lastPressMs = now;
    }
    while (digitalRead(BUTTON_PIN) == LOW) delay(1); // wait for button release
  }
}

// ------------ READ & SEND DATA ------------
void readAndSendData() {
  unsigned long nowMs = millis();
  if (nowMs - lastReadMs < READ_INTERVAL_MS) return;
  lastReadMs = nowMs;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    temp = NAN; hum = NAN;
  } else {
    temp = t; hum = h;
    Serial.printf("Temp: %.2f C, Hum: %.1f %%\n", temp, hum);

    if (Firebase.ready()) {
        time_t now; time(&now);
        unsigned long ts = now;

        // Send data to /DHT_Data/<timestamp>
        String path = "/DHT_Data/" + String(ts);
        FirebaseJson json;
        json.set("temperature", temp);
        json.set("humidity", hum);

        Serial.print("Pushing to: "); Serial.println(path);
        if (!Firebase.setJSON(fbdo, path.c_str(), json)) {
        Serial.printf("FAILED to push to DHT_Data: %s\n", fbdo.errorReason().c_str());
        }

        // Update latest temperature and humidity in /Config
        Serial.println("Updating /Config with latest temp and hum...");
        Firebase.setFloat(fbdo, "/Config/latest/temperature", temp);
        Firebase.setFloat(fbdo, "/Config/latest/humidity", hum);
        Firebase.setInt(fbdo,   "/Config/latest/ts", (int)ts);
    }
  }
  if (currentMode == MODE_ALERT) showMode(currentMode);
}

// ------------ SETUP ------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  dht.begin();

  // WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Synchronizing time");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) { Serial.print("."); delay(500); }
  Serial.println("\nTime synchronized!");

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (!Firebase.signUp(&config, &auth, "", "")) {
    Serial.printf("SignUp FAILED: %s\n", config.signer.signupError.message.c_str());
  }
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Stream /Config
  if (!Firebase.beginStream(stream, "/Config")) {
    Serial.println("Can't begin stream on /Config");
    Serial.println("REASON: " + stream.errorReason());
  }
  Firebase.setStreamCallback(stream, streamCallback, streamTimeoutCallback);

  // Initial values
  Serial.println("Getting initial config...");
  if (Firebase.getString(fbdo, "/Config/mode"))
    currentMode = (fbdo.stringData() == "normal") ? MODE_NORMAL : MODE_ALERT;
  if (Firebase.getFloat(fbdo, "/Config/threshold"))
    tempThreshold = fbdo.floatData();

  Serial.printf("Initial Mode: %s\n", (currentMode == MODE_ALERT ? "ALERT" : "NORMAL"));
  Serial.printf("Initial Threshold: %.2f\n", tempThreshold);

  showMode(currentMode);
  Serial.println("System ready.");
}

// ------------ LOOP ------------
void loop() {
  handleButtonPress();
  readAndSendData();
}