#include <tiny-sensor-toolbox.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

const char* WIFI_SSID = "Hier WLAN-SSID eintragen";
const char* WIFI_PASSWORD = "Hier WLAN-Passwort eintragen";
const char* THING_SPEAK_API_KEY = "Hier API-Key für ThingSpeak eintragen";
ThingSpeakSender sender(WIFI_SSID, WIFI_PASSWORD, THING_SPEAK_API_KEY);

U8G2_SH1106_128X64_NONAME_F_SW_I2C display(U8G2_R0, 5, 17);

float temperature_C = 0.0;
float windSpeed_rpm = 0.0;
float pressure_hPa = 0.0;

String statusMessage = "";
unsigned long statusMessageTime_ms = 0;

DS18B20Sensor temperatureSensor(32);
SignalEdgeSensor windSensor(4, 10.0);
Adafruit_BMP280 pressureSensor(15, 13, 12, 14); // An Hardware-SPI des ESP32.


void setup() {
  pinMode(19, OUTPUT);
  digitalWrite(19, HIGH);
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  display.begin();

  // Initialisierung für Temperatursensor.
  temperatureSensor.setup();

  // Pin für Neigungssensor im Windrad.
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  windSensor.setup();

  pressureSensor.begin();
}


// Diese Funktion zeigt eine Statusmeldung im Display an.
template<typename T>
void showStatusMessage(T msg) {
  statusMessage = msg;
  statusMessageTime_ms = millis();
  redrawScreen();
}


// Liest die Messwerte für Temperatur und Windgeschwindigkeit
// von den Sensoren.
void updateMeasurements() {
  showStatusMessage("Aktualisiere Messung ...");

  // Lies Temperatur aus.
  temperatureSensor.loop();
  if (temperatureSensor.getData() > -100.0f) {
    temperature_C = temperatureSensor.getData();
  }

  // Berechne Umdrehungen pro Minute.
  windSensor.loop();
  windSpeed_rpm = windSensor.getData() * 60.0f;

  pressure_hPa = (float)pressureSensor.readPressure() / 100.0; // Konvertierung von Pascal zu hPa.
  pressure_hPa = pressure_hPa / 0.938; // Luftdruck auf Meereshöhe statt 500m.

  showStatusMessage("Messung aktualisiert.");
}


// Malt den Bildschirminhalt neu.
void redrawScreen() {
  display.clearBuffer();
  display.setFont(u8g_font_helvB10);
  display.drawStr(0, 13, ("Temp: " + String(temperature_C, 1) + " C").c_str());
  display.drawStr(0, 30, ("Wind: " + String(windSpeed_rpm, 1) + " U/min").c_str());
  display.drawStr(0, 47, ("Druck: " + String(pressure_hPa, 1) + " hPa").c_str());

  // Wenn Magnetsensor im Windrad kürzlich ausgelöst hat, so male einen
  // kleinen Kreis in die rechte obere Ecke.
  if (millis() < windSensor.getTimeOfLastSignalEdge() + 250) {
    display.drawDisc(123, 5, 4);
  }

  // Wenn die Statusnachricht kürzlich neu gesetzt wurde,
  // so schreibe sie in die unterste Zeile.
  if (millis() < statusMessageTime_ms + 1000) {
    display.setFont(u8g_font_helvR08);
    display.drawStr(0, 62, statusMessage.c_str());
  }

  display.sendBuffer();
}


void loop () {
  static unsigned long nextEspRestartTime_ms = 30 * 60 * 1000;
  static unsigned long nextSendMeasurementToThingSpeakTime_ms = 15000;
  static unsigned long nextUpdateMeasurementsTime_ms = 1000;
  static unsigned long nextRedrawScreenTime_ms = 1500;

  if (millis() > nextEspRestartTime_ms) {
    ESP.restart();
    // Das Programm hört hier auf.
  } else if (millis() > nextSendMeasurementToThingSpeakTime_ms) {
    showStatusMessage("Sende zu ThingSpeak ...");
    bool success = sender.sendMeasurement(String(temperature_C), String(windSpeed_rpm));
    if (success) {
      showStatusMessage("An ThingSpeak versandt.");
      nextSendMeasurementToThingSpeakTime_ms = millis() + 20 * 1000;
    } else {
      showStatusMessage("Keine Verbindung zu ThingSpeak.");
      // Wenn Senden nicht erfolgreich, dann neuer Versuch nicht sofort, sondern
      // nach 10 Sekunden, damit zwischendurch andere Funktionen ausgeführt werden
      // können.
      nextSendMeasurementToThingSpeakTime_ms = millis() + 10 * 1000;
    }
  } else if (millis() > nextUpdateMeasurementsTime_ms) {
    updateMeasurements();
    nextUpdateMeasurementsTime_ms = millis() + 10 * 1000;
  } else if (millis() > nextRedrawScreenTime_ms) {
    redrawScreen();
    nextRedrawScreenTime_ms = millis() + 33;
  }
}
