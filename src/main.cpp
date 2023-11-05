#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Models.h>

/////// MARK: - Setup ///////
// Constants
#define LED_DATA_LENGTH 5
#define AP_MODE 10001
#define CLIENT_MODE 10002
#define WIFI_RETRY_BEFORE_AP 10
#define HOSTNAME_SUFFIX "testDevice_"
// #define DEBUG_MODE

// LED strip pins
#define LED_R_PIN D6
#define LED_G_PIN D7
#define LED_B_PIN D2
#define LED_CW_PIN D1
#define LED_CC_PIN D8

// WiFi Mode
int wifiMode = AP_MODE;

// WiFi credentials to connect to an AP
String ssid = "";
String password = "";

// Set led status (R, B, G, CW, CC)
int ledData[LED_DATA_LENGTH] = {0, 0, 0, 0, 0};

// Define web server
ESP8266WebServer server(80);

// Static IP address configuration
IPAddress staticIP(192, 168, 10, 125);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

/// @brief Deletes settings file forcing a reset on credentials
void deleteSettings() { LittleFS.remove("/settings.txt"); }

/// @brief Reads settings from file to memory
/// @return -1 if error and 0 if success
int readSettings() {
  File file = LittleFS.open("/settings.txt", "r");
  if (!file) {
#ifdef DEBUG_MODE
    Serial.println("Failed to open file for reading");
    Serial.println("Defaulting to AP_MODE");
#endif
    return -1;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.startsWith("SSID=")) {
      ssid = line.substring(sizeof("SSID=") - 1);
#ifdef DEBUG_MODE
      Serial.println("SSID= " + ssid);
#endif
    }

    if (line.startsWith("Password=")) {
      password = line.substring(sizeof("Password=") - 1);
#ifdef DEBUG_MODE
      Serial.println("Password= " + password);
#endif
    }
  }

  if (ssid == "" || password == "") {
    return -1;
  }

  file.close();

  return 0;
}

/// @brief Sets up pins
void setupPins() {
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(LED_CW_PIN, OUTPUT);
  pinMode(LED_CC_PIN, OUTPUT);

  analogWrite(LED_R_PIN, 0);
  analogWrite(LED_G_PIN, 0);
  analogWrite(LED_B_PIN, 0);
  analogWrite(LED_CW_PIN, 0);
  analogWrite(LED_CC_PIN, 0);
}

/// @brief Creates unique hostname/ssid out of mac address
/// @return hostname string
String setupHostname() {
  uint8_t mac[6];
  WiFi.macAddress(mac);

  String macAddress = HOSTNAME_SUFFIX;
  for (int i = 3; i < 6; i++) {
    macAddress += String(mac[i], 16);
  }
  return macAddress;
}

