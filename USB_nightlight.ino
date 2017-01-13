#include <Arduino.h>

#include <ESP8266WiFi.h>        // for connecting to web
#include "fauxmoESP.h"                  //for Wemo fakery

#include <WiFiClient.h>         //todo: can I remove this?
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <TimeLib.h>     // for time

#include "script_js.h"          // my scripts.
#include "root_html.h"          // the dash page
#include "wifi_creds.h"         // my wifi

#define SERIAL_OUTPUT     true       
#define HOST              "nightlight"

#define DEVICE_NAME       "Ted's Night Light"
#define DEVICE_DESC       "The IKEA lamp in Teddy's room"

#define ALEXA_NAME_NORMAL "Ted's night light"
#define ALEXA_DESC_NORMAL "Powers the light according to lux limit for the current fade or time of day"

#define ALEXA_NAME_ALTERNATE "Ted's full beam"
#define ALEXA_DESC_ALTERNATE "Powers the light at 100%, fading back to minimum after 30 mins"

#define EVENT_COUNT 5
#define MAX_BRIGHTNESS  1023 //the highest PWM value available  
#define MIN_BRIGHTNESS  35   //below this, steps are obvious.

struct device {
  bool powered;
  int brightness_current;
  bool locked;
  int fadePin = D2;
  int buttonPin = D6;
};
struct device thisDevice;

struct fade {
  bool active = false;
  unsigned long startTime = 0L;
  long duration = 1 ;
  int startBrightness = 0;
  int endBrightness = 0;
};
struct fade thisFade;

typedef struct {
  byte h;
  byte m;
  bool enacted;
  String label;
  int targetBrightness;
  int transitionDurationInSeconds;
  bool enabled;
} event;
event dailyEvents[EVENT_COUNT] = {
  { 7, 00, false, "wake", MAX_BRIGHTNESS, 15 * 60, true},  /* 0 */
  { 8, 30, false, "gone",              0,     1, true},    /* 1 */
  {19, 55, false, "bedtime", MAX_BRIGHTNESS,   4*60, true},   /* 2 */
  {20, 00, false, "drop-off", 35, 45 * 60, true}, /* 3 */
  {23, 00, false, "sleep", 1, 5, true}, /* 4 */
};                            // initialises my events to a reasonable set of defaults.

const int testAdjustment = 3; //use this to nudge the sync'd clock signal on by x seconds.

bool alexaInstructionPending = false;
bool alexaPowerState = false;
bool buttonActionPending = false;

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 0;     // GMT, supply -5 for EST.

ESP8266WebServer httpServer(80); // this is the web server used to expose the web interface.
ESP8266HTTPUpdateServer httpUpdater;  //this is the /update service
fauxmoESP fauxmo;
WiFiUDP Udp;                 // required for time server connection

unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
void padPrint(int n);


int lastDateFromNTP = 0; //used to compare, if its a new date, we can reset the events

void testFades() {
  thisDevice.powered = true;
  bool usingSimpleCycleDebugger = true;

  if (!usingSimpleCycleDebugger) {
    for (int i = 0; i < EVENT_COUNT; i++) {
      dailyEvents[i].h = hour();
      dailyEvents[i].m = minute() + i;
      dailyEvents[i].transitionDurationInSeconds = 30;
      if (SERIAL_OUTPUT) {
        Serial.printf("Setting ");
        Serial.print(dailyEvents[i].label);
        Serial.printf(" as %d duration, triggering at %d:%d", dailyEvents[i].transitionDurationInSeconds, dailyEvents[i].h, dailyEvents[i].m);
      }
      dailyEvents[1].transitionDurationInSeconds = 1;
    }

  } else {
    
    for (int i = 1023; i > 0; i--) {
      thisDevice.brightness_current = i;
      updateLEDBrightness();
      delay(2);
    }

    for (int i = 0; i < 1024; i++) {
      thisDevice.brightness_current = i;
      updateLEDBrightness();
      delay(2);
    }
  }

}

