// REM ------------------------------------------------------------------
// REM  FILE: Dot_Matrix_Web_ISO.ino
// REM  VERSION: Superchook Web Matrix Controller
// REM  PLATFORM: ESP8266 + FC16 4x8x8 LED Matrix
// REM  AUTHOR: Reaper Harvester / Wills / master Damian Williamson Grad.
// REM  COLLABORATOR: Microsoft Copilot (AI Companion)
// REM  DESCRIPTION:
// REM    - Web-controlled scrolling matrix display
// REM    - Wi-Fi STA/AP fallback with EEPROM credential storage
// REM    - Live client tracking via JavaScript ping
// REM    - Status pixel logic (bottom-right)
// REM    - Superchook animation ritual (egg → crack → flap → boom)
// REM    - Mobile UI scaling (400%) with desktop fidelity
// REM    - Honest, mythic infrastructure for public-facing control
// REM ------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Display config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN D8

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MD_MAX72XX matrix = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
ESP8266WebServer server(80);

// EEPROM config
#define EEPROM_SIZE 512
String ssid = "";
String password = "";
String scrollMessage = "Welcome to the Workshop!";
textPosition_t textPosition = PA_CENTER;
textEffect_t scrollEffect = PA_SCROLL_LEFT;
uint16_t scrollSpeed = 100;

// Status pixel tracking
unsigned long lastClientPing = 0;

void handlePing() {
  lastClientPing = millis();
  server.send(200, "text/plain", "pong");
}

void writeEEPROMString(int addr, String data) {
  for (int i = 0; i < data.length(); ++i) {
    EEPROM.write(addr + i, data[i]);
  }
  EEPROM.write(addr + data.length(), 0);  // Null terminator
  EEPROM.commit();
}

String readEEPROMString(int addr) {
  char data[100];
  int len = 0;
  while (len < 100) {
    char c = EEPROM.read(addr + len);
    if (c == 0) break;
    data[len++] = c;
  }
  data[len] = 0;
  return String(data);
}

void setupWiFi() {
  EEPROM.begin(EEPROM_SIZE);
  ssid = readEEPROMString(0);
  password = readEEPROMString(100);

  Serial.println("Connecting to Wi-Fi...");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);

  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi failed. Starting AP mode.");
    WiFi.softAP("MatrixDisplay", "milopower");
  } else {
    Serial.println("\nConnected to Wi-Fi!");
    Serial.println("IP Address: " + WiFi.localIP().toString());
  }
}