/////// MARK: - Request Controllers ///////
/// @brief Handles RGB CW CC values
void handleColorRequest() {
  int parametersCount = server.args();

  if (parametersCount == 0) {
     String htmlContent = "<!DOCTYPE html><html><head>";
      htmlContent +=
          "<meta name='viewport' content='width=device-width, "
          "initial-scale=1'>";
      htmlContent += "<style>";
      htmlContent +=
          "body { font-family: Arial, sans-serif; text-align: center; }";
      htmlContent += "h2 { color: #333; }";
      htmlContent +=
          ".container { margin: 0 auto; max-width: 300px; padding: 20px; "
          "border: "
          "1px solid #ccc;";
      htmlContent += "border-radius: 5px; background-color: #f9f9f9; }";
      htmlContent +=
          ".input-field { margin: 10px 0; padding: 5px; width: 100%; border: "
          "1px "
          "solid #ccc;";
      htmlContent += "border-radius: 3px; }";
      htmlContent +=
          "#send-button { background-color: #0074cc; color: #fff; padding: "
          "10px "
          "20px; border: none;";
      htmlContent += "border-radius: 3px; cursor: pointer; }";
      htmlContent +=
          ".button-group { display: flex; flex-direction: column; align-items: "
          "center; }";
      htmlContent += ".button-group button { margin: 5px 0; width: 100%; }";
      htmlContent += "@media (max-width: 500px) {";
      htmlContent += ".container { max-width: 100%; }";
      htmlContent += ".input-field, .button-group button { width: 100%; }";
      htmlContent += ".button-group button { height: 48px; }";
      htmlContent += "}";
      htmlContent += "</style>";
      htmlContent += "<script>";
      htmlContent += "function validateInput(inputField) {";
      htmlContent += "   var value = parseInt(inputField.value, 10);";
      htmlContent += "   if (isNaN(value) || value < 0 || value > 255) {";
      htmlContent +=
          "       alert('Please enter a valid value between 0 and 255.');";
      htmlContent += "       inputField.value = '';";
      htmlContent += "   }";
      htmlContent += "}";
      htmlContent += "function setToZeroAndSubmit() {";
      htmlContent += "   document.getElementById('red').value = '0';";
      htmlContent += "   document.getElementById('green').value = '0';";
      htmlContent += "   document.getElementById('blue').value = '0';";
      htmlContent += "   document.getElementById('cold-white').value = '0';";
      htmlContent +=
          "   document.getElementById('color-changing').value = '0';";
      htmlContent += "}";
      htmlContent += "function setSpecificAndSubmit() {";
      htmlContent += "   document.getElementById('red').value = '10';";
      htmlContent += "   document.getElementById('green').value = '0';";
      htmlContent += "   document.getElementById('blue').value = '0';";
      htmlContent += "   document.getElementById('cold-white').value = '255';";
      htmlContent +=
          "   document.getElementById('color-changing').value = '40';";
      htmlContent += "}";
      htmlContent += "</script>";
      htmlContent += "</head><body>";
      htmlContent += "<h2>LED Control</h2>";
      htmlContent +=
          "<div class='container'><form action='/color' method='get'>";
      htmlContent += "<label for='red'>R:</label>";
      htmlContent +=
          "<input type='text' id='red' name='r' class='input-field' value='" +
          String(ledData[0]) + "' onblur='validateInput(this)'><br>";

      htmlContent += "<label for='green'>G:</label>";
      htmlContent +=
          "<input type='text' id='green' name='g' class='input-field' value='" +
          String(ledData[1]) + "' onblur='validateInput(this)'><br>";

      htmlContent += "<label for='blue'>B:</label>";
      htmlContent +=
          "<input type='text' id='blue' name='b' class='input-field' value='" +
          String(ledData[2]) + "' onblur='validateInput(this)'><br>";

      htmlContent += "<label for='cold-white'>CW:</label>";
      htmlContent +=
          "<input type='text' id='cold-white' name='cw' class='input-field' "
          "value='" +
          String(ledData[3]) + "' onblur='validateInput(this)'><br>";

      htmlContent += "<label for='color-changing'>CC:</label>";
      htmlContent +=
          "<input type='text' id='color-changing' name='cc' "
          "class='input-field' "
          "value='" +
          String(ledData[4]) + "' onblur='validateInput(this)'><br>";

      htmlContent += "<div class='button-group'>";
      htmlContent +=
          "<button type='submit' id='send-button' "
          "onclick='setToZeroAndSubmit()'>Turn off</button>";
      htmlContent +=
          "<button type='submit' id='send-button' "
          "onclick='setSpecificAndSubmit()'>Set to default</button>";
      htmlContent +=
          "<button type='submit' id='send-button'>Send Request</button>";
      htmlContent += "</div>";
      htmlContent += "</form></div></body></html>";

      server.send(200, "text/html", htmlContent);
      return;
  }

  for (int i = 0; i < parametersCount; i++) {
    // Validate argument string
    String argName = server.argName(i);
    String value = server.arg(i);

    // Ignore unsopported params
    if (value == NULL) {
      continue;
    }

    // Set value to int and check if it is within range (0 - 255)
    // If out of range ignore value
    int numericValue = value.toInt();

    if (numericValue < 0 || numericValue > 255) {
      continue;
    }

    ledData[i] = numericValue;

    server.send(200);
  }

  // Write led data to pins
  analogWrite(LED_R_PIN, ledData[0]);
  analogWrite(LED_B_PIN, ledData[1]);
  analogWrite(LED_G_PIN, ledData[2]);
  analogWrite(LED_CW_PIN, ledData[3]);
  analogWrite(LED_CC_PIN, ledData[4]);

  server.send(200, "text/html", htmlContent);
}

