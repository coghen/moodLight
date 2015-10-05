#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
const int bufSize = 64;
int i = 0;
int address = 0;
byte value;

char charBuf[bufSize];
char ssidReadBuf[64];
char passReadBuf[64];

String wifiIN = "XXXX";
char wifiCreds[bufSize];

ESP8266WebServer server(80);

String wifiSent = "";
String ssidSelf = "";
int wifiSolved = 0;

const char* host = "www.moodlighting.co";
String serverIP = "x";
char serverBuf[20];

long tickNow = 0;
long nextRun = 0;
int interval = 0 * 1000;
int sendTimes = 10;
String macAddr = "XX:XX:XX:XX:XX:XX";

int updateInterval = 1000;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

unsigned int localPort = 2390;      // local port to listen for UDP packets
const int PACKET_SIZE = 32;
char packetBuffer[PACKET_SIZE]; //buffer to hold incoming and outgoing packets
unsigned int MOODtime = 0;

String outString = "";
bool serialDone = false;

byte ip1 = 0;
byte ip2 = 0;
byte ip3 = 0;
byte ip4 = 0;

void readWIFIfromEEPROM() {
  int address = 0;
  int readStart = millis();
  EEPROM.begin(512);
  int ssidSolved = 0;
  while (ssidSolved == 0) {
    if ((millis() - readStart) > 2000) {
      ssidSolved = 2;
    }
    // read a byte from the current address of the EEPROM
    int value = EEPROM.read(address);

    charBuf[address] = value;

    if (charBuf[address] == '&') {
      ssidSolved = 1;
    }
    else {
      ssidReadBuf[address] = value;
    }


    // advance to the next address of the EEPROM
    address = address + 1;

    // there are only 512 bytes of EEPROM, from 0 to 511, so if we're
    // on address 512, wrap around to address 0
    if (address == 128) {
      address = 0;
    }

  }

  readStart = millis();
  int offset = address;
  int passSolved = 0;
  while (passSolved == 0) {
    if ((millis() - readStart) > 2000) {
      passSolved = 2;
    }
    // read a byte from the current address of the EEPROM
    int value = EEPROM.read(address);

    charBuf[address] = value;

    if (charBuf[address] == '&') {
      passSolved = 1;
    }
    else {
      passReadBuf[address - offset] = value;
    }

    // advance to the next address of the EEPROM
    address = address + 1;

    // there are only 512 bytes of EEPROM, from 0 to 511, so if we're
    // on address 512, wrap around to address 0
    if (address == 512) {
      address = 0;
    }

  }
  address = 0;
}

void writeWIFItoEEPROM() {
  wifiIN.toCharArray(wifiCreds, bufSize);
  EEPROM.begin(512);
  delay(1);

  i = 0;
  while (i < bufSize) {
    EEPROM.write(i, wifiCreds[i]);
    delay(1);
    i++;
  }
  EEPROM.commit();
}

void solveServer() {
  int serverSolved = 0;
  while (serverSolved == 0) {
    Serial.println("SERVER");
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("BAIL");
      solveServer();
      return;
    }

    // We now create a URI for the request
    String url = "/getServer.php";

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    delay(250);

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.println(line);
      if (line.substring(1, 2) == "%") {
        sendTimes = 10;
        while (sendTimes > 0) {
          Serial.println("*");
          sendTimes--;
          delay(10 );
        }
        serverIP = line.substring(2);
        serverIP.toCharArray(serverBuf, 20);
        serverSolved = 1;
      }
    }
    client.flush();
  }
}

void debugWiFi(String input, bool newline = true) {
  udp.beginPacket({ip1, ip2, ip3, ip4}, 2390);
  udp.write("ESP8266:  ");
  udp.print(input);
  if (newline == true) {
    udp.write("\n");
  }
  udp.endPacket();
}

