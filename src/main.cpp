#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/////// MARK: - Setup
// Constants
#define LED_DATA_LENGTH 5

// LED strip pins
#define LED_R_PIN D6
#define LED_G_PIN D7
#define LED_B_PIN D2
#define LED_CW_PIN D1
#define LED_CC_PIN D8

// WiFi credentials
const char *ssid = "Slowbro";
const char *password = "v=08zSwim26x";
const char *hostname = "controller";

// Set led status (R, G, B, CW, CC)
int ledData[LED_DATA_LENGTH] = {0, 0, 0, 0, 0};

// Define web server
ESP8266WebServer server(80);

// Static IP address configuration
IPAddress staticIP(192, 168, 0, 125);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

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

/// @brief Sends back usage at root
void handleIndex()
{
  String description = "To set the LED strip color, send a HTTP GET request to this URL:\r\n/color?r=[0-255]&g=[0-255]&b=[0-255]&cw=[0-255]&cc=[0-255]\r\n";
  server.send(200, "plain/text", description);
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(74880);

  // Setup WiFi server
  WiFi.config(staticIP, gateway, subnet);
  WiFi.hostname("Controller");
  WiFi.setAutoConnect(true);
  WiFi.persistent(true);

  // Connect to WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi.");
  Serial.println(WiFi.localIP());

  // Set routes
  server.on("/", HTTP_GET, handleIndex);
  server.on("/color", HTTP_GET, handleColorRequest);
  server.on("/status", HTTP_GET, getStatus);

  server.begin();
  Serial.println("Server started");

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