/*
 * MoodLight
 */

#include <avr/pgmspace.h>
#include <Adafruit_NeoPixel.h>
#include <CapacitiveSensor.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 3
Adafruit_NeoPixel strip = Adafruit_NeoPixel(9, PIN, NEO_GRB + NEO_KHZ800);

CapacitiveSensor   cs_9_8 = CapacitiveSensor(9, 8);
unsigned long total1 = 0;
unsigned long lastTouch = 0;
unsigned long touchTime = 0;
int touchThreshold = 50;

//uncomment for Serial debug output at 38400 baud
//#define DEBUG
unsigned long debugNextTick = 0;
const int debugInterval PROGMEM = 500;

String outString = "";
bool serialDone = false;
char* mode = "MOOD";

bool shutDownLamp = false;

unsigned long tickNow = 0;
unsigned long tickNext = 0;
byte tickTime = 30; // time between strip updates (frame rate)
byte tickTimeDefault = tickTime;
float tickMultiplier = 1.00;
float masterBrightness = 1.00;
int i = 0;

int moodLevels[6] = {0, 0, 0, 0, 0, 0};
char* moodNames[6] = {"LOVE", "JOY", "SURPRISE", "ANGER", "SADNESS", "FEAR"};

const float moodColors[6][3] = {
  {1.00, 0.20, 0.34},
  {1.00, 0.40, 0.00},
  {0.50, 0.00, 1.00},
  {1.00, 0.00, 0.00},
  {0.00, 1.00, 1.00},
  {0.20, 1.00, 0.20}
};

int highest = 0;
const byte majorThreshold PROGMEM = 250;
byte mainfadeDirection[4] = {0, 0, 0, 1};
float mainbrightness[4] = {1, 1, 1, 1};
float mainbrightnessF[4] = {1, 1, 1, 1};
float maincolorR[4] = {0, 0, 0, 0};
float maincolorG[4] = {1, 1, 1, 1};
float maincolorB[4] = {1, 1, 1, 1};
float maincolorRnext[4] = {0, 0, 0, 0};
float maincolorGnext[4] = {1, 1, 1, 1};
float maincolorBnext[4] = {1, 1, 1, 1};

const int loveVal  = 0;
const int joyVal  = 1;
const int surpriseVal  = 2;
const int angerVal  = 3;
const int sadnessVal  = 4;
const int fearVal  = 5;

byte comType = 0;
unsigned int checkSum = 0;
bool checkSumPass = false;
unsigned int sum = 0;

byte secLevel = 0;
byte secLevelNext = 0;
const byte secLevelDefault = 255;

float secbrightness[5] = {0, 0, 0, 0, 0};
float secbrightnessF[5] = {0, 0, 0, 0, 0};

byte secfadeDirection[5] = {0, 0, 0, 0, 0};

float seccolorR[5] = {0, 0, 0, 0, 0};
float seccolorG[5] = {1, 1, 1, 1, 1};
float seccolorB[5] = {1, 1, 1, 1, 1};

float seccolorRnext[5] = {0, 0, 0, 0, 0};
float seccolorGnext[5] = {1, 1, 1, 1, 1};
float seccolorBnext[5] = {1, 1, 1, 1, 1};

int com1 = 0;
int com2 = 0;
int com3 = 0;
int com4 = 0;
int com5 = 0;
int com6 = 0;

byte wifiColorR = 255;
byte wifiColorG = 0;
byte wifiColorB = 0;

bool interpolation = true;
float interpolationSpeed = 0.05;

const byte main[4] = {1, 3, 5, 7};
const byte sec[5] = {0, 2, 4, 6, 8};

byte wifiReset = 2;

byte sleepStateNext = 1;
byte sleepState = 1;
float sleepBrightness = 0.01;
float notifyBrightness = 1.00;

float brightnessLevels[] = {1.00, 0.50, 0.20};
byte brightnessIndex = 0;

String comString = "X";

void debugWiFi(String input, bool newline = true) {
  Serial.print(input);
  if (newline == true) {
    Serial.print("\n");
  }
}

