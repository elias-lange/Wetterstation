#include <OneWire.h> 
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>

const char* WIFI_SSID = "Hier WLAN-SSID eintragen";
const char* WIFI_PASSWORD = "Hier WLAN-Passwort eintragen";
const char* THING_SPEAK_API_KEY = "Hier API-Key für ThingSpeak eintragen";

U8G2_SH1106_128X64_NONAME_F_SW_I2C display(U8G2_R0, 15, 18);

volatile int magnetEventCount = 0;
volatile unsigned long timeOfLastMagnetEvent_ms = 0;
float temperature_C = 0.0;
float windSpeed_rpm = 0.0;

String statusMessage = "";
unsigned long statusMessageTime_ms = 0;

OneWire oneWire(19); 
DallasTemperature sensors(&oneWire);


// Diese Funktion ist ein Interrupt-Handler. Sie wird jedes Mal aufgerufen,
// wenn der Neigungssensor im Windrad auslöst, das heißt seinen Zustand
// ändert. Bei jeder Umdrehung wird der Interrupt zwei Mal ausgelöst.
// Tatsächlich löst der Interrupt noch öfters aus, weil die Schaltung
// 'prellt' (https://de.wikipedia.org/wiki/Prellen). Deswegen werden
// mehrere Auslösungen innerhalb von 10ms ignoriert.
void IRAM_ATTR onMagnetEvent() {
  const unsigned long MAGNET_EVENT_DEBOUNCE_MS = 10;
  if (millis() > timeOfLastMagnetEvent_ms + MAGNET_EVENT_DEBOUNCE_MS) {
    magnetEventCount++;
    timeOfLastMagnetEvent_ms = millis();
  }
}


void setup() {
  display.begin();

  // Initialisierung für Temperatursensor.
  sensors.begin();

  // Pin für Neigungssensor im Windrad.
  pinMode(16, INPUT);

  // Da Schaltung 'prellt' unbedingt CHANGE und nicht nur RISING oder FALLING
  // verwenden! Wenn nur RISING oder FALLING verwendet würde, so würde bei
  // Prellen sowieso wie CHANGE gezählt, bei Nicht-Prellen aber nur RISING
  // bzw. FALLING und damit inkonsistent.
  // Details: https://de.wikipedia.org/wiki/Prellen
  attachInterrupt(digitalPinToInterrupt(16), onMagnetEvent, CHANGE);
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
  static unsigned long lastUpdateMeasurements_ms = 0;

  showStatusMessage("Aktualisiere Messung ...");
  
  // Lies Temperatur aus.
  sensors.requestTemperatures(); 
  temperature_C = sensors.getTempCByIndex(0);

  // Berechne Umdrehungen pro Minute.
  windSpeed_rpm = 60.0 * 1000.0 * (magnetEventCount / 2.0) / (millis() - lastUpdateMeasurements_ms);
  magnetEventCount = 0;

  lastUpdateMeasurements_ms = millis();

  showStatusMessage("Messung aktualisiert.");
}


// Diese Funktion verbindet sich mit dem WLAN und sendet dann
// eine Messung an ThingSpeak. Wenn alles geklappt hat, so wird
// 'true' zurückgegeben, sonst 'false'.
bool sendMeasurementToThingSpeak() {
  // Nachricht an ThingSpeak zusammensetzen.
  String url = String("https://api.thingspeak.com/update?api_key=") + String(THING_SPEAK_API_KEY) + String("&field1=") + String(temperature_C) + String("&field2=") + String(windSpeed_rpm);

  // Verbinde mit WLAN.
  boolean success = false;
  WiFiClient client;
  WiFi.mode(WIFI_STA);
  showStatusMessage("Verbinde zu " + String(WIFI_SSID) + " ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(5000); // Warte einige Sekunden bis Verbindung aufgebaut.

  if (WiFi.status() == WL_CONNECTED) {
    showStatusMessage("Sende zu ThingSpeak ...");
    HTTPClient http;
    http.begin(url.c_str());
    int response = http.GET();
    if (response == 200) {
      success = true;
      showStatusMessage("An ThingSpeak versandt.");
    } else {
      showStatusMessage("HTTP Error: " + String(response) + ".");
    }
    http.end();
  } else {
    showStatusMessage("Keine Verbindung zu " + String(WIFI_SSID) + ".");
  }
  client.stop();
  return success;
}


// Malt den Bildschirminhalt neu.
void redrawScreen() {
  display.clearBuffer();
  display.setFont(u8g_font_helvB10);
  display.drawStr(0, 20, ("Temp: " + String(temperature_C, 1) + " C").c_str());
  display.drawStr(0, 40, ("Wind: " + String(windSpeed_rpm, 1) + " U/min").c_str());

  // Wenn Magnetsensor im Windrad kürzlich ausgelöst hat, so male einen
  // kleinen Kreis in die rechte obere Ecke.
  if (millis() < timeOfLastMagnetEvent_ms + 250) {
    display.drawDisc(120, 8, 4);
  }

  // Wenn die Statusnachricht kürzlich neu gesetzt wurde,
  // so schreibe sie in die unterste Zeile.
  if (millis() < statusMessageTime_ms + 1000) {
    display.setFont(u8g_font_helvR08);
    display.drawStr(0, 60, statusMessage.c_str());
  }

  display.sendBuffer();
}


void loop () {
  static unsigned long nextEspRestartTime_ms = 3 * 60 * 60 * 1000;
  static unsigned long nextSendMeasurementToThingSpeakTime_ms = 15000;
  static unsigned long nextUpdateMeasurementsTime_ms = 1000;
  static unsigned long nextRedrawScreenTime_ms = 1500;

  if (millis() > nextEspRestartTime_ms) {
    ESP.restart();
    // Das Programm hört hier auf.
  } else if (millis() > nextSendMeasurementToThingSpeakTime_ms) {
    bool success = sendMeasurementToThingSpeak();
    if (success) {
      nextSendMeasurementToThingSpeakTime_ms = millis() + 15 * 60 * 1000;
    } else {
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
