/* Gets the time of next SF Muni at a given stop and displays on a 7-segment display */
/*
*
* --Circuit--
*
*    a
*    _
* f |_| b   g = center seg
* e |_| c   . = dp
*    d
*
* Check the wiring for your 7-segment display.
* http://www.learningaboutelectronics.com/Articles/Arduino-7-segment-LED-display.php
*
* a->2, b->3, c->4, d->6, e->7, f->9, g->8, DP->5, 2*CC->GND
*
* Ethernet shield attached to pins 10, 11, 12, 13
*
* Various examples and code from David A. Mellis, Tom Igoe and Adrian McEwen
*/

#include <SPI.h>
#include <Ethernet.h>

*/ Config */

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,0,177);

// MAC address to use (check your shield for one)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };



/* End Config */

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "webservices.nextbus.com";    // name address for Nextbus (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

//pin config for digit
const int segmentPins[8]= { 5,8,9,7,6,4,3,2 };

//bits representing numerals 0-9 and some other stuff
const byte numeral[13]= {
B11111100, //0
B01100000, //1
B11011010, //2
B11110010, //3
B01100110, //4
B10110110, //5
B00111110, //6
B11100000, //7
B11111110, //8
B11100110, //9
B00000000, //shows nothing
B11101100, //N
B11101110, //A
};

void setup() {
 // Open //Serial communications and wait for port to open:
  //Serial.begin(9600);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  } else {
     //Serial.println(Ethernet.localIP()); 
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  startClient();
  
  //set up digit pins
  for (int i=0; i < 8; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }

  
}

int counter = 0;
char lookAhead[8] = {'m','i','n','u','t','e','s'};
int lapos = 0;
int grabTime = false;
int gtpos = 0;
int time;
String cache = String();
int times[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int tcount = 0;

void loop()
{
  // if there are incoming bytes available 
  // from the server, read them and print them:

  if (client.available()) {
    char c = client.read();
    ////Serial.print(c);
    
    if(lapos == 7) {
      lapos = 0;
      grabTime = true;
    } else if(c == lookAhead[lapos]) {
      /*
      //Serial.print("*lookAhead* ");
      //Serial.print("lapos= ");
      //Serial.print(lapos);
      //Serial.print(" char= ");
      //Serial.print(c);
      //Serial.print(" *lookAhead*\n");
      */
      lapos++;
    } else {
      lapos = 0; 
    }
    if (grabTime) {
      /*
      //Serial.print("*grabTime* ");
      //Serial.print(c);
      //Serial.print(" *grabTime*\n");
      */
      if(gtpos > 1) {
        if(c == '"') {
          times[tcount] = time;
          tcount++;
          gtpos = -1;
          grabTime = false;
          time = 0;
        } else {
          
          int exponent = gtpos - 2;
          int oldTime = time * (10^exponent);
          int newTime = int(c) - 48;
          time = oldTime + newTime;

        }
      } 
      gtpos++;
  
    }
  }


  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    //Serial.println("***MUNI TIMES***");
    for(int i=0;i<10;i++) {
      if(times[i] != -1) {
        //Serial.println(times[i]); 
      }
    }
    //Serial.println("***MUNI TIMES***");
    
    //set digit to next muni
    if(times[0] != -1 && times[0] < 10) {
      showDigit(times[0]);
      delay(60000);
    } else {
      notAvailable();
      delay(30000 - 1500);
      notAvailable();
      delay(30000 - 1500);
    }
    
    //Serial.println();
    //Serial.println("disconnecting.");
    client.stop();


    // clear time values, wait for a minute and check again
    for(int i=0;i<10;i++) {
      times[i] = -1;
    }
    tcount = 0;
    startClient();
  }
}

void showDigit (int number) {
  boolean isBitSet;
  for (int segment=1; segment < 8; segment++) {
    isBitSet= bitRead(numeral[number], segment);
    digitalWrite(segmentPins[segment], isBitSet);
  }
}

void notAvailable () {
    boolean isBitSet;
    for (int segment=1; segment < 8; segment++) {
      isBitSet= bitRead(numeral[11], segment);
      digitalWrite(segmentPins[segment], isBitSet);
    }
    delay(750);
    for (int segment=1; segment < 8; segment++) {
      isBitSet= bitRead(numeral[12], segment);
      digitalWrite(segmentPins[segment], isBitSet);
    }
    delay(750);
    
    for (int segment=1; segment < 8; segment++) {
      isBitSet= bitRead(numeral[10], segment);
      digitalWrite(segmentPins[segment], isBitSet);
    }
  
}

void startClient() {
  //Serial.println("connecting...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.println("GET /service/publicXMLFeed?command=predictions&a=sf-muni&r=J&dirTag=J__IB1&stopId=13998 HTTP/1.1");
    client.println("Host: webservices.nextbus.com");
    client.println("User-Agent: MuniBot/1.0");
    client.println("Connection: Close");
    client.println();
  } 
  else {
    // kf you didn't get a connection to the server:
    //Serial.println("connection failed");
  }
}