void setup() {
  Serial.begin(38400);
  pinMode(wifiReset, OUTPUT);
  digitalWrite(wifiReset, LOW);
  delay(500);
  digitalWrite(wifiReset, HIGH);
  debugWiFi("----------------------------");
  debugWiFi("MoodLight V1.0");
  debugWiFi("----------------------------");
  // put your setup code here, to run once:

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  masterBrightness = 0;
  while (masterBrightness < 0.99) {
    i = 0;
    while (i < 4) {
      strip.setPixelColor(main[i], wifiColorR * (masterBrightness) , wifiColorG * (masterBrightness) , wifiColorB * (masterBrightness));
      i++;
    }

    strip.show();
    masterBrightness += 0.02;
    delay(tickTime);
  }

  bool wifiSolved = false;
  byte wifiMode = 0;
  byte conCount = 0;
  byte sucCount = 0;
  byte serCount = 0;
  byte badCount = 0;
  while (wifiSolved == 0) {
    // put your main code here, to run repeatedly:
    if (Serial.available() > 0) {
      char c = char(Serial.read());
      if (c == '#') {
        conCount++;
        sucCount = 0;
        serCount = 0;
        badCount = 0;
      }
      if (c == '$') {
        sucCount++;
        conCount = 0;
        serCount = 0;
        badCount = 0;
      }
      if (c == '*') {
        serCount++;
        conCount = 0;
        sucCount = 0;
        badCount = 0;
      }
      if (c == '!') {
        badCount++;
        conCount = 0;
        sucCount = 0;
        serCount = 0;
      }

      if (conCount == 3) {
        wifiColorR = 255;
        wifiColorG = 120;
        wifiColorB = 000;
        wifiMode = 1;
      }
      if (sucCount == 3) {
        wifiColorR = 000;
        wifiColorG = 255;
        wifiColorB = 000;
        wifiMode = 2;
      }
      if (serCount == 3) {
        wifiColorR = 000;
        wifiColorG = 255;
        wifiColorB = 255;
        wifiMode = 3;
      }
      if (badCount == 3) {
        wifiColorR = 128;
        wifiColorG = 000;
        wifiColorB = 255;
        wifiMode = 4;
      }
    }

    i = 0;
    while (i < 4) {
      strip.setPixelColor(main[i], wifiColorR , wifiColorG , wifiColorB);
      i++;
    }

    strip.show();

    if (wifiMode == 3) {
      wifiSolved = true;
    }
  }
  Serial.println("START LOOP\n");
}

void(* resetFunc) (void) = 0;

void loop() {
  checkSerial();
  checkTick();
  //  debugVariables();
}

void checkTouch() {
  if (millis() - lastTouch > 400) {
    total1 = cs_9_8.capacitiveSensor(30);
    if (total1 > touchThreshold) {
      lastTouch = millis();
      touchTime = 0;
      //do here
      debugWiFi("Touch begin");
      while (total1 > touchThreshold) {
        total1 = cs_9_8.capacitiveSensor(30);
        touchTime = millis() - lastTouch;

        //nothing
      }
      debugWiFi("Touch ended. Duration: ", false);
      debugWiFi(String(touchTime), false);
      debugWiFi("ms");
      if (touchTime < 250) {
        if (sleepState == 1) {
          brightnessIndex++;
          if (brightnessIndex > 2) {
            brightnessIndex = 0;
          }
          masterBrightness = brightnessLevels[brightnessIndex];
        }
      }
      else if (touchTime >= 250 && touchTime < 5000) {
        if (sleepState == 1) {
          sleepStateNext = 0;
        }
        else if (sleepState == 0 || sleepState == 2) {
          sleepStateNext = 1;
        }
      }
      else if (touchTime >= 5000) {
        Serial.println("%201999");
      }
    }
  }
}

void checkSleep() {
  if (sleepState != sleepStateNext) {
    brightnessIndex = 0;
    if (sleepStateNext == 0 || sleepStateNext == 2) {
      if (masterBrightness > sleepBrightness) {
        masterBrightness -= 0.01;
      }
      else {
        sleepState = sleepStateNext;
        debugWiFi("sleepState changed to ", false);
        debugWiFi(String(sleepStateNext));
      }
    }
    if (sleepStateNext == 1) {
      if (masterBrightness < 0.99) {
        masterBrightness += 0.01;
      }
      else {
        sleepState = sleepStateNext;
        debugWiFi("sleepState changed to ", false);
        debugWiFi(String(sleepStateNext));
      }
    }

  }
}

unsigned long newrandom(unsigned long howsmall, unsigned long howbig)
{
  randomSeed(analogRead(A0));
  return howsmall + random() % (howbig - howsmall);
}