void handleRoot() {
  lastClientPing = millis();

  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
    <style>
      body {
        font-family: sans-serif;
        padding: 20px;
      }
      input, select, button {
        font-size: 1em;
        margin-bottom: 10px;
      }
      @media (max-width: 1024px) {
        body {
          transform: scale(4);
          transform-origin: top left;
        }
      }
    </style>
    <script>
      setInterval(() => {
        fetch("/ping")
          .then(res => {
            if (!res.ok) throw new Error("Ping failed");
            console.log("Client ping sent");
          })
          .catch(err => console.error(err));
      }, 3000); // Ping every 4 seconds
    </script>
    </head>
    <body>
      <h2>Matrix Display Control</h2>
      <label>Message:</label><br>
      <input type="text" id="msg"><br><br>

      <label>Alignment:</label>
      <select id="align">
        <option value="PA_LEFT">Left</option>
        <option value="PA_CENTER">Center</option>
        <option value="PA_RIGHT">Right</option>
      </select><br><br>

      <label>Effect:</label>
      <select id="effect">
        <option value="PA_SCROLL_LEFT">Scroll Left</option>
        <option value="PA_SCROLL_RIGHT">Scroll Right</option>
        <option value="PA_FADE">Fade</option>
        <option value="PA_WIPE">Wipe</option>
        <option value="PA_RANDOM">Random</option>
      </select><br><br>

      <label>Speed:</label>
      <select id="speed">
        <option value="1">Superfast (1ms)</option>
        <option value="20">Fastest (20ms)</option>
        <option value="25">Faster (25ms)</option>
        <option value="50">Fast (50ms)</option>
        <option value="75">Default (75ms)</option>
        <option value="100">Slow (100ms)</option>
        <option value="150">Slower (150ms)</option>
        <option value="200">Deliberate (200ms)</option>
        <option value="400">Crawl (400ms)</option>
        <option value="1000">Snail (1000ms)</option>
      </select><br><br>

      <button onclick="sendUpdate()">Update</button><br><br>
      <button onclick="playSuperchook()">Play Superchook</button><br><br>
      <a href="/wifi">Configure Wi-Fi</a><br><br>
      <button onclick="restoreDefaults()">Restore Default</button>

      <script>
        window.onload = function() {
          fetch("/current")
            .then(res => res.text())
            .then(data => {
              const params = new URLSearchParams(data);
              document.getElementById("msg").value = params.get("msg");
              document.getElementById("align").value = params.get("align");
              document.getElementById("effect").value = params.get("effect");
              document.getElementById("speed").value = params.get("speed");
            });
        };

        function sendUpdate() {
          const msg = document.getElementById('msg').value;
          const align = document.getElementById('align').value;
          const effect = document.getElementById('effect').value;
          const speed = document.getElementById('speed').value;
          fetch(`/update?msg=${encodeURIComponent(msg)}&align=${align}&effect=${effect}&speed=${speed}`);
        }

        function restoreDefaults() {
          fetch("/default").then(() => location.reload());
        }

        function playSuperchook() {
          fetch("/superchook").then(() => location.href = "/");
        }
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.hasArg("msg")) {
    scrollMessage = server.arg("msg");
    writeEEPROMString(200, scrollMessage);
  }

  if (server.hasArg("align")) {
    String a = server.arg("align");
    if (a == "PA_LEFT") textPosition = PA_LEFT;
    else if (a == "PA_CENTER") textPosition = PA_CENTER;
    else if (a == "PA_RIGHT") textPosition = PA_RIGHT;
  }

  if (server.hasArg("effect")) {
    String e = server.arg("effect");
    if (e == "PA_SCROLL_LEFT") scrollEffect = PA_SCROLL_LEFT;
    else if (e == "PA_SCROLL_RIGHT") scrollEffect = PA_SCROLL_RIGHT;
    else if (e == "PA_FADE") scrollEffect = PA_FADE;
    else if (e == "PA_WIPE") scrollEffect = PA_WIPE;
    else if (e == "PA_RANDOM") scrollEffect = PA_RANDOM;
  }

  if (server.hasArg("speed")) {
    scrollSpeed = server.arg("speed").toInt(); // ✅ direct conversion
  }


  myDisplay.displayClear();
  myDisplay.displayScroll(scrollMessage.c_str(), textPosition, scrollEffect, scrollSpeed);
  Serial.println("Updated message: " + scrollMessage);
  Serial.print("Configuration: ");
  Serial.println((textPosition == PA_LEFT) ? "PA_LEFT" :
                 (textPosition == PA_CENTER) ? "PA_CENTER" : "PA_RIGHT");
  Serial.print("Effect       : ");
  Serial.println((scrollEffect == PA_SCROLL_LEFT) ? "PA_SCROLL_LEFT" :
                 (scrollEffect == PA_SCROLL_RIGHT) ? "PA_SCROLL_RIGHT" :
                 (scrollEffect == PA_FADE) ? "PA_FADE" :
                 (scrollEffect == PA_WIPE) ? "PA_WIPE" : "PA_RANDOM");
  Serial.print("Speed: ");
  Serial.println(scrollSpeed);
  Serial.println(getSpeedLabel(scrollSpeed));
  server.send(200, "text/plain", "OK");
}

void handleCurrent() {
  String response = "msg=" + scrollMessage;
  response += "&align=" + String((textPosition == PA_LEFT) ? "PA_LEFT" :
                                 (textPosition == PA_CENTER) ? "PA_CENTER" : "PA_RIGHT");
  response += "&effect=" + String((scrollEffect == PA_SCROLL_LEFT) ? "PA_SCROLL_LEFT" :
                                  (scrollEffect == PA_SCROLL_RIGHT) ? "PA_SCROLL_RIGHT" :
                                  (scrollEffect == PA_FADE) ? "PA_FADE" :
                                  (scrollEffect == PA_WIPE) ? "PA_WIPE" : "PA_RANDOM");
  response += "&speed=" + String(scrollSpeed);  // Direct ms value
  server.send(200, "text/plain", response);
}

String getSpeedLabel(uint16_t speed) {
  if (speed == 1) return "Superfast (1ms)";
  if (speed == 20) return "Fastest (20ms)";
  if (speed == 25) return "Faster (25ms)";
  if (speed == 50) return "Fast (50ms)";
  if (speed == 75) return "Default (75ms)";
  if (speed == 100) return "Slow (100ms)";
  if (speed == 150) return "Slower (150ms)";
  if (speed == 200) return "Deliberate (200ms)";
  if (speed == 400) return "Crawl (400ms)";
  if (speed == 1000) return "Snail (1000ms)";
  return "Unknown";
}

