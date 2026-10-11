#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "sevina"
#define MESH_PASSWORD "123aja123"
#define MESH_PORT     5544

#define LDRPIN A0

const char* nodeName = "Node 2";

Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

// Membaca dan mengirim nilai LDR
void sendMessage() {
  int adc = analogRead(LDRPIN);

  StaticJsonDocument<200> doc;
  doc["node"] = nodeName;
  doc["tipe"] = "cahaya_node";
  doc["adc"] = adc;

  String msg;
  serializeJson(doc, msg);

  mesh.sendBroadcast(msg);

  Serial.println("==============================");
  Serial.println("[KIRIM] Node 2 - Sensor LDR");
  Serial.printf("ADC Cahaya : %d\n", adc);
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

  mesh.setDebugMsgTypes(ERROR | STARTUP);

  mesh.init(MESH_PREFIX, MESH_PASSWORD,
            &userScheduler, MESH_PORT);

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.println("==============================");
  Serial.println("NODE 2 - SENSOR LDR");
  Serial.println("Jaringan Mesh: sevina");
  Serial.println("Menunggu koneksi Mesh...");
  Serial.println("==============================");
}

void loop() {
  mesh.update();
}