void checkSerial() {
  if (serialDone == true) {
    serialDone = false;
    comString = outString;
    parseInput();
    outString = "";
  }

  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    char c = char(Serial.read());
    if (c == '%') {
      outString = "";
    }
    else if (c == '\n') {
      serialDone = true;
    }
    else {
      outString += String(c);
    }
  }
}

void checkTick() {
  tickNow = millis();
  if (tickNow >= tickNext) {
    tickNext = tickNow + tickTime * tickMultiplier;
    if (mode == "MOOD" || mode == "TWIRL") {
      cycleMain();
      cycleSec();
      checkSleep();
      checkTouch();
    }
    else if (mode == "MANUAL") {
      if (interpolation == true) {
        i = 0;
        while (i < 5) {
          if (seccolorR[i] > seccolorRnext[i]) {
            seccolorR[i] -= interpolationSpeed;
          }
          if (seccolorR[i] < seccolorRnext[i]) {
            seccolorR[i] += interpolationSpeed;
          }
          if (seccolorG[i] > seccolorGnext[i]) {
            seccolorG[i] -= interpolationSpeed;
          }
          if (seccolorG[i] < seccolorGnext[i]) {
            seccolorG[i] += interpolationSpeed;
          }
          if (seccolorB[i] > seccolorBnext[i]) {
            seccolorB[i] -= interpolationSpeed;
          }
          if (seccolorB[i] < seccolorBnext[i]) {
            seccolorB[i] += interpolationSpeed;
          }
          i++;
        }

        i = 0;
        while (i < 4) {
          if (maincolorR[i] > maincolorRnext[i]) {
            maincolorR[i] -= interpolationSpeed;
          }
          if (maincolorR[i] < maincolorRnext[i]) {
            maincolorR[i] += interpolationSpeed;
          }
          if (maincolorG[i] > maincolorGnext[i]) {
            maincolorG[i] -= interpolationSpeed;
          }
          if (maincolorG[i] < maincolorGnext[i]) {
            maincolorG[i] += interpolationSpeed;
          }
          if (maincolorB[i] > maincolorBnext[i]) {
            maincolorB[i] -= interpolationSpeed;
          }
          if (maincolorB[i] < maincolorBnext[i]) {
            maincolorB[i] += interpolationSpeed;
          }
          i++;
        }
      }
      else {
        i = 0;
        while (i < 5) {
          seccolorR[i] = seccolorRnext[i];
          seccolorG[i] = seccolorGnext[i];
          seccolorB[i] = seccolorBnext[i];
          i++;
        }
        i = 0;
        while (i < 4) {
          maincolorR[i] = maincolorRnext[i];
          maincolorG[i] = maincolorGnext[i];
          maincolorB[i] = maincolorBnext[i];
          i++;
        }
      }

      i = 0;
      while (i < 4) {
        strip.setPixelColor(main[i], 255 * (constrain(maincolorR[i], 0.00, 1.00)) , 255 * (constrain(maincolorG[i], 0.00, 1.00)) , 255 * (constrain(maincolorB[i], 0.00, 1.00)));
        i++;
      }

      i = 0;
      while (i < 5) {
        strip.setPixelColor(sec[i], 255 * (constrain(seccolorR[i], 0.00, 1.00)) , 255 * (constrain(seccolorG[i], 0.00, 1.00)) , 255 * (constrain(seccolorB[i], 0.00, 1.00)));
        i++;
      }

    }

    if (shutDownLamp == true) {
      debugWiFi("Shutting down lamp...");
      if (masterBrightness > 0) {
        masterBrightness -= 0.03;
        masterBrightness = constrain(masterBrightness, 0, 1.00);
      }
      else {
        debugWiFi("Bye!");
        resetFunc(); // CALL OFFICIAL RESET
      }
    }

    strip.show();
  }
}

void cycleMain() {
  i = 0;
  while (i < 4) {
    if (mainbrightness[i] < 1.99 && mainfadeDirection[i] == 1) {
      mainbrightness[i] = mainbrightness[i] + (0.01 + (newrandom(0, 5) / 100.00));
    }
    else {
      mainfadeDirection[i] = 0;
    }

    if (mainbrightness[i] > -0.50 && mainfadeDirection[i] == 0) {
      mainbrightness[i] -= 0.01;
    }
    else {
      mainfadeDirection[i] = 1;
    }


    if (mainbrightness[i] <= 0) {
      maincolorR[i] = maincolorRnext[i];
      maincolorG[i] = maincolorGnext[i];
      maincolorB[i] = maincolorBnext[i];
    }

    mainbrightnessF[i] = mainbrightness[i];

    if (highest < majorThreshold) {
      mainbrightnessF[i] = constrain(mainbrightness[i], 0.00, 1.00);
    }

    strip.setPixelColor(main[i], 255 * (maincolorR[i]) * mainbrightnessF[i] * (masterBrightness) , 255 * (maincolorG[i]) * mainbrightnessF[i] * (masterBrightness) , 255 * (maincolorB[i]) * mainbrightnessF[i] * (masterBrightness));
    i++;
  }
}