void callback(const char * device_name, bool state) {
  Serial.println("Alexa callback triggered");
  Serial.print("Device "); Serial.print(device_name);
  Serial.print(" state: ");
  if (state) {
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }

  String device_handle(device_name);

  if (device_handle == ALEXA_NAME_ALTERNATE) {
    thisFade.startTime = millis(); //getCurrentSecond();
    thisFade.duration = 60 * 30 * 1000;
    thisFade.startBrightness = MAX_BRIGHTNESS;
    thisFade.endBrightness = 1;
    thisFade.active = true;
    thisDevice.powered = true;
  }else{
    thisDevice.powered = state;
  }
}

bool buttonChangedState() {
  static bool lastValue;
  static unsigned long ignoreUntil = 0UL;

  bool buttonState = (digitalRead(thisDevice.buttonPin) == HIGH);
  if (buttonState != lastValue) {
    //check the debounce
    if (millis() > ignoreUntil) {
      lastValue = buttonState;
      ignoreUntil = millis() + 500UL;
      if (SERIAL_OUTPUT) Serial.println("Enacting, and setting debounce for half a sec");
      return true;
    } else {
      if (SERIAL_OUTPUT) Serial.println("Debouncing");
      return false;
    }

  } else {
    return false; //button hasn't changed.
  }
}

void padPrint(int n) {
  if (n < 10) {
    if (SERIAL_OUTPUT) Serial.print("0");
  }
  if (SERIAL_OUTPUT) Serial.print(n);
}

void resetDailyEvents() {
  if (SERIAL_OUTPUT) Serial.println("Resetting daily events");
  for (int i = 0; i < EVENT_COUNT; i++) {
    dailyEvents[i].enacted = false;
  }
}

void updateLEDBrightness() {
  static int lastBrightnessLevel = 0;

  if (thisFade.active) {
    //calculate new brightness level
    long timeElapsed = millis() - thisFade.startTime;
    float timeAsFraction = (timeElapsed / float(thisFade.duration));

    //now work out the difference between start and end brightness level.
    int fadeTotalTravel = thisFade.endBrightness - thisFade.startBrightness;
    float newBrightnessLevel = thisFade.startBrightness + (fadeTotalTravel * timeAsFraction);

    //now convert new brightness level to the operable range.
    thisDevice.brightness_current = (int) newBrightnessLevel;

    if (timeAsFraction >= 1) {
      if (SERIAL_OUTPUT) Serial.println("\nFade target reached. Turning off thisFade.active");
      thisDevice.brightness_current = thisFade.endBrightness;
      thisFade.active = false;
    } else {
      if (SERIAL_OUTPUT) Serial.printf("\nFading from %d to %d over %dms, currently %d (%f percent)", thisFade.startBrightness, thisFade.endBrightness, thisFade.duration, thisDevice.brightness_current, (timeAsFraction * 100));
    }
  }

  int powerLevel = (thisDevice.brightness_current * (thisDevice.powered ? 1 : 0));
  if (powerLevel != lastBrightnessLevel) {
    if (SERIAL_OUTPUT) Serial.printf(" Writing new LED Brightness as %d ", thisDevice.brightness_current);
    if (SERIAL_OUTPUT) Serial.printf("(powered equiv %d)\n", powerLevel);
    analogWrite(thisDevice.fadePin, powerLevel);
    analogWrite(BUILTIN_LED, MAX_BRIGHTNESS - powerLevel);
    lastBrightnessLevel = powerLevel;
  }
}
void checkEvents() {
  /* look through the daily events using time. if thye're not done, and the time is now, do it!  */
  if (SERIAL_OUTPUT) Serial.println("Checking events");

  //check for new day
  if (day() != lastDateFromNTP) {
    //its a new day
    resetDailyEvents();
    lastDateFromNTP = day();
  }

  for (int i = 0; i < EVENT_COUNT; i++) {
    if (dailyEvents[i].enacted) {
      if (SERIAL_OUTPUT) Serial.printf("Event %d enacted\n", i);  //no worries.
    } else {
      if (dailyEvents[i].h == hour() && dailyEvents[i].m == minute()) {
        //time to do this
        if (SERIAL_OUTPUT) {
          Serial.printf("Event %d ready to fire (", i);
          Serial.print(dailyEvents[i].label);
          Serial.print (")\n");
        }
        dailyEvents[i].enacted = true;
        if (!thisDevice.locked) {
          if (dailyEvents[i].enabled) {
            //todo: do something about the fade. Currently its ignored.
            //also, think about if the light gets turned back on... how to refade.
            thisFade.startTime = millis(); //getCurrentSecond();
            thisFade.duration = dailyEvents[i].transitionDurationInSeconds * 1000;
            thisFade.startBrightness = thisDevice.brightness_current;
            thisFade.endBrightness = dailyEvents[i].targetBrightness;
            thisFade.active = true;
          } else {
            if (SERIAL_OUTPUT) Serial.println("event was not enabled");
          }
        } else {
          if (SERIAL_OUTPUT) Serial.println("device locked");
        }
      }
    }
  }
}
void handleRoot() {
  if (SERIAL_OUTPUT) Serial.print(F("\nSending dashboard..."));
  httpServer.send_P ( 200, "text/html", ROOT_HTML);
  feedback(false);
  if (SERIAL_OUTPUT) Serial.println(F("...done."));
}

