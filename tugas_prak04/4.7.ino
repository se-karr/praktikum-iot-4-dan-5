#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Konfigurasi WiFi
const char* ssid = "kar";
const char* password = "12345678";

// Konfigurasi Pin
const byte dhtPin = 2;       // D4
const byte buttonPin = 4;    // D2
const byte ledPin = 12;      // D6
const byte pwmPin = 5;       // D1

DHT dht(dhtPin, DHT22);

// Variabel status
bool ledState = false;
String currentTemp = "--";
int nilaiPWM = 0;

int buttonState = LOW;
int lastButtonState = LOW;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastTime = 0;

// Web Server dan WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ===================================
// HTML, CSS, DAN JAVASCRIPT
// ===================================

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Lighting - PWM Slider</title>

  <style>
    body {
      font-family: Arial;
      text-align: center;
      background-color: #f0f0f0;
    }

    .card {
      background-color: white;
      margin: 20px auto;
      padding: 20px;
      max-width: 350px;
      border-radius: 10px;
    }

    button {
      padding: 15px 30px;
      font-size: 18px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      color: white;
    }

    .btn-on {
      background-color: #4CAF50;
    }

    .btn-off {
      background-color: #f44336;
    }

    input[type="range"] {
      width: 90%;
    }
  </style>
</head>

<body>

  <h1>Smart Lighting</h1>
  <h3>NodeMCU ESP8266 - WebSockets</h3>

  <div class="card">
    <h2>Monitoring Suhu</h2>
    <h2>
      <span id="tempValue">--</span> Celcius
    </h2>
  </div>

  <div class="card">
    <h2>Kontrol LED ON/OFF</h2>
    <h3>LED: <span id="ledStatus">OFF</span></h3>

    <button id="toggleBtn"
      class="btn-off"
      onclick="toggleLed()">
      Turn ON
    </button>
  </div>

  <div class="card">
    <h2>Kontrol Kecerahan LED</h2>
    <p>Geser slider untuk mengatur PWM</p>

    <input type="range"
      min="0"
      max="1023"
      value="0"
      id="pwmSlider"
      oninput="sendPWM(this.value)">

    <h3>
      Nilai PWM:
      <span id="pwmValue">0</span>
    </h3>
  </div>

<script>

  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;

  window.addEventListener('load', onLoad);

  function onLoad(event) {
    initWebSocket();
  }

  function initWebSocket() {
    websocket = new WebSocket(gateway);

    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
  }

  function onOpen(event) {
    console.log('WebSocket Terkoneksi');
  }

  function onClose(event) {
    console.log('WebSocket Terputus');
    setTimeout(initWebSocket, 2000);
  }

  // Mengontrol LED ON/OFF
  function toggleLed() {
    if (websocket.readyState === WebSocket.OPEN) {
      websocket.send('toggle');
    }
  }

  // Mengirim nilai PWM
  function sendPWM(value) {
    document.getElementById('pwmValue').innerHTML = value;

    if (websocket.readyState === WebSocket.OPEN) {
      websocket.send('pwm,' + value);
    }
  }

  // Menerima data dari NodeMCU
  function onMessage(event) {
    var dataObj = JSON.parse(event.data);

    // Menampilkan suhu
    if (dataObj.suhu !== undefined) {
      document.getElementById('tempValue').innerHTML =
        dataObj.suhu;
    }

    // Memperbarui status LED
    if (dataObj.led !== undefined) {
      var btn = document.getElementById('toggleBtn');
      var status = document.getElementById('ledStatus');

      if (dataObj.led == "1") {
        status.innerHTML = "ON";
        btn.innerHTML = "Turn OFF";
        btn.className = "btn-on";
      } else {
        status.innerHTML = "OFF";
        btn.innerHTML = "Turn ON";
        btn.className = "btn-off";
      }
    }

    // Sinkronisasi nilai slider
    if (dataObj.pwm !== undefined) {
      document.getElementById('pwmSlider').value = dataObj.pwm;
      document.getElementById('pwmValue').innerHTML = dataObj.pwm;
    }
  }

</script>
</body>
</html>
)rawliteral";

// ===================================
// BACK-END NODEMCU
// ===================================

// Mengirim data ke seluruh browser
void notifyClients() {
  String jsonString = "{\"led\":\"" +
                      String(ledState ? 1 : 0) + "\",";
  jsonString += "\"suhu\":\"" + currentTemp + "\",";
  jsonString += "\"pwm\":\"" + String(nilaiPWM) + "\"}";

  ws.textAll(jsonString);
}

// Menerima perintah dari WebSocket
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 &&
      info->len == len && info->opcode == WS_TEXT) {

    String pesan = "";

    for (size_t i = 0; i < len; i++) {
      pesan += (char)data[i];
    }

    // Perintah LED ON/OFF
    if (pesan == "toggle") {
      ledState = !ledState;
      digitalWrite(ledPin, ledState ? HIGH : LOW);

      Serial.println(ledState ? "LED D6 MENYALA" : "LED D6 MATI");

      notifyClients();
    }

    // Perintah PWM Slider
    else if (pesan.startsWith("pwm,")) {
      nilaiPWM = pesan.substring(4).toInt();
      nilaiPWM = constrain(nilaiPWM, 0, 1023);

      analogWrite(pwmPin, nilaiPWM);

      Serial.print("Nilai PWM LED D1: ");
      Serial.println(nilaiPWM);

      notifyClients();
    }
  }
}

// Event WebSocket
void onEvent(AsyncWebSocket *server,
             AsyncWebSocketClient *client,
             AwsEventType type,
             void *arg, uint8_t *data, size_t len) {

  switch (type) {
    case WS_EVT_CONNECT:
      Serial.println("Client WebSocket terhubung");
      notifyClients();
      break;

    case WS_EVT_DISCONNECT:
      Serial.println("Client WebSocket terputus");
      break;

    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pwmPin, OUTPUT);

  digitalWrite(ledPin, LOW);

  // Rentang PWM 0 sampai 1023
  analogWriteRange(1023);
  analogWrite(pwmPin, 0);

  dht.begin();

  // Koneksi WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Menghubungkan WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Terhubung!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Menampilkan halaman website
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.begin();

  Serial.println("Web Server Siap!");
  Serial.println("Sistem PWM Slider Berjalan!");
}

void loop() {
  ws.cleanupClients();

  // Kendali LED ON/OFF
  digitalWrite(ledPin, ledState ? HIGH : LOW);

  // Membaca push button
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        ledState = !ledState;
        notifyClients();
      }
    }
  }

  lastButtonState = reading;

  // Membaca suhu setiap 3 detik
  if ((millis() - lastTime) > 3000) {
    float t = dht.readTemperature();

    if (!isnan(t)) {
      currentTemp = String(t);
      notifyClients();
    }

    lastTime = millis();
  }
}