void cycleSec() {
  i = 0;
  while (i < 5) {
    if (secbrightness[i] < 0.99 && secfadeDirection[i] == 1) {
      secbrightness[i] = secbrightness[i] + (0.01 + (newrandom(0, 5) / 100.00));
    }
    else {
      secfadeDirection[i] = 0;
    }
    if (secbrightness[i] > -0.20 && secfadeDirection[i] == 0) {
      secbrightness[i] -= 0.01;
    }
    else {
      secfadeDirection[i] = 1;
    }

    if (secbrightness[i] <= 0) {
      seccolorR[i] = seccolorRnext[i];
      seccolorG[i] = seccolorGnext[i];
      seccolorB[i] = seccolorBnext[i];
    }

    secbrightnessF[i] = secbrightness[i];

    secbrightnessF[i] = constrain(secbrightness[i], 0.00, 1.00);

    i++;
  }

  if (secLevel != secLevelNext) {
    if (secLevel > secLevelNext) {
      secLevel--;
    }
    if (secLevel < secLevelNext) {
      secLevel++;
    }
  }

  i = 0;
  while (i < 5) {
    strip.setPixelColor(sec[i], secLevel * (seccolorR[i]) * (secbrightnessF[i]) * (masterBrightness) , secLevel * (seccolorG[i]) * (secbrightnessF[i]) * (masterBrightness) , secLevel * (seccolorB[i]) * (secbrightnessF[i]) * (masterBrightness));
    i++;
  }

}

void parseInput() {
  byte header = comString.substring(0, 3).toInt();
  if (header == 001) {
    debugWiFi("Mood received: ", false);
    debugWiFi(comString);
    comType = 1;
    moodLevels[loveVal] = comString.substring(3, 6).toInt();
    moodLevels[joyVal] = comString.substring(6, 9).toInt();
    moodLevels[surpriseVal] = comString.substring(9, 12).toInt();
    moodLevels[angerVal] = comString.substring(12, 15).toInt();
    moodLevels[sadnessVal] = comString.substring(15, 18).toInt();
    moodLevels[fearVal] = comString.substring(18, 21).toInt();
    parseMood();
    comString = "";
  }
  if (header == 101) {
    debugWiFi("Command received: ", false);
    debugWiFi(comString);
    comType = 2;
    parseCommand();
    comString = "";
  }
  else {
    comType = 666;
    comString = "";
  }



#ifdef DEBUG
  Serial.print("\n");
  Serial.print("COM RX: ");
  Serial.println(comString);
  Serial.print("COM TYPE: ");
  if (comType == 1) {
    Serial.println("MOOD");
    Serial.print("LOVE:     ");
    Serial.println(moodLevels[loveVal]);
    Serial.print("JOY:      ");
    Serial.println(moodLevels[joyVal]);
    Serial.print("SURPRISE: ");
    Serial.println(moodLevels[surpriseVal]);
    Serial.print("ANGER:    ");
    Serial.println(moodLevels[angerVal]);
    Serial.print("SADNESS:  ");
    Serial.println(moodLevels[sadnessVal]);
    Serial.print("FEAR:     ");
    Serial.println(moodLevels[fearVal]);
  }
  if (comType == 2) {
    Serial.println("COMMAND");
  }
  else if (comType == 666) {
    Serial.println("BAD COMMAND! -----------------------------!");
  }
  Serial.print("\n");

  // More debug code...
#endif
}