void setup() {
  thisDevice.powered = true;
  thisDevice.brightness_current = MIN_BRIGHTNESS;
  thisDevice.locked = false;

  ESP.eraseConfig();
  delay(1000);

  pinMode(thisDevice.fadePin, OUTPUT);
  pinMode(thisDevice.buttonPin, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);

  Serial.begin(115200);
  delay(250);

  Serial.println("");
  Serial.println(F("Alexa USB PWM Using NTP"));

  Serial.print(F("Serial output is "));
  Serial.print(F("Serial output is "));
  Serial.println(SERIAL_OUTPUT ? "ON" : "OFF");

  Serial.println();
  Serial.print("Connecting to WIFI: ");
  Serial.println(ssid);;
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    Serial.println("WiFi failed, retrying.");
  }

  Serial.println(WiFi.localIP());

  MDNS.begin(HOST);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  if (SERIAL_OUTPUT) Serial.println(F("    MDNS responder started as blind.local, accepting:"));

  if (SERIAL_OUTPUT) Serial.println(F("      root: the UI"));
  httpServer.on("/", handleRoot);

  httpServer.on("/set", handleSet);
 
  httpServer.on("/fade", handleFade);


  if (SERIAL_OUTPUT) Serial.println(F("      /script.js: returns script"));
  httpServer.on("/script.js", []() {
    if (SERIAL_OUTPUT) Serial.print(F("\nSending script"));
    httpServer.send_P ( 200, "text/plain", SCRIPT_JS);
    if (SERIAL_OUTPUT) Serial.println(F("...done."));

  });
  if (SERIAL_OUTPUT) Serial.println(F("      /status.json: returns summary values for features"));
  httpServer.on("/status.json", sendStatus);

  if (SERIAL_OUTPUT) Serial.println(F("      /config.json: returns setup info for features"));
  httpServer.on("/config.json", sendConfig);


  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready!\n  Open http://%s.local/update in your browser\n", HOST);

  Serial.printf("\nStarting Fauxmo as %s\n", DEVICE_NAME);
  fauxmo.addDevice(ALEXA_NAME_NORMAL);
  fauxmo.addDevice(ALEXA_NAME_ALTERNATE);
  fauxmo.onMessage(callback);

  Serial.println(F("\n    Starting UDP"));
  Udp.begin(localPort);

  Serial.print(F("    Local port: "));
  Serial.println(Udp.localPort());
  Serial.println(F("    Waiting for sync"));

  setSyncProvider(getNtpTime);
  setSyncInterval(60 * 10);

  if (SERIAL_OUTPUT) Serial.println(F("Entering Loop"));
  if (SERIAL_OUTPUT) {
    Serial.print(F("Signal Strength at "));
    Serial.println(WiFi.RSSI());
  }
}
void handleSet() {
  Serial.println(F(" set request received"));

  if(httpServer.hasArg("power")){
      int powered = httpServer.arg("power").toInt();
      Serial.print("Power is ["); Serial.print(powered); Serial.print("].");
      thisDevice.powered = (powered > 0); 
  }
  
  if(httpServer.hasArg("brightness")){
      int brightness = httpServer.arg("brightness").toInt();
      Serial.print("Brightness is ["); Serial.print(brightness); Serial.print("].");
      thisDevice.brightness_current = brightness;
      thisFade.active = false; //turn off any current fade, otherwise your change will be lost.
  }
  
  sendStatus();
}

