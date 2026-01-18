#include "WebServer.h"
#include "WiFi.h"

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

bool ledState = false;
unsigned long startMillis;

String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Control</title>
<style>
body {
  background: radial-gradient(circle, #0f2027, #203a43, #2c5364);
  color: #00ffcc;
  font-family: Arial;
  text-align: center;
  padding-top: 30px;
}
h1 {
  text-shadow: 0 0 15px #00ffcc;
}
button {
  font-size: 22px;
  padding: 15px 40px;
  border-radius: 40px;
  border: none;
  background: linear-gradient(45deg, #00ffcc, #00ffaa);
  color: black;
  cursor: pointer;
  box-shadow: 0 0 20px #00ffcc;
  transition: 0.2s;
}
button:hover {
  transform: scale(1.05);
  box-shadow: 0 0 40px #00ffcc;
}
.card {
  margin: 20px auto;
  padding: 15px;
  width: 90%;
  max-width: 400px;
  border: 1px solid #00ffcc;
  border-radius: 12px;
  box-shadow: 0 0 15px #00ffcc;
}
</style>
</head>
<body>

<h1>ESP32 CYBER PANEL</h1>

<div class="card">
<p>LED Status: <b id="led">---</b></p>
<button onclick="toggle()">Toggle LED</button>
</div>

<div class="card">
<p>IP: <span id="ip"></span></p>
<p>WiFi RSSI: <span id="rssi"></span> dBm</p>
<p>Uptime: <span id="uptime"></span> sec</p>
</div>

<script>
function refresh(){
  fetch('/status')
  .then(r=>r.json())
  .then(d=>{
    document.getElementById("led").innerText = d.led ? "ON" : "OFF";
    document.getElementById("ip").innerText = d.ip;
    document.getElementById("rssi").innerText = d.rssi;
    document.getElementById("uptime").innerText = d.uptime;
  });
}

function toggle(){
  fetch('/toggle');
  setTimeout(refresh,200);
}

setInterval(refresh,2000);
refresh();
</script>

</body>
</html>
)rawliteral";
  return html;
}

void handleRoot() { server.send(200, "text/html", htmlPage()); }

void handleToggle() {
  ledState = !ledState;
  digitalWrite(2, ledState ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  unsigned long uptime = (millis() - startMillis) / 1000;

  String json = "{";
  json += "\"led\":" + String(ledState ? "true" : "false") + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"uptime\":" + String(uptime);
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  startMillis = millis();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("Web server started!");
}

void loop() { server.handleClient(); }
