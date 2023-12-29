# Kombi-Uhr (Uhr, Countdown, Temperatur, Scoreboard) 
Die Software basiert aud dem Code von https://github.com/leonvandenbeukel/7-Segment-Digital-Clock-V2

-----------------------------------------
License

Creative Commons License
This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
http://creativecommons.org/licenses/by-nc-sa/4.0/

----------------------------------------

Benötigte Bauteile:
* 1x ESP8266 WeMos Mini D1  ---  Board WeMos D1 R1
* 1x DS3231 RTC  
* 1x Micro USB Breakout board  
* 1x Micro USB Kabel
* 1x 5V 2.5A Netzteil
* 2x WS2812B LED Strip 60 LED's pro Meter
------------------------------------------
*  Siebdruckplatten für den Bau des Gehäuses
*  Plexiglasplatte als Wetterschutz vorne
*  Scharniere und Schloss
*  3D-gedruckte Komponenten für die 7-Segmente etc.

-------------------------------------------

Die Aufgabe war es, für einen Sportverein eine Uhr zu konstruieren, die groß genug ist, um sie auch bei schneller Vorbeifahrt ablesen zu können.
Es soll ein Countdown angezeigt werden, die Länge des Countdowns soll frei wählbar sein.

Um das ganze auch sonst nutzen zu können, soll es auch möglich sein, die Uhrzeit anzuzeigen, dafür kommt ein Echtzeituhrenbaustein zum Einsatz, 
der mit einem NTP-Server synchronisiert, sobald sie Uhr mit dem Internet verbunden ist.

Auf der Suche nach fertigen Programmbeispielen stieß ich auf den Code von Leon van den Beukel
https://github.com/leonvandenbeukel/7-Segment-Digital-Clock-V2
der die wesentlichen Komponenten schin beinhaltet und zusätzlich noch ein Scoreboard bietet.
Diesen Code habe ich zum größten Teil übernommen und für meine Zwecke angepasst. 
Die größte Hürde für mich als Programmier-Anfänger war die Länge der Variablen, die durch die Anzahl der vielen LEDs meiner Anzeige den normalen Rahmen überschriotten hat.

---------------------------------------------