/// @brief Handels factoryReset request
void handleDelete() {
  deleteSettings();
  server.send(200);
  ESP.restart();
}

/// @brief Handles connection request to a WiFi AP
void connectToWifi() {
  if (wifiMode == AP_MODE) {
    String _ssid = server.arg("ssid");
    String _password = server.arg("password");

    File file = LittleFS.open("/settings.txt", "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      server.send(500, "text/plain", "Error while saving the credentials");
      return;
    }

    file.print("SSID=");
    file.println(_ssid);
    file.print("Password=");
    file.println(_password);
    file.close();

    String htmlPage = R"html(
      <!DOCTYPE html>
      <html>
      <head>
        <style>
          body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f2f2f2;
            margin: 0;
            padding: 0;
          }
          .container {
            background-color: #fff;
            border-radius: 10px;
            box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2);
            padding: 20px;
            margin: 10% auto;
            width: 80%;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h2>Settings Saved</h2>
          <p>Now rebooting the device. You can now connect to your WiFi network.</p>
        </div>
      </body>
      </html>
      )html";
    server.send(200, "text/html", htmlPage);
    delay(3000);
    ESP.restart();
  } else {  // TODO: Allow user to be able to change the credentials
    server.send(418, "text/plain", "I'm a teapot and can't connect you to");
    return;
  }
}

/// @brief Sends back usage at root
void handleIndex() {
  if (wifiMode == CLIENT_MODE) {
    String htmlContent = "<!DOCTYPE html><html><head>";
    // Add the viewport meta tag for mobile responsiveness
    htmlContent +=
        "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    htmlContent += "<style>";
    // Your CSS styles go here

    // CSS styles for common elements
    htmlContent +=
        "body { font-family: Arial, sans-serif; text-align: center; }";
    htmlContent +=
        ".container { max-width: 300px; margin: 0 auto; padding: 20px; border: "
        "1px solid #ccc; border-radius: 5px; background-color: #f9f9f9; }";
    htmlContent +=
        ".button-group { display: flex; flex-direction: column; padding: 10px "
        "20px; align-items: center; }";
    htmlContent +=
        ".button { background-color: #0074cc; color: #fff; padding: 10px 20px; "
        "border: none; border-radius: 3px; cursor: pointer; margin: 5px 0; "
        "width: 100%; text-decoration: none; font-size: 16px; }";

    // Media query for smaller screens
    htmlContent += "@media (max-width: 500px) {";
    htmlContent += "    .container { max-width: 100%; }";
    htmlContent += "    .button-group button { height: 48px; }";
    htmlContent += "}";

    // End of CSS styles

    htmlContent += "</style>";
    htmlContent += "</head><body>";
    htmlContent += "<div class='container'>";
    htmlContent += "<h2>Welcome to Cem's LED Controller WebUI</h2>";
    htmlContent += "<div class='button-group'>";
    htmlContent += "<a href='/color' class='button'>Color Control</a>";
    htmlContent += "<a href='/factoryReset' class='button'>Factory Reset</a>";
    htmlContent += "</div>";
    htmlContent += "</div></body></html>";

    server.send(200, "text/html", htmlContent);
  } else {
    String html = R"HTML(
    <html>
      <head>
        <style>
          body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f0f0f0;
          }

          h2 {
            color: #333;
          }

          form {
            background-color: #fff;
            padding: 20px;
            border-radius: 5px;
            display: inline-block;
          }

          input[type='text'], input[type='password'] {
            width: 100%;
            padding: 10px;
            margin: 5px 0;
            border: 1px solid #ccc;
            border-radius: 4px;
          }

          input[type='submit'] {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
          }
        </style>
      </head>
      <body>
        <h2>Login Form</h2>
        <form action='/connect' method='POST'>
          SSID: <input type='text' name='ssid'><br>
          Password: <input type='password' name='password'><br>
          <input type='submit' value='Submit'>
        </form>
      </body>
    </html>
    )HTML";

    server.send(200, "text/html", html);
  }
}