void handleDefault() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();

  scrollMessage = "Welcome to the Workshop!";
  textPosition = PA_CENTER;
  scrollEffect = PA_SCROLL_LEFT;

  myDisplay.displayClear();
  myDisplay.displayScroll(scrollMessage.c_str(), textPosition, scrollEffect, scrollSpeed);

  Serial.println("EEPROM wiped. Defaults restored.");
  server.send(200, "text/plain", "Defaults restored.");
}

void handleWiFiPage() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
    <style>
      body {
        font-family: sans-serif;
        padding: 20px;
      }
      input, select, button {
        font-size: 1em;
        margin-bottom: 10px;
      }
      @media (max-width: 1024px) {
        body {
          transform: scale(4);
          transform-origin: top left;
        }
      }
    </style>
    <script>
      setInterval(() => {
        fetch("/ping")
          .then(res => {
            if (!res.ok) throw new Error("Ping failed");
            console.log("Client ping sent");
          })
          .catch(err => console.error(err));
      }, 3000); // Ping every 4 seconds
    </script>
    </head>
    <body>
      <h2>Wi-Fi Configuration</h2>
      <form action="/setwifi">
        <label>SSID:</label><br>
        <input type="text" name="ssid"><br>
        <label>Password:</label><br>
        <input type="text" name="pass"><br><br>
        <input type="submit" value="Save and Reboot">
      </form>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleSetWiFi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    ssid = server.arg("ssid");
    password = server.arg("pass");
    writeEEPROMString(0, ssid);
    writeEEPROMString(100, password);
    server.send(200, "text/plain", "Wi-Fi credentials saved. Rebooting...");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing SSID or password.");
  }
}

void updateStatusPixel() {
  bool active = (millis() - lastClientPing < 5000);
  matrix.setPoint(7, 0, active);  // Bottom-right pixel
}

const uint8_t PROGMEM superchookFrames[][8] = {
  { B00011000, B00111100, B01111110, B11011011, B11111111, B01111110, B00111100, B00011000 }, // Egg
  { B00011000, B00111100, B01111110, B11011011, B11111111, B01111110, B00111100, B01011010 }, // Crack
  { B01011010, B10100101, B01011010, B10100101, B01011010, B10100101, B01011010, B10100101 }, // Flap
  { B11111111, B00000000, B11111111, B00000000, B11111111, B00000000, B11111111, B00000000 }  // Boom
};

void playSuperchookAnimation() {
  for (int frame = 0; frame < 4; frame++) {
    Serial.println("Playing Superchook frame " + String(frame));
    for (int device = 0; device < MAX_DEVICES; device++) {
      for (int row = 0; row < 8; row++) {
        matrix.setRow(device, row, pgm_read_byte(&superchookFrames[frame][row]));
      }
    }
    delay(500);
  }
  matrix.clear();
}

void setup() {
  Serial.begin(115200);
  setupWiFi();

  myDisplay.begin();
  matrix.begin();  // Needed for pixel control
  myDisplay.setIntensity(5);
  myDisplay.displayClear();
  matrix.clear();

  scrollMessage = readEEPROMString(200);
  Serial.println("Raw EEPROM message: [" + scrollMessage + "]");

  bool corrupt = false;
  for (int i = 0; i < scrollMessage.length(); i++) {
    if ((uint8_t)scrollMessage[i] == 0xFF) {
      corrupt = true;
      break;
    }
  }
  if (scrollMessage.length() == 0 || corrupt) {
    scrollMessage = "Welcome to the Workshop!";
    Serial.println("EEPROM message was corrupt. Using fallback.");
  }

  myDisplay.setTextAlignment(textPosition);
  myDisplay.setSpeed(scrollSpeed);
  myDisplay.setPause(0);
  myDisplay.displayScroll(scrollMessage.c_str(), textPosition, scrollEffect, scrollSpeed);

  server.on("/", handleRoot);
  server.on("/ping", handlePing);
  server.on("/update", handleUpdate);
  server.on("/current", handleCurrent);
  server.on("/default", handleDefault);
  server.on("/wifi", handleWiFiPage);
  server.on("/setwifi", handleSetWiFi);
  server.on("/superchook", []() {
  playSuperchookAnimation();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
});

  server.begin();
}

void loop() {
  server.handleClient();
  updateStatusPixel();
  if (scrollMessage.indexOf("Superchooks") >= 0) {
    playSuperchookAnimation();
    writeEEPROMString(200, scrollMessage);
    myDisplay.displayReset();
  }

  if (myDisplay.displayAnimate()) {
    myDisplay.displayScroll(scrollMessage.c_str(), textPosition, scrollEffect, scrollSpeed);
    myDisplay.displayReset();
  }
}