void parseCommand() {
  Serial.println("COM: ");
  Serial.println(comString);
  com1 = (int)comString.substring(3, 6).toInt();
  Serial.println(com1);
  if (com1 == 1) {
    com2 = float(comString.substring(6, 9).toInt());
    if (com2 == 0) {
      debugWiFi("Set to mood mode");
      mode = "MOOD";
      tickTime = tickTimeDefault;
      tickTime = 30;
    }
    else if (com2 == 1) {
      mode = "MANUAL";
      debugWiFi("Set to manual mode");
      tickTime = 5;
    }
  }
  if (com1 == 2) {
    com2 = float(comString.substring(6, 9).toInt());
    masterBrightness = com2 / 100.00;
    debugWiFi("Brightness set to ", false);
    debugWiFi(String(masterBrightness));
  }
  if (com1 == 3) {
    com2 = float(comString.substring(6, 9).toInt());
    com3 = float(comString.substring(9, 12).toInt());
    com4 = float(comString.substring(12, 15).toInt());
    mode = "MANUAL";
    debugWiFi("Set to manual mode");
    debugWiFi("Color set to: R", false);
    debugWiFi(String(com2), false);
    debugWiFi(", G", false);
    debugWiFi(String(com3), false);
    debugWiFi(", B", false);
    debugWiFi(String(com4));

    i = 0;
    while (i < 5) {
      seccolorRnext[i] = (com2 / 255.00);
      seccolorGnext[i] = (com3 / 255.00);
      seccolorBnext[i] = (com4 / 255.00);
      i++;
    }

    i = 0;
    while (i < 4) {
      maincolorRnext[i] = (com2 / 255.00);
      maincolorGnext[i] = (com3 / 255.00);
      maincolorBnext[i] = (com4 / 255.00);
      i++;
    }
  }
  if (com1 == 4) {

    com2 = float(comString.substring(6, 9).toInt());
    com3 = float(comString.substring(9, 12).toInt());
    com4 = float(comString.substring(12, 15).toInt());
    com5 = float(comString.substring(15, 18).toInt());
    com6 = float(comString.substring(18, 21).toInt());

    debugWiFi("Set to manual mode\nPixel ");
    debugWiFi(String(com2), false);
    debugWiFi(" set to: R", false);
    debugWiFi(String(com2), false);
    debugWiFi(", G", false);
    debugWiFi(String(com3), false);
    debugWiFi(", B", false);
    debugWiFi(String(com4));

    mode = "MANUAL";
    strip.setPixelColor((int)com2, (int)com3, (int)com4, (int)com5);
    if ((int)com6 == 1) {
      strip.show();
    }
  }
  if (com1 == 5) {
    debugWiFi("Strip updated.");
    strip.show();
  }
  if (com1 == 6) {
    com2 = float(comString.substring(6, 9).toInt());
    touchThreshold = com2 * 10;
    debugWiFi("Touch threshold set to: ", false);
    debugWiFi(String(touchThreshold));
  }
  if (com1 == 7) {
    com2 = float(comString.substring(6, 9).toInt());
    sleepStateNext = com2;
    debugWiFi("Sleep state set to: ", false);
    debugWiFi(String(sleepStateNext));
  }

  if (com1 == 8) {
    com2 = int(comString.substring(6, 9).toInt());
    com3 = int(comString.substring(9, 12).toInt());
    com4 = int(comString.substring(12, 15).toInt());
    com5 = int(comString.substring(15, 18).toInt());

    debugWiFi("Set to manual mode\nFlashing ", false);
    debugWiFi(String(com5), false);
    debugWiFi(" times with color: R", false);
    debugWiFi(String(com2), false);
    debugWiFi(", G", false);
    debugWiFi(String(com3), false);
    debugWiFi(", B", false);
    debugWiFi(String(com4));

    if (sleepState != 2) {
      float lastBrightness = masterBrightness;
      uint32_t pixel0 = strip.getPixelColor(0);
      uint32_t pixel1 = strip.getPixelColor(1);
      uint32_t pixel2 = strip.getPixelColor(2);
      uint32_t pixel3 = strip.getPixelColor(3);
      uint32_t pixel4 = strip.getPixelColor(4);
      uint32_t pixel5 = strip.getPixelColor(5);
      uint32_t pixel6 = strip.getPixelColor(6);
      uint32_t pixel7 = strip.getPixelColor(7);
      uint32_t pixel8 = strip.getPixelColor(8);

      char* oldMode = mode;
      mode = "MANUAL";
      int flashCount = com5;

      i = 0;
      while (i < 9) {
        strip.setPixelColor(i, 0, 0, 0);
        i++;
      }
      strip.show();
      masterBrightness = 1.00;
      delay(100);

      while (flashCount > 0) {
        i = 0;
        while (i < 9) {
          strip.setPixelColor(i, com2 * notifyBrightness, com3 * notifyBrightness, com4 * notifyBrightness);
          i++;
        }
        strip.show();
        delay(100);
        i = 0;
        while (i < 9) {
          strip.setPixelColor(i, 0, 0, 0);
          i++;
        }
        strip.show();
        delay(100);
        flashCount--;
      }

      masterBrightness = lastBrightness;

      if (oldMode != "MOOD") {
        strip.setPixelColor(0, pixel0);
        strip.setPixelColor(1, pixel1);
        strip.setPixelColor(2, pixel2);
        strip.setPixelColor(3, pixel3);
        strip.setPixelColor(4, pixel4);
        strip.setPixelColor(5, pixel5);
        strip.setPixelColor(6, pixel6);
        strip.setPixelColor(7, pixel7);
        strip.setPixelColor(8, pixel8);
        strip.show();
      }

      mode = oldMode;
    }
  }
  if (com1 == 9) {
    shutDown();
  }
  if (com1 == 10) {
    com2 = float(comString.substring(6).toInt());
    tickMultiplier = 1.0;
    tickTime = com2;
    debugWiFi("Tick time set to: ", false);
    debugWiFi(String(com2));
  }
  if (com1 == 11) {
    com2 = float(comString.substring(6, 9).toInt());
    sleepBrightness = com2 / 100.00;
    debugWiFi("Sleep brightness set to: ", false);
    debugWiFi(String(com2));
  }
  if (com1 == 12) {
    com2 = float(comString.substring(6, 9).toInt());
    notifyBrightness = com2 / 100.00;
    debugWiFi("Notify brightness set to: ", false);
    debugWiFi(String(com2));
  }
  if (com1 == 13) {
    int rVal = int(comString.substring(6,  9).toInt()); // rVal
    int gVal = int(comString.substring(9,  12).toInt());
    int bVal = int(comString.substring(12, 15).toInt());
    int rVal2 = int(comString.substring(15, 18).toInt());
    int gVal2 = int(comString.substring(18, 21).toInt());
    int bVal2 = int(comString.substring(21, 24).toInt());
    int flashCount = int(comString.substring(24, 27).toInt());

    if ((rVal % 2) != 0) {
      rVal--;
    }
    if ((gVal % 2) != 0) {
      gVal--;
    }
    if ((bVal % 2) != 0) {
      bVal--;
    }
    if ((rVal2 % 2) != 0) {
      rVal2--;
    }
    if ((gVal2 % 2) != 0) {
      gVal2--;
    }
    if ((bVal2 % 2) != 0) {
      bVal2--;
    }

    if (sleepState != 2) {
      float lastBrightness = masterBrightness;
      uint32_t pixel0 = strip.getPixelColor(0);
      uint32_t pixel1 = strip.getPixelColor(1);
      uint32_t pixel2 = strip.getPixelColor(2);
      uint32_t pixel3 = strip.getPixelColor(3);
      uint32_t pixel4 = strip.getPixelColor(4);
      uint32_t pixel5 = strip.getPixelColor(5);
      uint32_t pixel6 = strip.getPixelColor(6);
      uint32_t pixel7 = strip.getPixelColor(7);
      uint32_t pixel8 = strip.getPixelColor(8);

      char* oldMode = mode;
      mode = "MANUAL";

      masterBrightness = 1.00;

      while (flashCount > 0) {
        i = 0;
        while (i < 9) {
          strip.setPixelColor(i, 0, 0, 0);
          i++;
        }
        strip.show();
        delay(100);
        i = 0;
        while (i < 9) {
          strip.setPixelColor(i, rVal * notifyBrightness, gVal * notifyBrightness, bVal * notifyBrightness);
          i++;
        }
        strip.show();


        delay(100);
        flashCount--;
      }

      delay(250);

      byte colorFading = 1;
      while (colorFading == 1) {
        if (rVal < rVal2) {
          rVal += 2;
        }
        if (rVal > rVal2) {
          rVal -= 2;
        }
        if (gVal < gVal2) {
          gVal += 2;
        }
        if (gVal > gVal2) {
          gVal -= 2;
        }
        if (bVal < bVal2) {
          bVal += 2;
        }
        if (bVal > bVal2) {
          bVal -= 2;
        }

        i = 0;
        while (i < 9) {
          strip.setPixelColor(i, rVal * notifyBrightness, gVal * notifyBrightness, bVal * notifyBrightness);
          i++;
        }
        strip.show();

        if (rVal == rVal2 && gVal == gVal2 && bVal == bVal2) {
          colorFading = 0;
        }
      }

      masterBrightness = lastBrightness;

      if (oldMode != "MOOD") {
        strip.setPixelColor(0, pixel0);
        strip.setPixelColor(1, pixel1);
        strip.setPixelColor(2, pixel2);
        strip.setPixelColor(3, pixel3);
        strip.setPixelColor(4, pixel4);
        strip.setPixelColor(5, pixel5);
        strip.setPixelColor(6, pixel6);
        strip.setPixelColor(7, pixel7);
        strip.setPixelColor(8, pixel8);
        strip.show();
      }
      else {
        i = 0;
        while (i < 4) {
          maincolorR[i] = 255.00 / rVal;
          maincolorG[i] = 255.00 / gVal;
          maincolorB[i] = 255.00 / bVal;
          //maincolorRnext[i] = 255.00/rVal;
          //maincolorGnext[i] = 255.00/gVal;
          //maincolorBnext[i] = 255.00/bVal;
          mainbrightness[i] = 1.00;
          i++;
        }

        i = 0;
        while (i < 5) {
          seccolorR[i] = 255.00 / rVal;
          seccolorG[i] = 255.00 / gVal;
          seccolorB[i] = 255.00 / bVal;
          //seccolorRnext[i] = 255.00/rVal;
          //seccolorGnext[i] = 255.00/gVal;
          //seccolorBnext[i] = 255.00/bVal;
          secbrightness[i] = 1.00;
          i++;
        }
      }

      mode = oldMode;
    }
  }
  if (com1 == 14) {
    float interpolationSpeed = float(comString.substring(6).toInt()) / 100.00;
  }
}

