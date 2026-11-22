#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --- Lábkiosztások ---
#define DHTPIN        5     // D5: DHT11 adatvonal (S)
#define DHTTYPE       DHT11
#define FLAME_PIN     18    // D18: Lángérzékelő D0
#define SERVO_PIN     13    // D13: Szervómotor jelvezeték
#define BUZZER_PIN    25    // D25: Buzzer modul (S)
#define LED_R         23    // D23: RGB Piros
#define LED_G         22    // D22: RGB Zöld
#define LED_B         21    // D21: RGB Kék

// --- Küszöbértékek és szervó szögek ---
const float TEMP_HIGH = 20.0; // E felett nyit ki az ajtó (°C)
const int DOOR_CLOSED = 115;  // Zárt ajtó szöge (nem ütközik a keretbe; 110-130 között finomítható)
const int DOOR_OPEN   = 180;  // Nyitott ajtó szöge

// --- Hálózati beállítások (futtatás előtt töltsd ki a saját adataiddal!) ---
const char* ssid        = "SAJAT_WIFI_NEV";      // <-- Ide írd a saját Wi-Fi hálózatod nevét
const char* password    = "SAJAT_WIFI_JELSZO";   // <-- Ide írd a saját Wi-Fi jelszavadat
const char* mqtt_server = "192.168.X.X";         // <-- Ide írd a helyi Mosquitto broker IP-címét
const int   mqtt_port   = 1883;

const char* topic_telemetry = "serverszoba/telemetria";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0;
unsigned long lastReconnectAttempt = 0;
bool alarmActive = false;
int lastServoAngle = -1;

void setRgbColor(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}

void moveServoSafe(int angle) {
  if (angle == lastServoAngle) return;
  lastServoAngle = angle;

  // Teljes 0-180° tartomány (0.5ms - 2.5ms)
  int duty = map(angle, 0, 180, 1638, 8192);
  ledcWrite(SERVO_PIN, duty);
  delay(700);               // Megvárja a teljes elfordulást
  ledcWrite(SERVO_PIN, 0);  // Leveszi a jelet, hogy ne zúgjon és ne melegedjen
}

void playAlarmBeep() {
  for (int i = 0; i < 150; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(250);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(250);
  }
}

bool reconnect() {
  String clientId = "ESP32Serverszoba-" + String(random(0xffff), HEX);
  if (client.connect(clientId.c_str())) {
    Serial.println("[MQTT] Sikeresen visszacsatlakozva!");
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  dht.begin();

  ledcAttach(SERVO_PIN, 50, 16);

  // Indulási teszt: kinyit, majd beáll a zárt pozícióba (115°)
  setRgbColor(false, true, false);
  moveServoSafe(DOOR_OPEN);
  moveServoSafe(DOOR_CLOSED);
  setRgbColor(false, false, true);

  Serial.print("\n[WiFi] Csatlakozás: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Csatlakozva! IP: " + WiFi.localIP().toString());

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(500);
    return;
  }

  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 2000) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }

  if (alarmActive) {
    playAlarmBeep();
    delay(150);
  }

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int flameRaw = digitalRead(FLAME_PIN);

    bool flameDetected = (flameRaw == HIGH);

    if (isnan(temp)) temp = 0.0;
    if (isnan(hum)) hum = 0.0;

    // --- SZERVERSZOBA LOGIKA ---
    if (flameDetected) {
      // 1. TŰZRIADÓ: Bezár (115°), Piros LED, Buzzer pittyeg
      moveServoSafe(DOOR_CLOSED);
      setRgbColor(true, false, false);
      alarmActive = true;
    } 
    else if (temp > TEMP_HIGH) {
      // 2. MAGAS HŐMÉRSÉKLET (20°C felett): Kinyit (180°), Zöld LED, Buzzer néma
      moveServoSafe(DOOR_OPEN);
      setRgbColor(false, true, false);
      alarmActive = false;
    } 
    else {
      // 3. NORMÁL ÁLLAPOT: Zárva marad (115°), Kék LED, Buzzer néma
      moveServoSafe(DOOR_CLOSED);
      setRgbColor(false, false, true);
      alarmActive = false;
    }

    StaticJsonDocument<256> doc;
    doc["temperature"] = temp;
    doc["humidity"] = hum;
    doc["flame"] = flameDetected;

    char buffer[256];
    serializeJson(doc, buffer);

    if (client.connected()) {
      client.publish(topic_telemetry, buffer);
    }
  }
}