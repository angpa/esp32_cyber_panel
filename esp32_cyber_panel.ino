#include "WebServer.h"
#include "WiFi.h"

// 🔐 Sanitized credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
unsigned long startMillis;

String html() {
  return R"rawliteral(
<!DOCTYPE html><html>
<head><meta name=viewport content="width=device-width, initial-scale=1">
<title>ESP32 Analyzer</title>
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
  font-size: 16px;
  padding: 10px 20px;
  border-radius: 40px;
  border: none;
  background: linear-gradient(45deg, #00ffcc, #00ffaa);
  color: black;
  cursor: pointer;
  box-shadow: 0 0 20px #00ffcc;
  transition: 0.2s;
  margin: 10px;
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
canvas { background: black; border: 1px solid #00ffcc; width: 100%; border-radius: 4px; }
pre { color: #00ffaa; text-align: left; font-size: 12px; overflow: auto; }
#qual { font-weight: bold; font-size: 18px; }
</style></head>
<body>

<h1>ESP32 RADIO ANALYZER</h1>

<div class=card>
IP: <span id=ip></span><br>
RSSI: <span id=rssi></span> dBm<br>
Average: <span id=avg></span> dBm<br>
Status: <span id=qual>---</span><br>
Heap: <span id=heap></span><br>
CPU: <span id=cpu></span> MHz<br>
Uptime: <span id=uptime></span> s
</div>

<div class=card>
RSSI Oscilloscope<br>
<canvas id=c width=320 height=140></canvas>
</div>

<div class=card>
<button onclick="scan()">WiFi Scan</button>
<pre id=w></pre>
</div>

<script>
let ctx=c.getContext("2d"),buf=[];

function classify(v){
 if(v>-60){qual.innerText="GOOD";qual.style.color="#00ff00";}
 else if(v>-75){qual.innerText="FAIR";qual.style.color="#ffff00";}
 else{qual.innerText="POOR";qual.style.color="#ff3333";}
}

function stat(){
fetch('/s').then(r=>r.json()).then(d=>{
ip.innerText=d.ip;
rssi.innerText=d.r;
heap.innerText=d.h;
cpu.innerText=d.c;
uptime.innerText=d.u;
});
}

function osc(){
fetch('/r').then(r=>r.text()).then(v=>{
let val=parseInt(v);
rssi.innerText=val;

buf.push(val);
if(buf.length>100)buf.shift();

let sum=0;
for(let i=0;i<buf.length;i++) sum+=buf[i];
let av=Math.round(sum/buf.length);
avg.innerText=av;

classify(val);

ctx.clearRect(0,0,320,140);

// reference lines
ctx.strokeStyle="#003333";
[-40,-60,-80].forEach(l=>{
 let y=140-((l+100)/60*140);
 ctx.beginPath();
 ctx.moveTo(0,y);
 ctx.lineTo(320,y);
 ctx.stroke();
});

// labels
ctx.fillStyle="#00ffaa";
ctx.fillText("-40 dBm",5,12);
ctx.fillText("-60 dBm",5,52);
ctx.fillText("-80 dBm",5,92);

// signal
ctx.beginPath();
ctx.strokeStyle="#00ffcc";
for(let i=0;i<buf.length;i++){
 let y=140-((buf[i]+100)/60*140);
 if(i==0) ctx.moveTo(i*3.2,y);
 else ctx.lineTo(i*3.2,y);
}
ctx.stroke();
});
}

function scan(){
 fetch('/w').then(r=>r.text()).then(t=>w.innerText=t);
}

setInterval(stat,2000);
setInterval(osc,120);
stat();
</script>

</body></html>
)rawliteral";
}

void handleRoot() { server.send(200, "text/html", html()); }

void handleStatus() {
  String j = "{";
  j += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  j += "\"r\":" + String(WiFi.RSSI()) + ",";
  j += "\"h\":" + String(ESP.getFreeHeap()) + ",";
  j += "\"c\":" + String(getCpuFrequencyMhz()) + ",";
  j += "\"u\":" + String((millis() - startMillis) / 1000);
  j += "}";
  server.send(200, "application/json", j);
}

void handleRSSI() { server.send(200, "text/plain", String(WiFi.RSSI())); }

void handleScan() {
  String o = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++)
    o += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)\n";
  server.send(200, "text/plain", o);
}

void setup() {
  Serial.begin(115200);
  startMillis = millis();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.on("/", handleRoot);
  server.on("/s", handleStatus);
  server.on("/r", handleRSSI);
  server.on("/w", handleScan);

  server.begin();
}

void loop() { server.handleClient(); }