void handleFade() {
  //eg ?start=1023&end=1&duration=1800
  Serial.println(F(" fade request received"));

  thisFade.startTime = millis(); 
  thisFade.duration = httpServer.arg("duration").toInt() * 1000;
  thisFade.startBrightness = httpServer.arg("start").toInt();
  thisFade.endBrightness = httpServer.arg("end").toInt();
  thisFade.active = true;
  
  thisDevice.powered = true;
  thisDevice.brightness_current = thisFade.startBrightness;
   
  sendStatus();
}


void sendConfig() {
  /* send json for current values */
  feedback(true);
  if (SERIAL_OUTPUT) Serial.println(F("\n/config values request received: "));

  String config = "{"
  + (String) "\"device_name\": \"" + DEVICE_NAME + "\","
  + (String) "\"device_description\": \"" + DEVICE_DESC + "\","
  + (String) "\"brightness_current\": \"" + thisDevice.brightness_current + "\","
  + (String) "\"power\": \"" + thisDevice.powered + "\","
  + (String) "\"local_ip\": \"" + WiFi.localIP().toString() + "\","
  + (String) "\"time_h\": \"" + hour() + "\","
  + (String) "\"time_m\": \"" + minute() + "\","
  + (String) "\"time_s\": \"" + second() + "\","
  
  + (String) "\"alexa\": [{"
  + (String) "\"name\": \"" + ALEXA_NAME_NORMAL + "\","
  + (String) "\"description\": \"" + ALEXA_DESC_NORMAL + "\""
  + (String) "},{"
  + (String) "\"name\": \"" + ALEXA_NAME_ALTERNATE + "\","
  + (String) "\"description\": \"" + ALEXA_DESC_ALTERNATE + "\""
  + (String) "}],"
  
  + (String) "\"events\": [";

  for (int i = 0; i < EVENT_COUNT; i++) {
    if(i>0){config += ",";}
    
    config += (String) "{\"hour\": \"" + dailyEvents[i].h + "\",";
    config += (String) "\"minute\": \"" + dailyEvents[i].m + "\",";
    config += (String) "\"caption\": \"" + dailyEvents[i].label + "\",";
    config += (String) "\"power\": \"" + dailyEvents[i].targetBrightness + "\",";
    config += (String) "\"duration\": \"" + dailyEvents[i].transitionDurationInSeconds + "\"";
    config += (String) "}";
  }
  
  config += (String) "]}";

  
  httpServer.send(200, "application/json", config);
  feedback(false);
  if (SERIAL_OUTPUT) Serial.println(F("...done."));
}

void sendStatus() {
  /* send json for current values */
  feedback(true);
  if (SERIAL_OUTPUT) Serial.println(F("\n/status values request received: "));
  String values = "{\"brightness_current\":\"" + (String) thisDevice.brightness_current + "\","
                  + "\"power\":\"" + (String) thisDevice.powered + "\""
                  + "}";

  httpServer.send(200, "application/json", values);
  feedback(false);
  if (SERIAL_OUTPUT) Serial.println(F("...done."));
}


void feedback(bool on) {
  digitalWrite(BUILTIN_LED, (on ? LOW : HIGH));
}

long getCurrentSecond() {
  long secondsSinceMidnight = (hour() * 60 * 60)
                              + (minute() * 60)
                              + second();
  return secondsSinceMidnight;
}

