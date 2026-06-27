/*
  ESP32 - 4 Channel Relay Web Control (Access Point Mode + Memory)
  -------------------------------------------------------------------
  The ESP32 creates its OWN WiFi network. No home WiFi or internet needed.
  Relay states are saved to flash memory, so they survive WiFi disconnects,
  power loss, and reboots — relays come back exactly as they were left.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ---------------- USER SETTINGS ----------------
// This is the WiFi network name/password the ESP32 will CREATE.
const char* AP_SSID     = "ESP32_Relay_Control";
const char* AP_PASSWORD = "12345678";   // must be at least 8 characters

#define RELAY1_PIN 16
#define RELAY2_PIN 17
#define RELAY3_PIN 18
#define RELAY4_PIN 19

// Most cheap relay modules turn the relay ON when the GPIO is LOW.
// If your relays behave backwards, flip this.
bool RELAY_ACTIVE_LOW = true;
// -------------------------------------------------

WebServer server(80);
Preferences prefs;

const int relayPins[4] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};
bool relayState[4] = {false, false, false, false}; // false = OFF, true = ON
const char* relayKeys[4] = {"r0", "r1", "r2", "r3"}; // keys used to save each relay's state

// Turn a relay on/off, accounting for active-low modules
void setRelay(int index, bool on) {
  relayState[index] = on;
  bool pinLevel = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(relayPins[index], pinLevel ? HIGH : LOW);
  prefs.putBool(relayKeys[index], on); // save so it survives power loss / reboot
}

String buildPage() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 Relay Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial, sans-serif; text-align:center; background:#1e1e2f; color:#fff; margin:0; padding:20px;}";
  html += "h1{margin-bottom:30px;}";
  html += ".card{background:#2b2b40; border-radius:12px; padding:20px; margin:15px auto; max-width:320px; box-shadow:0 4px 10px rgba(0,0,0,0.3);}";
  html += ".relay-row{display:flex; justify-content:space-between; align-items:center;}";
  html += ".btn{padding:10px 20px; border:none; border-radius:8px; font-size:16px; cursor:pointer; color:#fff;}";
  html += ".on{background:#28a745;}";
  html += ".off{background:#dc3545;}";
  html += ".status{font-weight:bold; font-size:14px; padding:4px 10px; border-radius:6px;}";
  html += ".status-on{background:#28a745;}";
  html += ".status-off{background:#555;}";
  html += "</style></head><body>";
  html += "<h1>ESP32 4-Channel Relay Control</h1>";

  for (int i = 0; i < 4; i++) {
    html += "<div class='card'><div class='relay-row'>";
    html += "<span>Relay " + String(i + 1) + " ";
    html += "<span class='status " + String(relayState[i] ? "status-on" : "status-off") + "'>";
    html += relayState[i] ? "ON" : "OFF";
    html += "</span></span>";
    html += "<a href='/relay?ch=" + String(i) + "&state=" + String(relayState[i] ? 0 : 1) + "'>";
    html += "<button class='btn " + String(relayState[i] ? "off" : "on") + "'>";
    html += relayState[i] ? "Turn OFF" : "Turn ON";
    html += "</button></a>";
    html += "</div></div>";
  }

  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleRelay() {
  if (server.hasArg("ch") && server.hasArg("state")) {
    int ch = server.arg("ch").toInt();
    int state = server.arg("state").toInt();
    if (ch >= 0 && ch < 4) {
      setRelay(ch, state == 1);
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  prefs.begin("relays", false); // open (or create) the "relays" storage namespace

  // Initialize relay pins as outputs, then restore each one's last saved state
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    bool savedState = prefs.getBool(relayKeys[i], false); // false = default if never saved before
    setRelay(i, savedState);
  }

  // Start WiFi in Access Point mode (ESP32 creates its own network)
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("Access Point started!");
  Serial.print("Connect your phone/PC to WiFi network: ");
  Serial.println(AP_SSID);
  Serial.print("Then open this address in your browser: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/relay", handleRelay);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
