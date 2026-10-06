#include <ESP8266WiFi.h>
#include <espnow.h>

// MAC Address Receiver
uint8_t receiver1[] = {0xEC, 0xFA, 0xBC, 0x10, 0x62, 0x3C}; // Sela
uint8_t receiver2[] = {0xE8, 0xDB, 0x84, 0x88, 0x19, 0x9E}; // Lilis
uint8_t receiver3[] = {0x2C, 0xF4, 0x32, 0x8E, 0xA3, 0xD7}; // Mita

typedef struct struct_pesan {
  int perintahId;
  int nilaiParameter;
} struct_pesan;

struct_pesan paketKirim;

unsigned long previousMillis = 0;
const long interval = 2000;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  char macStr[18];

  snprintf(macStr, sizeof(macStr),
           "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Kirim paket ke: ");
  Serial.print(macStr);
  Serial.print(" | Status: ");

  if (sendStatus == 0) {
    Serial.println("Berhasil Diterima");
  } else {
    Serial.println("Gagal (Tidak Terjangkau)");
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("Gagal menginisialisasi ESP-NOW!");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  // Daftarkan Receiver Sela
  esp_now_add_peer(
    receiver1,
    ESP_NOW_ROLE_SLAVE,
    1,
    NULL,
    0
  );

  // Daftarkan Receiver Lilis
  esp_now_add_peer(
    receiver2,
    ESP_NOW_ROLE_SLAVE,
    1,
    NULL,
    0
  );

  // Daftarkan Receiver Mita
  esp_now_add_peer(
    receiver3,
    ESP_NOW_ROLE_SLAVE,
    1,
    NULL,
    0
  );

  Serial.println("Controller Sekar ESP-NOW Siap!");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    paketKirim.perintahId = 101;
    paketKirim.nilaiParameter = random(10, 100);

    // 0 = kirim ke semua peer yang sudah didaftarkan
    esp_now_send(
      0,
      (uint8_t *) &paketKirim,
      sizeof(paketKirim)
    );
  }
}