void loop() {
  static int prevMinute = 0;

  yield(); //let the ESP8266 do its background tasks.
  httpServer.handleClient();

  //connect wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    return;
  }


  if (buttonChangedState()) {
    Serial.println("TODO: Enact something on behalf of BUTTON STATE");
    /*  If its powered in the day, turn it on at max permissable brightness.
        If its powered on in the evening, turn it on at current fade level, or max allowed at that time
        Not sure of logic here... I guess:
          if a fade is active, just toggle the device.powered.
          if no fade is active, and its after 8am and before sleep, set power to 255 and toggle powered (unless it was already powered at PWM 0).
          if no fade active, and its after sleep, set power to MIN and toggle powered.
    */
    if (thisFade.active) {
      //just toggle the power
      thisDevice.powered = !thisDevice.powered;
      if (SERIAL_OUTPUT) Serial.println("Fade Active when button pushed. Toggling power.");
    } else {
      int currentMinuteOfDay = (hour() * 60) + minute();
      int dayStarts = (dailyEvents[1].h * 60) + dailyEvents[1].m;
      int dayEnds = (dailyEvents[3].h * 60) + dailyEvents[3].m;

      if (currentMinuteOfDay >= dayStarts && currentMinuteOfDay <= dayEnds) {
        if (thisDevice.powered && thisDevice.brightness_current == 0) {
          if (SERIAL_OUTPUT) Serial.println("Daytime, and device was powered at 0 when button pushed. Setting max brightness.");

          thisDevice.brightness_current = MAX_BRIGHTNESS;
        } else {
          if (SERIAL_OUTPUT) Serial.println("Day when button pushed. Setting max brightness and toggling power.");

          thisDevice.brightness_current = MAX_BRIGHTNESS;
          thisDevice.powered = !thisDevice.powered;
        }
      } else {
        //its night time
        if (SERIAL_OUTPUT) Serial.println("Night when bUtton pushed. Setting min brightness and toggling power.");

        thisDevice.brightness_current = MIN_BRIGHTNESS;
        thisDevice.powered = !thisDevice.powered;
      }
    }

    buttonActionPending = false;
  }

  if (timeStatus() == timeNotSet) {
    //try to get the time?
  } else {
    if (minute() != prevMinute) { //if the hour/minute have change
      prevMinute = minute();     //store these for the next comparison
      checkEvents();
    }
  }

  updateLEDBrightness(); //doesn't really need to happen.
  delay(10);
}


/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {

  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  if (SERIAL_OUTPUT) Serial.print(F("    Transmit NTP Request\n      "));
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  if (SERIAL_OUTPUT) Serial.print(ntpServerName);
  if (SERIAL_OUTPUT) Serial.print(": ");
  if (SERIAL_OUTPUT) Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      if (SERIAL_OUTPUT) Serial.println(F("      Receive NTP Response"));
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];

      //adjust for DST
      int dstOffset = 0;
      time_t t = secsSince1900 - 2208988800UL + (timeZone) * SECS_PER_HOUR;
      bool dstActive = isDst(day(t), month(t), weekday(t));

      if (dstActive) {
        if (SERIAL_OUTPUT) Serial.println(F("    Adjusting for DST"));
        dstOffset = -1;
      }
      return testAdjustment + secsSince1900 - 2208988800UL + (timeZone - dstOffset) * SECS_PER_HOUR;

    }
  }
  if (SERIAL_OUTPUT) Serial.println(F("    No NTP Response :-("));
  //warn using the built in led;
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUILTIN_LED, LOW);
    delay(250);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(250);
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

bool isDst(int day, int month, int dow) {
  /* Purpose: works out whether or not we are in UK daylight savings time
              using day, month and dayOfWeek
     Expects: day (1-31), month(0-11), day of week (0-6)
     Returns: boolean true if DST
  */

  //do the big rules first.
  if (month < 3 || month > 10)  return false;
  if (month > 3 && month < 10)  return true;

  //now the picky ones.
  int previousSunday = day - dow;
  if (month == 3) return previousSunday >= 25;
  if (month == 10) return previousSunday < 25;

  return false; // something went wrong.
}