void shutDown() {
  shutDownLamp = true;
}

void parseMood() {
  highest = 0;
  byte highestPlace = 0;
  i = 0;
  while (i < 6) {
    if (moodLevels[i] >= highest) {
      highest = moodLevels[i];
      highestPlace = i;
    }
    i++;
  }

  if (highest > 100) {
    secLevelNext = secLevelDefault - (highest - 100);
    secLevelNext = constrain(secLevelNext, 10, secLevelDefault);
    tickMultiplier = 1.0 / (highest * 1.35 / 100.00);
  }
  else {
    secLevelNext = secLevelDefault;
    tickMultiplier = 1.0;
  }

  i = 0;
  while (i < 4) {
    maincolorRnext[i] = moodColors[highestPlace][0];
    maincolorGnext[i] = moodColors[highestPlace][1];
    maincolorBnext[i] = moodColors[highestPlace][2];
    i++;
  }

  char* newMood = moodNames[highestPlace];
  if (newMood == "LOVE") {
    // JOY
    seccolorRnext[1] = moodColors[1][0];
    seccolorGnext[1] = moodColors[1][1];
    seccolorBnext[1] = moodColors[1][2];
    // SURPRISE
    seccolorRnext[2] = moodColors[2][0];
    seccolorGnext[2] = moodColors[2][1];
    seccolorBnext[2] = moodColors[2][2];
    // ANGER
    seccolorRnext[3] = moodColors[3][0];
    seccolorGnext[3] = moodColors[3][1];
    seccolorBnext[3] = moodColors[3][2];
    // SADNESS
    seccolorRnext[4] = moodColors[4][0];
    seccolorGnext[4] = moodColors[4][1];
    seccolorBnext[4] = moodColors[4][2];
    // FEAR
    seccolorRnext[5] = moodColors[5][0];
    seccolorGnext[5] = moodColors[5][1];
    seccolorBnext[5] = moodColors[5][2];
  }
  if (newMood == "JOY") {
    // LOVE
    seccolorRnext[1] = moodColors[0][0];
    seccolorGnext[1] = moodColors[0][1];
    seccolorBnext[1] = moodColors[0][2];
    // SURPRISE
    seccolorRnext[2] = moodColors[2][0];
    seccolorGnext[2] = moodColors[2][1];
    seccolorBnext[2] = moodColors[2][2];
    // ANGER
    seccolorRnext[3] = moodColors[3][0];
    seccolorGnext[3] = moodColors[3][1];
    seccolorBnext[3] = moodColors[3][2];
    // SADNESS
    seccolorRnext[4] = moodColors[4][0];
    seccolorGnext[4] = moodColors[4][1];
    seccolorBnext[4] = moodColors[4][2];
    // FEAR
    seccolorRnext[5] = moodColors[5][0];
    seccolorGnext[5] = moodColors[5][1];
    seccolorBnext[5] = moodColors[5][2];
  }
  if (newMood == "SURPRISE") {
    // LOVE
    seccolorRnext[1] = moodColors[0][0];
    seccolorGnext[1] = moodColors[0][1];
    seccolorBnext[1] = moodColors[0][2];
    // JOY
    seccolorRnext[2] = moodColors[1][0];
    seccolorGnext[2] = moodColors[1][1];
    seccolorBnext[2] = moodColors[1][2];
    // ANGER
    seccolorRnext[3] = moodColors[3][0];
    seccolorGnext[3] = moodColors[3][1];
    seccolorBnext[3] = moodColors[3][2];
    // SADNESS
    seccolorRnext[4] = moodColors[4][0];
    seccolorGnext[4] = moodColors[4][1];
    seccolorBnext[4] = moodColors[4][2];
    // FEAR
    seccolorRnext[5] = moodColors[5][0];
    seccolorGnext[5] = moodColors[5][1];
    seccolorBnext[5] = moodColors[5][2];
  }
  if (newMood == "ANGER") {
    // LOVE
    seccolorRnext[1] = moodColors[0][0];
    seccolorGnext[1] = moodColors[0][1];
    seccolorBnext[1] = moodColors[0][2];
    // JOY
    seccolorRnext[2] = moodColors[1][0];
    seccolorGnext[2] = moodColors[1][1];
    seccolorBnext[2] = moodColors[1][2];
    // SURPRISE
    seccolorRnext[3] = moodColors[2][0];
    seccolorGnext[3] = moodColors[2][1];
    seccolorBnext[3] = moodColors[2][2];
    // SADNESS
    seccolorRnext[4] = moodColors[4][0];
    seccolorGnext[4] = moodColors[4][1];
    seccolorBnext[4] = moodColors[4][2];
    // FEAR
    seccolorRnext[5] = moodColors[5][0];
    seccolorGnext[5] = moodColors[5][1];
    seccolorBnext[5] = moodColors[5][2];
  }
  if (newMood == "SADNESS") {
    // LOVE
    seccolorRnext[1] = moodColors[0][0];
    seccolorGnext[1] = moodColors[0][1];
    seccolorBnext[1] = moodColors[0][2];
    // JOY
    seccolorRnext[2] = moodColors[1][0];
    seccolorGnext[2] = moodColors[1][1];
    seccolorBnext[2] = moodColors[1][2];
    // SURPRISE
    seccolorRnext[3] = moodColors[2][0];
    seccolorGnext[3] = moodColors[2][1];
    seccolorBnext[3] = moodColors[2][2];
    // ANGER
    seccolorRnext[4] = moodColors[3][0];
    seccolorGnext[4] = moodColors[3][1];
    seccolorBnext[4] = moodColors[3][2];
    // FEAR
    seccolorRnext[5] = moodColors[5][0];
    seccolorGnext[5] = moodColors[5][1];
    seccolorBnext[5] = moodColors[5][2];
  }
  if (newMood == "FEAR") {
    // LOVE
    seccolorRnext[1] = moodColors[0][0];
    seccolorGnext[1] = moodColors[0][1];
    seccolorBnext[1] = moodColors[0][2];
    // JOY
    seccolorRnext[2] = moodColors[1][0];
    seccolorGnext[2] = moodColors[1][1];
    seccolorBnext[2] = moodColors[1][2];
    // SURPRISE
    seccolorRnext[3] = moodColors[2][0];
    seccolorGnext[3] = moodColors[2][1];
    seccolorBnext[3] = moodColors[2][2];
    // ANGER
    seccolorRnext[4] = moodColors[3][0];
    seccolorGnext[4] = moodColors[3][1];
    seccolorBnext[4] = moodColors[3][2];
    // SADNESS
    seccolorRnext[5] = moodColors[4][0];
    seccolorGnext[5] = moodColors[4][1];
    seccolorBnext[5] = moodColors[4][2];
  }
}

void debugVariables() {
#ifdef DEBUG
  if (millis() >= debugNextTick) {
    debugNextTick = millis() + debugInterval;
  }
#endif
}
