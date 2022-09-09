// Kombi-Uhr (Uhr, Countdown, Temperatur, Scoreboard)
// Bauteile
// 1x ESP8266 WeMos Mini D1
// 1x DS3231 RTC  
// 1x Micro USB Breakout board  
// 1x Micro USB Kabel
// 1x 5V 2.5A Netzteil
// 2x WS2812B LED Strip 60 LED's pro Meter

#include <Wire.h>
#include <RtcDS3231.h>                        // Include RTC library by Makuna: https://github.com/Makuna/Rtc
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FastLED.h>
#include <LittleFS.h>
// #include <FS.h>                               // Anleitung auf http://arduino.esp8266.com/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system
#define countof(a) (sizeof(a) / sizeof(a[0]))
#define chipset ws2812B
#define NUM_LEDS 214                          // Insgesamt 214 LED's     
#define DATA_PIN D6                           // Daten-Pin für die LED-Streifen. Wird ein anderer ESP verwendet als der D1 mini, dann muss der Port hier angepasst werden
#define MILLI_AMPS 2400                       // maximaler Strom durch die LEDs
#define COUNTDOWN_OUTPUT D5                   // D5 geht auf High, wenn der Countdown abgelaufen ist

#define WIFIMODE 0                            // 0 = Nur Soft Access Point, 1 = Nur am Lokalen WLAN anmelden mit UN/PW, 2 = Beides

#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2)
  const char* APssid = "MCA-Uhr";            // SSID fuer den Soft-Access Point
  const char* APpassword = "2134567890";     // Passwort für den Soft-Access Point
#endif
  
// #if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2)
//  #include "Credentials.h"                   // Create this file in the same directory as the .ino file and add your credentials (#define SID YOURSSID and on the second line #define PW YOURPASSWORD)
//  const char* ssid = SID;                    // Liest die SSID des WLAN aus der Datei  
//  const char* password = PW;                 // Liest das Passwort für das WLAN aus der Datei
// #endif

RtcDS3231<TwoWire> Rtc(Wire);                // Echtzeituhr
ESP8266WebServer server(80);                 // Webserver auf Port 80
ESP8266HTTPUpdateServer httpUpdateServer;    // Webserver aktualisieren
CRGB LEDs[NUM_LEDS];

// Einstellungen

unsigned long prevTime = 0;
byte r_val = 255;                             // Wert fuer Rot (RGB)
byte g_val = 0;                               // Wert fuer Gelb (RGB)
byte b_val = 0;                               // Wert fuer Blau (RGB)
bool dotsOn = true;                           // Doppelpunkte werden verwendet
byte brightness = 255;                        // volle Helligkeit
float temperatureCorrection = -3.0;           // Korrekturwert für die Temperaturanzeige. Temperatur wird aus der Echtzeituhr ausgelesen
byte temperatureSymbol = 12;                  // 12=Celcius, 13=Fahrenheit siehe 'numbers'
byte clockMode = 0;                           // Uhr-Modis: 0=Uhr, 1=Countdown, 2=Temperatur, 3=Scoreboard
unsigned long countdownMilliSeconds;
unsigned long endCountDownMillis;
byte hourFormat = 24;                         // Soll die Uhrzeit im 12 Stunden Format ausgegeben werden, muss hier '12' eingetragen werden               
CRGB countdownColor = CRGB::Green;            // Anzeigefarbe des Countdowns auf gruen setzen
byte scoreboardLeft = 0;
byte scoreboardMid = 0;
byte scoreboardRight = 0;
CRGB scoreboardColorLeft = CRGB::Green;       // Anzeige am Scoreboard: links gruen
CRGB scoreboardColorMid = CRGB::Black;        // Anzeige am Scoreboard: mitte aus
CRGB scoreboardColorRight = CRGB::Red;        // Anzeige am Scoreboard: rechts rot
CRGB alternateColor = CRGB::Black;            // Wenn nichts angezeigt werden soll
long numbers[] = {                            // Hier werden die LEDs addressiert 
  0b00000111111111111111111111111111111,  // [0] 0
  0b00000111110000000000000000000011111,  // [1] 1
  0b11111111111111100000111111111100000,  // [2] 2
  0b11111111111111100000000001111111111,  // [3] 3
  0b11111111110000011111000000000011111,  // [4] 4
  0b11111000001111111111000001111111111,  // [5] 5
  0b11111000001111111111111111111111111,  // [6] 6
  0b00000111111111100000000000000011111,  // [7] 7
  0b11111111111111111111111111111111111,  // [8] 8
  0b11111111111111111111000001111111111,  // [9] 9
  0b00000000000000000000000000000000000,  // [10] alle aus
  0b11111111111111111111000000000000000,  // [11] Grad-Ssymbol
  0b00000000001111111111111111111100000,  // [12] C(elsius)
  0b11111000001111111111111110000000000,  // [13] F(ahrenheit)
};