void setup(void) {
  Serial.begin(38400);
  readWIFIfromEEPROM();

  delay(3000);

  sendTimes = 10;
  while (sendTimes > 0) {
    Serial.println("#");
    sendTimes--;
    delay(10);
  }

  String ssidSelf = "moodLighting-";
  ssidSelf += String(ESP.getChipId()) + "_" + String(ESP.getFlashChipId());
  char ssidBuf[30];
  ssidSelf.toCharArray(ssidBuf, 30);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidReadBuf, passReadBuf);
  int connStart = millis();
  int wifiSuccess = 0;
  while (wifiSuccess == 0) {
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - connStart > 10000) {
        wifiSuccess = 2;
      }
    }
    else {
      sendTimes = 10;
      while (sendTimes > 0) {
        Serial.println("$");
        sendTimes--;
        delay(10);
      }
      wifiSuccess = 1;
    }
    delay(500);
  }

  if (wifiSuccess == 2) {
    sendTimes = 10;
    while (sendTimes > 0) {
      Serial.println("!");
      sendTimes--;
      delay(10);
    }

    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");

    String mac1 = String(mac[0], HEX);
    mac1.toUpperCase();
    String mac2 = String(mac[1], HEX);
    mac2.toUpperCase();
    String mac3 = String(mac[2], HEX);
    mac3.toUpperCase();
    String mac4 = String(mac[3], HEX);
    mac4.toUpperCase();
    String mac5 = String(mac[4], HEX);
    mac5.toUpperCase();
    String mac6 = String(mac[5], HEX);
    mac6.toUpperCase();

    macAddr = mac1 + ":" + mac2 + ":" + mac3 + ":" + mac4 + ":" + mac5 + ":" + mac6;
    Serial.println(macAddr);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidBuf);

    delay(3000);

    server.on("/", handleRoot);

    server.onNotFound(handleNotFound);

    server.begin();

    wifiSolved = 0;
    while (wifiSolved == 0) {
      server.handleClient();
    }
  }
  solveServer();
  delay(5);
  udp.begin(localPort);
}

void handleRoot() {
  String welcomeHTML = "<html>"
                       "<head>"
                       "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                       "</head>"
                       "<body style='background-color:#111111;color:#cccccc;font-family:courier;font-size:16px;margin:0px;'>";

  String welcomeTOP = "<div id='top' style='padding:20px;background-color:#242424;margin-bottom:0px;'>"
                      "<strong style='font-size:22px;'>Hello from MoodLight!</strong>"
                      "</div>";

  String welcomeMID = "<div id='mid' style='padding:20px;margin-bottom:0px;'>"
                      "I am now hosting this webpage and <span style='color:#9988ff;'>pulsing purple</span>,"
                      "which <font style='color:#3399FF;'>sadly</font> means that I don't know what your WIFI info is! Would you be <font style='color:#fbdb00;'>happy</font> to tell me?<br><br>"
                      "<span style='color:#999999;'>Enter your <strong>SSID</strong>:</span><input style='margin-top:10px;margin-bottom:10px;border:none;height:50px;width:100%;color:#cccccc;background-color:#080808;' type='text' id='ssid'/><br>"
                      "<span style='color:#999999;'>Enter your <strong>PASSWORD</strong>:</span><input style='margin-top:10px;margin-bottom:10px;border:none;height:50px;width:100%;color:#cccccc;background-color:#080808;' type='password' id='pass'/><br>"
                      "<button style='color:#cccccc;border:none;background-color:#333333;height:50px;width:100%' onclick='sendInfo();'>SUBMIT</button><br><br></div>";

  String welcomeBOTTOM = "<div id='bottom' style='padding:20px;background-color:#242424;margin-bottom:0px;text-align:center;'>"
                         "My MAC Address is<br><strong>" + macAddr + "</strong><br>in case you need it.<br><br>"
                         "<em>This information may appear to you as plain text, but it's sent over a WPA2PSK encrypted network that this moodLight lamp is hosting.</em></div>"
                         "<script type='text/javascript'>"
                         "function sendInfo(){"
                         "var ssidValue = document.getElementById('ssid').value;"
                         "var passValue = document.getElementById('pass').value;"
                         "window.location.href = '/wifi='+ssidValue+'&'+passValue+'&';"
                         "}"
                         "</script></body></html>";

  String output = welcomeHTML + welcomeTOP + welcomeMID + welcomeBOTTOM;
  server.send(200, "text/html", output);
}

