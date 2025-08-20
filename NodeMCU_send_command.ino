#include <SoftwareSerial.h>

SoftwareSerial espSerial(D7, D6);  // D7 = RX (8051 TX), D6 = TX (8051 RX)

void setup() {
  Serial.begin(9600);        // USB Serial to GUI
  espSerial.begin(9600);     // Serial to 8051
  Serial.println("NodeMCU Ready.");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    flushESPSerial();             // Clear stale data
    espSerial.write(cmd);         // Send command to 8051

    if (cmd == 'R') handleDHT11();                 // Temperature & Humidity
    else if (cmd == 'M') handleAuto();             // Auto Mode
    else if (cmd >= '1' && cmd <= '6') handleActuator(); // Actuator Commands
    else handleSingleSensor();                    // All other sensors
  }
}

void flushESPSerial() {
  while (espSerial.available()) espSerial.read();  // Flush old data
}

void handleDHT11() {
  String temp = "", hum = "";
  unsigned long start = millis();

  while (millis() - start < 5000) {
    if (espSerial.available()) {
      String data = espSerial.readStringUntil('\n');
      data.trim();

      if (data.startsWith("Hum:")) hum = data;
      else if (data.startsWith("Temp:")) temp = data;

      if (!hum.isEmpty() && !temp.isEmpty()) break;
    }
  }

  if (!hum.isEmpty()) Serial.println(hum);
  if (!temp.isEmpty()) Serial.println(temp);
  else if (hum.isEmpty() || temp.isEmpty()) Serial.println("DHT11 Read Timeout");

  Serial.println("DONE");
}

void handleSingleSensor() {
  unsigned long start = millis();
  String response = "";

  while (millis() - start < 3000) {
    if (espSerial.available()) {
      response = espSerial.readStringUntil('\n');
      response.trim();
      if (response.length()) {
        Serial.println(response);
        break;
      }
    }
  }
  Serial.println("DONE");
}

void handleActuator() {
  unsigned long start = millis();
  String res = "";

  while (millis() - start < 2000) {
    if (espSerial.available()) {
      res = espSerial.readStringUntil('\n');
      res.trim();
      if (res.length()) {
        Serial.println(res);
        break;
      }
    }
  }
  Serial.println("DONE");
}

void handleAuto() {
  unsigned long start = millis();
  String line;

  while (millis() - start < 5000) {
    if (espSerial.available()) {
      line = espSerial.readStringUntil('\n');
      line.trim();
      if (line.length()) {
        Serial.println(line);
        if (line == "AUTO MODE DONE") break;
      }
    }
  }
  Serial.println("DONE");
}
