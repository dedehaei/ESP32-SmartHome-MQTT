#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <DHT.h> 

// Konfigurasi WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Konfigurasi MQTT
const char* mqtt_server = "mqtt-dashboard.com";
const int mqtt_port = 1883;
const char* mqtt_username = "DaffaDandi";
const char* mqtt_password = "Daffabulet8927";

const int loadPin = 4;  // ST_CP (RCLK)
const int clockEnablePin = 2;  // SH_CP (SRCLK)
const int dataPin = 15;  // DS (SER)


int switches[8] = {0,0,0,0,0,0,0,0};

byte switchStates = 0; // Untuk menyimpan status saklar fisik


// Definisi pin relay
const int relayPins[] = {13,12,14,27,26,25,33,32};
const int numRelays = 8;

bool relayStatus[numRelays];

// Konfigurasi DHT22
#define DHT1PIN 5        // Pin data DHT22 pertama terhubung ke GPIO5 (D5)
#define DHT2PIN 18        // Pin data DHT22 kedua terhubung ke GPIO4 (D4)
#define DHTTYPE DHT22   // Tipe sensor DHT22 (DHT22)

DHT dht1(DHT1PIN, DHTTYPE);
DHT dht2(DHT2PIN, DHTTYPE);

unsigned long previousMillis = 0;
const long interval = 500;  // Interval pembacaan setiap 5 detik


// Inisialisasi objek MQTT
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  // Proses pesan MQTT yang diterima di sini
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Memproses pesan berdasarkan topik
  for (int i = 0; i < numRelays; i++) {
    if (String(topic) == "relay/" + String(i + 1)) {
      if (message == "ON") {
        digitalWrite(relayPins[i], HIGH); // Aktifkan relay
      } else if (message == "OFF") {
        digitalWrite(relayPins[i], LOW); // Matikan relay
      }
      break; // Keluar dari loop setelah menemukan topik yang cocok
    }
  }
}

void reconnect() {
  // Loop hingga terhubung ke broker MQTT
  while (!client.connected()) {
    Serial.println("Mencoba terhubung ke MQTT...");
    if (client.connect("Dedehaei", mqtt_username, mqtt_password)) {
      Serial.println("Terhubung ke MQTT");
      // Berlangganan topik MQTT di sini
      client.subscribe("relay/1");
      client.subscribe("relay/2");
      client.subscribe("relay/3");
      client.subscribe("relay/4");
      client.subscribe("relay/5");
      client.subscribe("relay/6");
      client.subscribe("relay/7");
      client.subscribe("relay/8");
      // Berlangganan ke topik untuk relay lainnya
    } else {
      Serial.print("Gagal terhubung ke MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setRelay(int relayNum, int state) {
  // Mengatur status relay
  digitalWrite(loadPin, LOW);  // Aktifkan LOAD (ST_CP) untuk mengirim data ke shift register
  shiftOut(dataPin, clockEnablePin, MSBFIRST, B10000000 >> relayNum); // Pindahkan bit ke shift register sesuai dengan nomor relay
  digitalWrite(loadPin, HIGH); // Matikan LOAD (ST_CP) untuk mentrigger shift register

  // Simpan status relay ke dalam array
  relayStatus[relayNum] = state;

  // Kirim status relay ke MQTT
  String relayTopic = "relay/" + String(relayNum + 1);
  String payload = (relayStatus[relayNum] == HIGH) ? "ON" : "OFF";
  client.publish(relayTopic.c_str(), payload.c_str());
}

void readPhysicalSwitches() {
  
  digitalWrite(loadPin, LOW); // Aktifkan LOAD (ST_CP) untuk mengambil data dari shift register
  delayMicroseconds(10); // Tunggu sebentar sebelum membaca data
  digitalWrite(loadPin, HIGH); // Matikan LOAD (ST_CP)
  
  

  for(int i = 0; i < 8; i++) {
    switches[7-i] = digitalRead(dataPin);
    digitalWrite(clockEnablePin, HIGH);
    digitalWrite(clockEnablePin, LOW);
    Serial.print(switches[i]);
  }
  Serial.println();

  // then unlatch the register
  digitalWrite(loadPin, LOW);

  // Perbarui status relay jika berubah
  for (int i = 0; i < numRelays; i++) {
    if (switches[i] != relayStatus[i]) {
      setRelay(i, switches[i]);
    }
  }
}

void setup() {
  
  pinMode(loadPin, OUTPUT);
  pinMode(clockEnablePin, OUTPUT);
  pinMode(dataPin, INPUT_PULLUP); // Data pin diatur sebagai input pull-up
  digitalWrite(loadPin, LOW);

  // Inisialisasi pin relay sebagai OUTPUT
  for (int i = 0; i < numRelays; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW); // Relay nonaktif awalnya
  }

  // Inisialisasi sensor DHT22
  dht1.begin();
  dht2.begin();

  // Mulai koneksi WiFi
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Koneksi WiFi...");
  }
  Serial.println("Terhubung ke WiFi");

  // Konfigurasi koneksi MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Koneksi ke broker MQTT
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  readPhysicalSwitches();

   unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // float temperature1 = dht1.readTemperature();
    // float humidity1 = dht1.readHumidity();

    float temperature1 = random(0,80);
    float humidity1 = random(0,100);

    if (!isnan(temperature1) && !isnan(humidity1)) {
      // Kirim data suhu dan kelembaban dari sensor pertama ke MQTT
      String temperatureTopic1 = "dht1/temperature";
      String humidityTopic1 = "dht1/humidity";
      client.publish(temperatureTopic1.c_str(), String(temperature1).c_str());
      client.publish(humidityTopic1.c_str(), String(humidity1).c_str());
    }
    
    // Baca data suhu dan kelembaban dari sensor DHT22 kedua
    // float temperature2 = dht2.readTemperature();
    // float humidity2 = dht2.readHumidity();
    float temperature2 = random(0,80);
    float humidity2 = random(0,100);
    
    if (!isnan(temperature2) && !isnan(humidity2)) {
      // Kirim data suhu dan kelembaban dari sensor kedua ke MQTT
      String temperatureTopic2 = "dht2/temperature";
      String humidityTopic2 = "dht2/humidity";
      client.publish(temperatureTopic2.c_str(), String(temperature2).c_str());
      client.publish(humidityTopic2.c_str(), String(humidity2).c_str());
    }
  }
}