/// @brief Sends back led status (rbg cc cw)
void getStatus() {
  /*
  char *status = (char *)malloc(sizeof(char));
  status[0] = '\0';
  */
  String status = "";
  for (int i = 0; i < LED_DATA_LENGTH; i++) {
    Color colorLiteral = static_cast<Color>(i);
    status += getColorString(colorLiteral) + ": " + String(ledData[i]) + " ";
    /*
    // Allocate size for charArray + 1 for null pointer
    char *charArray = (char *)malloc(sizeof(ledData[i]) + 1);
    charArray[0] = '\0';
    sprintf(charArray, "%d", ledData[i]);
    status = (char *)realloc(status, sizeof(charArray) * (i + 1));
    strcat(status, charArray);
    free(charArray);
    */
  }
  server.send(200, "text/plain", status);
  // free(status);
}

/////// MARK: - Lifecycle Methods ///////
void setup() {
  /////// INITIAL SETUP ///////
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(74880);

  /////// MOUNT FILESYSTEM ///////
  if (LittleFS.begin()) {
    Serial.println("File system mounting successful");
  } else {
    Serial.println("Mounting file system has failed");
    wifiMode = AP_MODE;
  }

  /////// READ SETTINGS TO MEMORY ///////
  if (readSettings() == -1) {
    Serial.println("No valid credentials found. mode is AP");
    wifiMode = AP_MODE;
  } else {
    wifiMode = CLIENT_MODE;
  }

  /////// WIFI SETUP ///////
  String hostname = setupHostname();
  Serial.println(hostname);

  // Setup WiFi server
  if (wifiMode == CLIENT_MODE) {
    WiFi.mode(WIFI_STA);
    // WiFi.config(staticIP, gateway, subnet);
    WiFi.hostname(hostname);
    WiFi.setAutoConnect(true);
    WiFi.persistent(true);

    // Connect to WiFi
    WiFi.begin(ssid, password);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if (retryCount == WIFI_RETRY_BEFORE_AP) {
        deleteSettings();
        ESP.restart();
      }
      delay(1000);
      Serial.println("Connecting to WiFi...");
      retryCount++;
    }

    Serial.println("Connected to WiFi.");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(staticIP, gateway, subnet);
    WiFi.softAP(hostname, "");
    Serial.println(WiFi.softAPIP());
  }

  /////// SERVER SETUP ///////
  // Set routes
  server.on("/", HTTP_GET, handleIndex);
  server.on("/connect", HTTP_POST, connectToWifi);
  server.on("/color", HTTP_GET, handleColorRequest);
  server.on("/status", HTTP_GET, getStatus);
  server.on("/factoryReset", HTTP_GET, handleDelete);

  server.begin();
  Serial.println("Server started");

  /////// START OTA ///////
  ArduinoOTA.setHostname(hostname.c_str());
  Serial.println(ArduinoOTA.getHostname());
  ArduinoOTA.begin(true);
  Serial.println("ArduinoOTA has started");

  /////// OTHER SETUP ///////
  setupPins();
  pinMode(LED_BUILTIN, HIGH);
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
}