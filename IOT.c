código criado via I.A pra teste do hardware. // main.ino (ESP32)
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 4          // ajuste conforme seu wiring (GPIO4 = D2 em alguns módulos)
#define DHTTYPE DHT22     // DHT11 ou DHT22

const char* ssid     = "SEU_SSID";
const char* password = "SUA_SENHA";
const char* mqtt_server = "192.168.1.100"; // IP do broker (ou localhost se rodando no mesmo host da rede)

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
const long intervalMs = 10000; // leitura a cada 10s

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Tentando MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando de novo em 2s");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > intervalMs) {
    lastMsg = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature(); // Celsius

    // valida leitura
    if (isnan(h) || isnan(t)) {
      Serial.println("Falha ao ler do DHT");
      continue;
    }

    // Formata JSON sem usar String (evita alocação dinâmica)
    char payload[64];
    int len = snprintf(payload, sizeof(payload), "{\"temp\":%.2f,\"hum\":%.2f}", t, h);
    if (len > 0 && len < (int)sizeof(payload)) {
      client.publish("home/esp/dht", payload);
      Serial.print("Publicado: ");
      Serial.println(payload);
    } else {
      Serial.println("Erro ao formatar payload");
    }
  }
}
