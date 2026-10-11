#include <painlessMesh.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "sevina"
#define MESH_PASSWORD "123aja123"
#define MESH_PORT     5544

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const char* nodeName = "Node 1";

Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

// Mengirim data suhu dan kelembapan
void sendMessage() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("[ERROR] Gagal membaca sensor DHT!");
    return;
  }

  StaticJsonDocument<200> doc;
  doc["node"] = nodeName;
  doc["tipe"] = "suhu_node";
  doc["suhu"] = suhu;
  doc["kelembapan"] = kelembapan;

  String msg;
  serializeJson(doc, msg);

  mesh.sendBroadcast(msg);

  Serial.println("==============================");
  Serial.println("[KIRIM] Node 1 - Sensor DHT");
  Serial.printf("Suhu       : %.2f C\n", suhu);
  Serial.printf("Kelembapan : %.2f %%\n", kelembapan);
  Serial.println("==============================");
}

// Menerima data dari node lain
void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    Serial.println("[ERROR] Gagal membaca JSON!");
    return;
  }

  String namaNode = doc["node"].as<String>();

  Serial.println("==============================");
  Serial.print("[TERIMA] Data dari ");

  if (namaNode != "") {
    Serial.println(namaNode);
  } else {
    Serial.println("Node lain");
  }

  Serial.println("==============================");
}

// Mendeteksi koneksi node baru
void newConnectionCallback(uint32_t nodeId) {
  Serial.println("==============================");
  Serial.println("[MESH] Node baru terhubung!");
  Serial.println("[MESH] Koneksi berhasil terbentuk");
  Serial.println("==============================");
}

// Mendeteksi perubahan topologi
void changedConnectionCallback() {
  Serial.println("==============================");
  Serial.println("[MESH] Topologi jaringan berubah!");
  Serial.println("[MESH] Memeriksa perubahan koneksi...");
  Serial.println("==============================");
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  mesh.setDebugMsgTypes(ERROR | STARTUP);

  mesh.init(MESH_PREFIX, MESH_PASSWORD,
            &userScheduler, MESH_PORT);

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.println("==============================");
  Serial.println("NODE 1 - SENSOR DHT");
  Serial.println("Jaringan Mesh: sevina");
  Serial.println("Menunggu koneksi Mesh...");
  Serial.println("==============================");
}

void loop() {
  mesh.update();
}