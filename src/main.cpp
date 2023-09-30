#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

/////// MARK: - Setup
// Constants
#define LED_DATA_LENGTH 5

#define AP_MODE 10001
#define CLIENT_MODE 10002

#define WIFI_RETRY_BEFORE_AP 10

#define DEBUG_MODE

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
const char *hostname = "controller";

// Wifi credentials as AP
const char *ssidAP = "testDevice_123";
const char *passwordAP = "";

// Set led status (R, G, B, CW, CC)
int ledData[LED_DATA_LENGTH] = {0, 0, 0, 0, 0};

// Define web server
ESP8266WebServer server(80);

// Static IP address configuration
IPAddress staticIP(192, 168, 0, 126);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

void deleteSettings()
{
  SPIFFS.remove("/settings.txt");
}

int readSettings()
{
  File file = SPIFFS.open("/settings.txt", "r");
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

  if(ssid == "" || password == "")
  {
    return -1;
  }

  file.close();

  return 0;
}

/// @brief Sets up pins
void setupPins()
{
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

/////// MARK: - Request Controllers

/// @brief Handles RGB CW CC values
void handleColorRequest()
{
  int parametersCount = server.args();
  for (int i = 0; i < parametersCount; i++)
  {
    // Validate argument string
    String argName = server.argName(i);
    String value = server.arg(i);

    // Ignore unsopported params
    if (value == NULL)
    {
      continue;
    }

    // Set value to int and check if it is within range (0 - 255)
    // If out of range ignore value
    int numericValue = value.toInt();

    if (numericValue < 0 || numericValue > 255)
    {
      continue;
    }

    ledData[i] = numericValue;
  }

  // Write led data to pins
  analogWrite(LED_R_PIN, ledData[0]);
  analogWrite(LED_G_PIN, ledData[1]);
  analogWrite(LED_B_PIN, ledData[2]);
  analogWrite(LED_CW_PIN, ledData[3]);
  analogWrite(LED_CC_PIN, ledData[4]);

  server.send(200);
}

void connectToWifi()
{
  if (wifiMode == AP_MODE)
  {
    String _ssid = server.arg("ssid");
    String _password = server.arg("password");

    File file = SPIFFS.open("/settings.txt", "w");
    if (!file)
    {
      Serial.println("Failed to open file for writing");
      server.send(500, "text/plain", "Error while saving the credentials");
      return;
    }

    file.print("SSID=");
    file.println(_ssid);
    file.print("Password=");
    file.println(_password);
    file.close();

    server.send(200, "text/plain", "Rebooting now.");
    ESP.restart();
  }
  else
  { // TODO: Allow user to be able to change the credentials
    server.send(418, "text/plain", "I'm a teapot and can't connect you to");
    return;
  }
}

/// @brief Sends back usage at root
void handleIndex()
{
  if (wifiMode == CLIENT_MODE)
  {
    String description = "To set the LED strip color, send a HTTP GET request to this URL:\r\n/color?r=[0-255]&g=[0-255]&b=[0-255]&cw=[0-255]&cc=[0-255]\r\n";
    server.send(200, "text/plain", description);
  }
  else
  {
    String html = "<html><body>";
    html += "<h2>Login Form</h2>";
    html += "<form action='/connect' method='POST'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Password: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Submit'>";
    html += "</form></body></html>";

    server.send(200, "text/html", html);
  }
}

/// @brief Sends back led status (rgb cc cw)
void getStatus()
{
  char *status = (char *)malloc(sizeof(char));
  status[0] = '\0';

  for (int i = 0; i < LED_DATA_LENGTH; i++)
  {
    // Allocate size for charArray + 1 for null pointer
    char *charArray = (char *)malloc(sizeof(ledData[i]) + 1);
    charArray[0] = '\0';
    sprintf(charArray, "%d", ledData[i]);
    status = (char *)realloc(status, sizeof(charArray) * (i + 1));
    strcat(status, charArray);
    free(charArray);
  }
  server.send(200, "text/plain", status);
  free(status);
}

/////// MARK: - Lifecycle Methods
void setup()
{
  /////// INITIAL SETUP ///////
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(74880);

  /////// MOUNT FILESYSTEM ///////
  if (SPIFFS.begin())
  {
    Serial.println("File system mounting successful");
  }
  else
  {
    Serial.println("Mounting file system has failed");
    wifiMode = AP_MODE;
  }

  if (readSettings() == -1)
  {
    Serial.println("No valid credentials found. mode is AP");
    wifiMode = AP_MODE;
  } else
  {
    wifiMode = CLIENT_MODE;
  }

  /////// WIFI SETUP ///////
  // Setup WiFi server
  if (wifiMode == CLIENT_MODE)
  {
    WiFi.mode(WIFI_STA);
    WiFi.config(staticIP, gateway, subnet);
    WiFi.hostname("Controller");
    WiFi.setAutoConnect(true);
    WiFi.persistent(true);

    // Connect to WiFi
    WiFi.begin(ssid, password);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      if (retryCount == WIFI_RETRY_BEFORE_AP)
      {
        deleteSettings();
        ESP.restart();
      }
      delay(1000);
      Serial.println("Connecting to WiFi...");
      retryCount++;
    }

    Serial.println("Connected to WiFi.");
    Serial.println(WiFi.localIP());
  }
  else
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAP, passwordAP);
    Serial.println(WiFi.softAPIP());
  }

  /////// SERVER SETUP ///////
  // Set routes
  server.on("/", HTTP_GET, handleIndex);
  server.on("/connect", HTTP_POST, connectToWifi);
  server.on("/color", HTTP_GET, handleColorRequest);
  server.on("/status", HTTP_GET, getStatus);

  server.begin();
  Serial.println("Server started");

  /////// OTHER SETUP ///////
  setupPins();
  pinMode(LED_BUILTIN, HIGH);
}

void loop()
{
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
}