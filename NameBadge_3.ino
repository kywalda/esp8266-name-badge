/************************************************************************************************
 * Versuch, einen Text übers Web einzugeben, abzuspeichern und auf ein LCD-Display auszugeben...
 * 2017-05-10 Arne Groh agroh@uya.de
 * 2017-05-18 ueber Timer gedimmte LEDs hinzugefuegt
 * 2017-05-20 etwas universeller (bisher nur mit 2 zeiligen Display getestet)
*************************************************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <DNSServer.h>
#include <user_interface.h>

#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

//************ Values to change ***************
const int cols = 20;
const int rows = 2;

const int ScrollDelay = 700; // milliseconds between char shift
const int LedDelay = 50; // Timer1 ticks in msecs | time between LED fade steps (look below)

const int val[] = {  // PWM values | 0 -> OFF | 1023 -> allways ON
1, 1, 1, 1,  
1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10,
11, 12, 13, 15, 17, 19, 21, 23, 26, 29, 32, 36, 40, 44, 49, 55,
61, 68, 76, 85, 94, 105, 117, 131, 146, 162, 181, 202, 225, 250,
279, 311, 346, 386, 430, 479, 534, 595, 663, 739, 824, 918, 1023,
1023, 1023, 1023
};
//********************************************

int NumVal = NUMITEMS (val);

int steps1 = 1;
int phase1 = 1;
int steps2 = steps1 + (NumVal / 2);
int phase2 = 1;
int steps3 = NumVal - 2;
int phase3 = -1;
int steps4 = steps3 - (NumVal / 2);
int phase4 = -1;

os_timer_t Timer1;         //make Timer 

//LiquidCrystal lcd(RS, E, data4, data5, data6, data7); 
LiquidCrystal lcd(D0, D1, D2, D3, D4, D5); // esp8266-nodemcu pins

int stringStart, stringStop = 0;
int scrollCursor = cols;
int MsgLength = 0;
int i = 0;

String line[rows];

const char *ssid = "VirenTestNetz";
const byte DNS_PORT = 53;
DNSServer dnsServer;

const String form = // this is the captive portal formular
  "<p>"
  "<center>"
  "<h1>You are infected</h1>"
  "<form action='msg'><p>Enter your PIN-Code <input type='text' name='msg' size=50 autofocus> <input type='submit' value='Submit'></form>"
  "</center>";

ESP8266WebServer server(80);

void handle_msg() 
{
  server.send(200, "text/html", form);  // Send same page so they can send another msg

  String msg = server.arg("msg");

  String decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client browser
  decodedMsg.replace("+", " ");
  decodedMsg.replace("%21", "!");
  decodedMsg.replace("%22", "");
  decodedMsg.replace("%23", "#");
  decodedMsg.replace("%24", "$");
  decodedMsg.replace("%25", "%");
  decodedMsg.replace("%26", "&");
  decodedMsg.replace("%27", "'");
  decodedMsg.replace("%28", "(");
  decodedMsg.replace("%29", ")");
  decodedMsg.replace("%2A", "*");
  decodedMsg.replace("%2B", "+");
  decodedMsg.replace("%2C", ",");
  decodedMsg.replace("%2F", "/");
  decodedMsg.replace("%3A", ":");
  decodedMsg.replace("%3B", ";");
  decodedMsg.replace("%3C", "<");
  decodedMsg.replace("%3D", "=");
  decodedMsg.replace("%3E", ">");
  decodedMsg.replace("%3F", "?");
  decodedMsg.replace("%40", "@");

  Serial.println(decodedMsg);                           // print original string to serial
  Serial.println(' ');                                  // print new line to serial

  while (decodedMsg.length() < (cols * rows)) {
  decodedMsg = decodedMsg + " ";
  }

  Serial.println("writing message to eeprom :");
  Serial.print("\"");
  MsgLength = decodedMsg.length();
  EEPROM.write(0, MsgLength); // first write length of string
  for (i = 1; i <= MsgLength; ++i) // then write string
       {
        EEPROM.write(i, decodedMsg[i - 1]);
        Serial.print(decodedMsg[i - 1]); 
       }
  EEPROM.commit();
  Serial.println("\" written");
  line[rows -1] = decodedMsg;

}

void timerCallback(void *pArg)
{
  if ((steps1 == 0) || (steps1 == NumVal)) {phase1 = phase1 * -1;}
  analogWrite(D7, (val[steps1]));
  steps1 = steps1 + phase1;

  if ((steps2 == 0) || (steps2 == NumVal)) {phase2 = phase2 * -1;}
  analogWrite(D8, (val[steps2]));
  steps2 = steps2 + phase2;

  if ((steps3 == 0) || (steps3 == NumVal)) {phase3 = phase3 * -1;}
  analogWrite(D9, (val[steps3]));
  steps3 = steps3 + phase3;

  if ((steps4 == 0) || (steps4 == NumVal)) {phase4 = phase4 * -1;}
  analogWrite(D10, (val[steps4]));
  steps4 = steps4 + phase4;    
} 

void setup() {
  Serial.begin(115200);
  lcd.begin(cols,rows);
    for (i = 0; i < (rows -1); i++){      // each line
        while (line[i].length() < cols) { // fill up
      	line[i] = line[i] + " ";          // with blanks
        }
    }
  while (line[(rows - 1)].length() < (cols * rows)) {  // last line stores the complete message (
  	line[(rows - 1)] = line[(rows - 1)] + " ";
    }

  EEPROM.begin(512);
  delay(100); 
  Serial.println();
  Serial.println("Reading EEPROM");
  String message;
  MsgLength = EEPROM.read(0);	          // first read length of following string
  for (i = 1; i <= MsgLength; ++i) {    // read   
  	message += char(EEPROM.read(i));    // the string char by char
    } 
  Serial.println(message);
  line[rows -1] = message; // put message in the last line
 
  IPAddress ip(1,1,1,1);
  IPAddress nm(255,255,255,0);  
  WiFi.softAPConfig(ip, ip, nm);
  dnsServer.start(DNS_PORT, "*", ip);

  Serial.print("Configuring access point... ");
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

// Set up the HTTP server
  server.on("/", []()
  {
    server.send(200, "text/html", form);
  });
  server.on("/msg", handle_msg);  // on incoming "msg" call this functions
  server.begin();                 // Start web server
  Serial.println("HTTP server started");

// Setup LEDs and Timer
  NumVal--;
  
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(D9, OUTPUT);
  pinMode(D10, OUTPUT);
  
  os_timer_setfn(&Timer1, timerCallback, &phase1); // on Timer1 call function timerCallback with the arg phase1 (not needed here)
  os_timer_arm(&Timer1, LedDelay, true);  
}



void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  for (i = 0; i < rows; i++){
    lcd.setCursor(scrollCursor, i);
    lcd.print(line[i].substring(stringStart,stringStop));
    }
  delay(ScrollDelay);
  
  if(stringStart == 0 && scrollCursor > 0){ // the cursor position is decreased while the string is increased (fill up rightest position)
  	scrollCursor--;
  	stringStop++;
  } else if (scrollCursor == 0) {  // wenn der Cursor den linken Rand erreicht, links einen Char wegschneiden und rechts ankleben
    for (i = 0; i < (rows - 1); i++) {
      line[i] = line[i].substring(1,line[i].length()) + line[i + 1].substring(0,1);
      }      
    line[rows -1] = line[rows -1].substring(1,line[rows - 1].length()) + line[rows - 1].substring(0,1);

  } else { // when string leaves the rightest position. String is shifted left.
  	stringStart++;
  	stringStop++;
  }
}