void setup() {
  pinMode(COUNTDOWN_OUTPUT, OUTPUT);
  Serial.begin(115200); 
  delay(200);


 setupFS(); // Fred
    
  // Echtzeituhr: Wird am ESP wie folgt angeschlossen: 3.3V, GND, SCL=D1, SDA=D2
  // RTC DS3231 Setup
  Rtc.Begin();    
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);  // Set current Datum und Uhrzeit if it was not running

  if (!Rtc.IsDateTimeValid()) {
      if (Rtc.LastError() != 0) {
          // we have a communications error see https://www.arduino.cc/en/Reference/WireEndTransmission for what the number means
          Serial.print("RTC Kommunikationsfehler = ");
          Serial.println(Rtc.LastError());
      } else {
          // Häufige Ursachen:
          //    1) Die Uhr läuft das erste Mal und die Uhrzeit ist noch nicht gesetzt
          //    2) Die Batterie des Uhrmoduls ist schwach oder fehlt
          Serial.println("RTC hat Datum/Uhrzeit verloren!");
          // Die folgende Zeile setzt die RTC auf das Datum und die Uhrzeit, zu der diese Skizze kompiliert wurde
          // es wird auch das Valid-Flag intern zurücksetzen, es sei denn, das RTC-Gerät hat ein Problem

          Rtc.SetDateTime(compiled);
      }
  }

  WiFi.setSleepMode(WIFI_NONE_SLEEP);  // WLAN nicht abschalten

  delay(200);
  Serial.setDebugOutput(true); // Fred wenns läuft, wieder auskommentieren
  
  // Check if you're LED strip is a RGB or GRB version (third parameter)
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // WiFi AP Modus oder beides
#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2) 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APssid, APpassword);    // IP is festgelegt auf 192.168.4.1
  Serial.println();
  Serial.print("IP Soft-AccessPoint: ");
  Serial.println(WiFi.softAPIP());
#endif
  // WiFi - WLAN Modus oder beides

