#include <ESP32Servo.h>
#include <NewPing.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "ChangeMySSID";
const char* password = "ChangeMyPassword";
#define SERVO_PIN_Exit 32
#define SERVO_PIN_Entry 26  
#define TRIG_PIN_EXIT 14
#define ECHO_PIN_EXIT 27
#define TRIG_PIN_ENTRY 18
#define ECHO_PIN_ENTRY 19
#define MAX_DISTANCE 5.00
#define MAX_PARKING_SPOTS 4 
#define Led_pin 15  // LED pin for indicator

NewPing sonarExit(TRIG_PIN_EXIT, ECHO_PIN_EXIT, MAX_DISTANCE);
NewPing sonarEntry(TRIG_PIN_ENTRY, ECHO_PIN_ENTRY, MAX_DISTANCE);
Servo myServoExit;
Servo myServoEntry;

int carCount = 0;
int carsEntered = 0; 
int carsExited = 0; 
bool isParkingFull = false;
bool isParkingLocked = false; 

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing...");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  myServoExit.attach(SERVO_PIN_Exit);
  myServoExit.write(120);
  delay(1000);
  myServoEntry.attach(SERVO_PIN_Entry);
  myServoEntry.write(120);
  delay(1000);

  pinMode(Led_pin, OUTPUT);
  digitalWrite(Led_pin, LOW);  

  // Web Interface for Car Parking System
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; margin: 0; padding: 0; text-align: center;}"; 
    html += "h1 { color: #4CAF50; margin-top: 50px; font-size: 36px; }";
    html += "p { font-size: 18px; color: #333; margin: 20px 0; }";
    html += "#carCount { font-size: 32px; font-weight: bold; color: #FF5733; }";
    html += ".container { max-width: 800px; margin: 0 auto; padding: 20px; background-color: #fff; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }";
    html += ".button { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 18px; transition: background-color 0.3s ease;} ";
    html += ".button:hover { background-color: #45a049; }";
    html += ".footer { margin-top: 50px; font-size: 14px; color: #777; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>Car Parking System</h1>";
    html += "<p>Current Cars in Parking: <span id='carCount'>" + String(carCount) + "</span></p>";

    if (isParkingFull) {
      html += "<p style='color: red; font-weight: bold;'>Parking is full! No more cars can enter.</p>";
    }

    if (isParkingLocked) {
      html += "<p style='color: red; font-weight: bold;'>Parking is locked! No cars can enter.</p>";
    } else {
      html += "<p style='color: green; font-weight: bold;'>Parking is open! Cars can enter.</p>";
    }

    // Button to toggle parking lock status
    if (isParkingLocked) {
      html += "<button class='button' onclick='window.location.href = \"/unlockParking\";'>Unlock Parking</button>";
    } else {
      html += "<button class='button' onclick='window.location.href = \"/lockParking\";'>Lock Parking</button>";
    }

    html += "<button class='button' onclick='window.location.reload();'>Refresh</button>";
    html += "<script>";
    html += "setInterval(function(){";
    html += "fetch('/getCarCount').then(response => response.text()).then(data => {";
    html += "document.getElementById('carCount').innerHTML = data;"; 
    html += "});";
    html += "}, 1000);"; 
    html += "</script>";
    html += "</div>";
    html += "<div class='footer'>";
    html += "<p>Parkmate powered by ESP32</p>";
    html += "</div>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/getCarCount", HTTP_GET, [](AsyncWebServerRequest *request){
    String count = String(carCount);
    request->send(200, "text/plain", count);
  });

  // Lock parking
  server.on("/lockParking", HTTP_GET, [](AsyncWebServerRequest *request){
    isParkingLocked = true;  // Lock the parking
    request->send(200, "text/plain", "Parking is locked");
  });

  // Unlock parking
  server.on("/unlockParking", HTTP_GET, [](AsyncWebServerRequest *request){
    isParkingLocked = false;  // Unlock the parking
    request->send(200, "text/plain", "Parking is unlocked");
  });

  server.begin();
}

void loop() {
  digitalWrite(TRIG_PIN_ENTRY, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_ENTRY, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_ENTRY, LOW);

  float duration = pulseIn(ECHO_PIN_ENTRY, HIGH);
  float distance_entry = (duration * 0.0343) / 2;

  digitalWrite(TRIG_PIN_EXIT, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_EXIT, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_EXIT, LOW);

  float duration_2 = pulseIn(ECHO_PIN_EXIT, HIGH);
  float distance_exit = (duration_2 * 0.0343) / 2;

  // Car entering logic
  if (!isParkingLocked && distance_entry > 0 && distance_entry <= MAX_DISTANCE) { // Allow entry only if parking is unlocked
    if (carCount < MAX_PARKING_SPOTS) {  
      myServoEntry.write(30);  
      delay(1000);
      digitalWrite(Led_pin, HIGH); 
      carCount++;
      carsEntered++;
      if (carCount == MAX_PARKING_SPOTS) {
        isParkingFull = true;  
      }
      delay(3000);  
      myServoEntry.write(120);  
      digitalWrite(Led_pin, LOW); 
      delay(1000);
    } else {
      isParkingFull = true;
    }
  }

  else if (distance_exit > 0 && distance_exit <= MAX_DISTANCE) {
    myServoExit.write(30);  
    carCount--;
    carsExited++;
    if (carCount < MAX_PARKING_SPOTS) {
      isParkingFull = false; 
    }
    delay(3000);  
    myServoExit.write(120);  
    delay(1000);
  }

  delay(1000);  
}
