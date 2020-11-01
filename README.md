# Wetterstation

Für Geo habe ich eine Wetterstation gebastelt. Diese misst Temperatur, Windgeschwindigkeit und Luftdruck, zeigt die Werte auf einem Display an und überträgt sie auch zu ThingSpeak.

![Box der Wetterstation in Betrieb](doc/Box_in_Betrieb.jpg)

## Elektronik

Folgende Komponenten wurden verwendet:

* Mikrocontroller: ESP32 NodeMCU
* Display: 1,3 Zoll OLED 128×64 mit SH1106 Chip
* Temperatursensor: DS18B20 (wasserdicht)
* Neigungssensor: SW-520D (hier als Reedkontakt verwendet)
* Luftdrucksensor: BMP280

Auf dem folgenden Foto sieht man den Temperatursensor, den USB-Anschluss, die kleine senkrechte Platine mit dem Luftdrucksensor und wie es unter dem Display aussieht. An der Seite sind zwei Buchsen, an die das Windrad (siehe unten) angeschlossen wird.

![Platine der Wetterstation](doc/Box_offen.jpg)

Hier auch eine Zeichnung der Schaltung (hier noch ohne Luftdrucksensor):

![Schaltung](doc/Schaltung.png)

## Messung der Windstärke

Für die Messung der Windstärke habe ich ein Windrad aus Holz gebastelt. Die Schalen für den Wind sind Ü-Ei-Becher. Die Windstärke wird in Umdrehungen pro Minute angezeigt. Um eine Umdrehung zu messen, ist im Halter des Windrads ein Neigungssensor und im Rotor ein Magnet.

![Magnet im Windrad](doc/Magnet_im_Windrad.jpg)

Wenn sich der Magnet über dem Neigungssensor bewegt, so löst dieser aus und wenn er sich weiterbewegt dann noch einmal.

![Windrad von der Seite](doc/Windrad.gif)

## Code

Das Programm habe ich mit der Arduino-IDE erstellt. Es findet sich unter [wetterstation/wetterstation.ino](wetterstation/wetterstation.ino).