#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2) 
  byte count = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // Stop if cannot connect
    if (count >= 60) {
      Serial.println("Kann mich nicht mit dem lokalen WLAN verbinden.");      
      return;
    }
       
    delay(500);
    Serial.print(".");
    LEDs[count] = CRGB::Green;
    FastLED.show();
    count+++++;
  }
  Serial.print("Lokale IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // IPAddress ip = WiFi.localIP();
  // Serial.println(ip[3]);
#endif   

  httpUpdateServer.setup(&server);

  // Handlers
  server.on("/color", HTTP_POST, []() {    
    r_val = server.arg("r").toInt();
    g_val = server.arg("g").toInt();
    b_val = server.arg("b").toInt();
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/setdate", HTTP_POST, []() { 
    // Beispieleingabe: Datum = "06 Sep 2022", Zeit = "12:34:56"
    // Jan Feb Mar Apr Mai Jun Jul Aug Sep Okt Nov Dez
    String datearg = server.arg("date");
    String timearg = server.arg("time");
    Serial.println(datearg);
    Serial.println(timearg);    
    char d[12];
    char t[9];
    datearg.toCharArray(d, 12);
    timearg.toCharArray(t, 9);
    RtcDateTime compiled = RtcDateTime(d, t);
    Rtc.SetDateTime(compiled);   
    clockMode = 0;                                          // Uhrzeit Modus
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/brightness", HTTP_POST, []() {    
    brightness = server.arg("brightness").toInt();    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/countdown", HTTP_POST, []() {    
    countdownMilliSeconds = server.arg("ms").toInt();     
    byte cd_r_val = server.arg("r").toInt();
    byte cd_g_val = server.arg("g").toInt();
    byte cd_b_val = server.arg("b").toInt();
    digitalWrite(COUNTDOWN_OUTPUT, LOW);
    countdownColor = CRGB(cd_r_val, cd_g_val, cd_b_val); 
    endCountDownMillis = millis() + countdownMilliSeconds;
    allBlank(); 
    clockMode = 1;                                           // Countdown Modus  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/temperature", HTTP_POST, []() {   
    temperatureCorrection = server.arg("correction").toInt();
    temperatureSymbol = server.arg("symbol").toInt();
    clockMode = 2;                                           // Temperatur Modus
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  

  server.on("/scoreboard", HTTP_POST, []() {   
    scoreboardLeft = server.arg("left").toInt();
    scoreboardRight = server.arg("right").toInt();
    scoreboardColorLeft = CRGB(server.arg("rl").toInt(),server.arg("gl").toInt(),server.arg("bl").toInt());
    scoreboardColorRight = CRGB(server.arg("rr").toInt(),server.arg("gr").toInt(),server.arg("br").toInt());
    clockMode = 3;                                           // Scoreboard Modus
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  

  server.on("/hourformat", HTTP_POST, []() {   
    hourFormat = server.arg("hourformat").toInt();
    clockMode = 0;                                           // Uhrzeit Modus
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 

  server.on("/clock", HTTP_POST, []() {       
    clockMode = 0;                                           // Uhrzeit Modus
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  
  
  // Before uploading the files with the "ESP8266 Sketch Data Upload" tool, zip the files with the command "gzip -r ./data/" (on Windows I do this with a Git Bash)
  // *.gz files are automatically unpacked and served from your ESP (so you don't need to create a handler for each file).
  // Fred server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  server.serveStatic("/", LittleFS, "/", "max-age=86400");
  server.begin();     

  LittleFS.begin();

  Serial.println("LittleFS enthält:");
  Dir dir = LittleFS.openDir("/");

  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
  }
  Serial.println(); 
  
  digitalWrite(COUNTDOWN_OUTPUT, LOW);
}

void loop(){

  server.handleClient(); 
  
  unsigned long currentMillis = millis();  
  if (currentMillis - prevTime >= 1000) {
    prevTime = currentMillis;

    if (clockMode == 0) {                                         // Uhrzeit Modus
      updateClock();
    } else if (clockMode == 1) {                                  // Countdown Modus
      updateCountdown();
    } else if (clockMode == 2) {                                  // Temperatur-Modus
      updateTemperature();      
    } else if (clockMode == 3) {                                  // Scoreboard Modus
      updateScoreboard();            
    }

    FastLED.setBrightness(brightness);
    FastLED.show();
  }   
}

void displayNumber(byte number, byte segment, CRGB color) {
  /*
   * 
   199200201202203      164165166167168         127128129130131       92 93 94 95 96          55 56 57 58 59        20 21 22 23 24  
  198           204    163           169       126           132    91              97      54              60    19              25
  197           205    162           170       125           133    90              98      53              61    18              26
  196           206    161           171       124           134    89              99      52              62    17              27
  195           207    160           172       123           135    88             100      51              63    16              28
  194           208    159           173  143  122           136    87             101  71  50              64    15              29
   213212211210209      178177176175174         141140139138137      106105104103102          69 68 67 66 65        34 33 32 31 30  
  193           179    158           144  142  121           107    86              72  70  49              35    14              00
  192           180    157           145       120           108    85              73      48              36    13              01
  191           181    156           146       119           109    84              74      47              37    12              02
  190           182    155           147       118           110    83              75      46              38    11              03
  189           183    154           148       117           111    82              76      45              39    10              04
   188187186185184      153152151150149         116115114113112       81 80 79 78 77          44 43 42 41 40        09 08 07 06 05   

   */
 
  // segment from left to right: 5, 4, 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = 35;

      break;
    case 2:
      startindex = 72;

      break;
    case 3:
      startindex = 107;

      break;
    case 4:
      startindex = 144;

      break;
    case 5:
      startindex = 179;

      break;                
  }

  for (byte i=0; i<35; i++){

    yield();
    LEDs[i + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? color : alternateColor;
  } 
}

void allBlank() {
  for (int i=0; i<NUM_LEDS; i++) {
    LEDs[i] = CRGB::Black;
  }
  FastLED.show();
}

void updateClock() {  
  RtcDateTime now = Rtc.GetDateTime();
  // printDateTime(now);    

  int hour = now.Hour();
  int mins = now.Minute();
  int secs = now.Second();

  if (hourFormat == 12 && hour > 12)
    hour = hour - 12;
  
  byte h1 = hour / 10;
  byte h2 = hour % 10;
  byte m1 = mins / 10;
  byte m2 = mins % 10;  
  byte s1 = secs / 10;
  byte s2 = secs % 10;
  
  CRGB color = CRGB(r_val, g_val, b_val);

//  if (h1 > 0)
//    displayNumber(h1,3,color);
//  else 
//    displayNumber(10,3,color);  // Blank
    
//  displayNumber(h2,2,color);
//  displayNumber(m1,1,color);
//  displayNumber(m2,0,color); 
// Fred
  if (h1 > 0)
    displayNumber(h1,5,color);
  else 
    displayNumber(10,5,color);  // dunkel
    
//  if (h2 > 0)
    displayNumber(h2,4,color);
//  else
//    displayNumber(10,4,color);  // dunkel
    
//  if (m1 > 0)
    displayNumber(m1,3,color);
//  else
//    displayNumber(10,3,color);  // dunkel
    
  displayNumber(m2,2,color); 
  displayNumber(s1,1,color);
  displayNumber(s2,0,color); 
  displayDots(color);  
}

void updateCountdown() {

  if (countdownMilliSeconds == 0 && endCountDownMillis == 0) 
    return;
    
  unsigned long restMillis = endCountDownMillis - millis();
  unsigned long hours   = ((restMillis / 1000) / 60) / 60;
  unsigned long minutes = (restMillis / 1000) / 60;
  unsigned long seconds = restMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  
  Serial.print(restMillis);
  Serial.print(" ");
  Serial.print(hours);
  Serial.print(" ");
  Serial.print(minutes);
  Serial.print(" ");
  Serial.print(seconds);
  Serial.print(" | ");
  Serial.print(remMinutes);
  Serial.print(" ");
  Serial.println(remSeconds);

  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;

  CRGB color = countdownColor;
  if (restMillis <= 60000) {
    color = CRGB::Red;
  }

  {
    // hh:mm:ss
    displayNumber(h1,5,color); 
    displayNumber(h2,4,color);
    displayNumber(m1,3,color);
    displayNumber(m2,2,color);  
    displayNumber(s1,1,color);
    displayNumber(s2,0,color);  
  }





  displayDots(color);  // Doppelpunkte einschalten

  if (hours <= 0 && remMinutes <= 0 && remSeconds <= 0) {
    Serial.println("Countdown beendet.");
    //endCountdown();
    countdownMilliSeconds = 0;
    endCountDownMillis = 0;
    //clockMode = 0;
    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
    return;
  }  
}

void endCountdown() {
  allBlank();
  for (int i=0; i<NUM_LEDS; i++) {
    if (i>0)
      LEDs[i-1] = CRGB::Black;
    
    LEDs[i] = CRGB::Red;
    FastLED.show();
    delay(25);
  }  
}

void displayDots(CRGB color) {
  if (dotsOn) {
    LEDs[70] = color;
    LEDs[71] = color;
    LEDs[142] = color;
    LEDs[143] = color;    
  } else {
    LEDs[70] = CRGB::Black;
    LEDs[71] = CRGB::Black;
    LEDs[142] = CRGB::Black;
    LEDs[143] = CRGB::Black;

  }

  dotsOn = !dotsOn;  
}

void hideDots() {
  LEDs[70] = CRGB::Black;
  LEDs[71] = CRGB::Black;
  LEDs[142] = CRGB::Black;
  LEDs[143] = CRGB::Black;
}

void updateTemperature() {
  RtcTemperature temp = Rtc.GetTemperature();
  float ftemp = temp.AsFloatDegC();
  float ctemp = ftemp + temperatureCorrection;
  Serial.print("Sensortemperatur: ");
  Serial.print(ftemp);
  Serial.print(" Korrigiert: ");
  Serial.println(ctemp);

  if (temperatureSymbol == 13)
    ctemp = (ctemp * 1.8000) + 32;

  byte t1 = int(ctemp) / 10;
  byte t2 = int(ctemp) % 10;
  CRGB color = CRGB(r_val, g_val, b_val);
  displayNumber(10,5,color);
  displayNumber(10,4,color);
  displayNumber(t1,3,color);
  displayNumber(t2,2,color);
  displayNumber(11,1,color);
  displayNumber(temperatureSymbol,0,color);
  hideDots();
}

void updateScoreboard() {
  byte sl1 = scoreboardLeft / 10;
  byte sl2 = scoreboardLeft % 10;
  byte sr1 = scoreboardRight / 10;
  byte sr2 = scoreboardRight % 10;

  displayNumber(sl1,5,scoreboardColorLeft);
  displayNumber(sl2,4,scoreboardColorLeft);
  displayNumber(10,3,scoreboardColorMid);  // Mittlere Ziffern dunkel
  displayNumber(10,2,scoreboardColorMid);  // Mittlere Ziffern dunkel
  displayNumber(sr1,1,scoreboardColorRight);
  displayNumber(sr2,0,scoreboardColorRight);
  hideDots();
}

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.println(datestring);
}