void handleNotFound() {
  String message = "";
  String sentType = server.uri().substring(1, 5);
  String sentInfo = server.uri().substring(6);

  if (sentType == "wifi") {

    String thanksHTML = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head><body style='background-color:#111111;color:#cccccc;font-family:courier;font-size:16px;margin:0px;'>";
    String thanksTOP = "<div id='top' style='padding:20px;background-color:#242424;margin-bottom:0px;'><strong style='font-size:22px;'>Thanks for the tip!</strong></div>";
    String thanksMID = "<div id='mid' style='padding:20px;margin-bottom:0px;'>I will now reboot and connect to the moodLighting service using the network credentials provided.</div>";
    String thanksBOTTOM = "<div id='bottom' style='padding:20px;background-color:#242424;margin-bottom:0px;text-align:center;'>My MAC Address is<br><strong>" + macAddr + "</strong><br>in case you need it.<br><br><em>This information may appear to you as plain text, but it's sent over a WPA2PSK encrypted network that this moodLight lamp is hosting.</em></div></body></html>";
    String good = thanksHTML + thanksTOP + thanksMID + thanksBOTTOM;

    wifiSent = sentInfo;
    server.send(200, "text/html", good);
    wifiIN = wifiSent;
    writeWIFItoEEPROM();
    delay(3000);
    ESP.restart();
  }
  else {
    server.send(500, "text/plain", "I don't understand that command. Check your formatting?");
  }
}


void loop(void) {
  checkSerial();

  if (digitalRead(2) == LOW) {
    wifiIN = "DEFAULT&DEFAULT&";
    writeWIFItoEEPROM();
    delay(120000);
  }
  if (millis() - MOODtime > updateInterval) {
    debugWiFi("Getting mood... ", false);
    getMood();
  }

  getUDP();
}

void checkSerial() {
  if (serialDone == true) {
    serialDone = false;
    udp.beginPacket({ip1, ip2, ip3, ip4}, 2390);
    udp.write("ATMEGA:  ");
    udp.print(outString);
    if (outString.substring(0, 3) == "201") {
      if (outString.substring(3, 6) == "999") {
        udp.beginPacket({ip1, ip2, ip3, ip4}, 2390);
        udp.write("WIFI RESET! -------!");
        wifiIN = "DEFAULT&DEFAULT&";
        writeWIFItoEEPROM();
        Serial.println("%101009");
        ESP.restart();
      }
    }
    outString = "";
    udp.endPacket();
  }

  // put your main code here, to run repeatedly:
  while (Serial.available() > 0) {
    char c = char(Serial.read());
    if (c == '@' or c == '%') {
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

void getMood() {
  MOODtime = millis();
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;

  if (!client.connect(serverBuf, httpPort)) {
    return;
  }

  // We now create a URI for the request
  String url = "/mood/check.php?getMood=" + String(ESP.getChipId()) + "_" + String(ESP.getFlashChipId());

  // This will send the request to the server
  client.print("GET " + url + " HTTP/1.1\r\n" +
               "Host: " + serverBuf + "\r\n" +
               "Connection: keep-alive\r\n\r\n");
  delay(5);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line.substring(1, 2) == "%") {
      Serial.print(line);
    }
  }
}

void getUDP() {
  int cb = udp.parsePacket();
  if (!cb) {
  }
  else {
    MOODtime = millis();
    // We've received a packet, read the data from it
    udp.read(packetBuffer, PACKET_SIZE); // read the packet into the buffer
    outString = String(packetBuffer);
    if (outString.substring(1, 4) == "201") {
      if (outString.substring(4, 7) == "001") {
        ip1 = outString.substring(7, 10).toInt();
        ip2 = outString.substring(10, 13).toInt();
        ip3 = outString.substring(13, 16).toInt();
        ip4 = outString.substring(16, 19).toInt();
      }
      if (outString.substring(4, 7) == "002") {
        updateInterval = outString.substring(7, 10).toInt() * 1000;
      }

      outString = "";
    }

    else {
      Serial.println(packetBuffer);
    }
  }



}


