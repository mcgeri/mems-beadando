#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SimpleTimer.h>
#include <EEPROM.h>
//#include <BlynkSimpleEthernet.h> //Blynk


#define W5100_CS  10   //Ethernet board CS pin
#define SDCARD_CS 4    //witch SDCard  CS pin

#define DHTPIN 2          // Hőmérő pin

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Auth Token in the Blynk App.
char auth[] = "d8bfff1ed6cb40ab8197ffa5ac07c6bb";


#define DHTTYPE DHT21     // AM2301
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301
unsigned long time;
int updateqty;
float h;
float t;
String readString = String(100); 
int refrate;
int timerID;
// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "P5JTDZFCHQTUQDL8";    // Write API Key for a ThingSpeak Channel

DHT dht(DHTPIN, DHTTYPE);
SimpleTimer timer;
EthernetClient client;
EthernetServer server(80); 

void sendSensor()
{
  Serial.println("Update temp/hum...");
  updateqty += 1;
  
  h = dht.readHumidity();
  t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  else
  { Serial.println("Temp:" + String(t) + "  Hum:" + String(h));}
  //updateBlynk(t,h);  //Blynk
  updateThingSpeak("&field1="+String(t)+"&field2="+String(h));
  
}

void setup()
{
  updateqty=0;
  refrate = EEPROM.read(0)*256;
  refrate = refrate+EEPROM.read(1);
  refrate=60;
  // Soros port beállítása
  Serial.begin(115200);
  server.begin();
  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH); // Deselect the SD card
  Serial.print(F("Refresh rate:"));
  Serial.println(refrate);
  Serial.print(F("Starting ethernet..."));
  
  //Ip címet kér a DHCP szervertől
  if(!Ethernet.begin(mac)) 
    Serial.println(F("DHCP request failed"));
  else 
    Serial.println(Ethernet.localIP());

  //Blynk.config(auth);  //Blynk beállítja a key-t
  
  dht.begin();
  sendSensor();
  timerID=timer.setInterval((long)refrate*1000, sendSensor);
 
}

void loop()
{
  timer.run();
    // listen for incoming clients
  client = server.available();
  if (client) {
  //  Serial.println("new client");
    
    boolean gotPost = false;
    time = millis();
    while (client.connected()) {
      unsigned long currentMillis = millis();
      if ((unsigned long)(currentMillis - time) >= 1000) {
        //Serial.println(readString);
        break;
      }
      if (client.available()) {
        time = millis();
        char c = client.read();
        //Serial.write(c);
        if (c == '\n' or readString.length()>90)
         {
         //Serial.println(readString);
         if (readString.indexOf("POST /")>=0)
            {
              gotPost = true;
            }
         readString="";
         }
        else
         {readString += c; }
      }
    }
    // close the connection:
     if (gotPost==true) {checkPost();}
     SendPage();
     client.stop();
//     Serial.println("client disconnected");
  }
}

void updateThingSpeak(String tsData)
{
if (client.connect(thingSpeakAddress, 80))
  {client.print("POST /update HTTP/1.1\n");
  client.print("Host: api.thingspeak.com\n");
  client.print("Connection: close\n");
  client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(tsData.length());
  client.print("\n\n");
  client.print(tsData);

  if (client.connected())
    {Serial.println("Adatkuldes a ThingSpeaknek");
    Serial.println(tsData);
    client.stop();}
  else
    {Serial.println("Connection to ThingSpeak Failed");
    Serial.println(); }}
else
  {Serial.println("Connection to ThingSpeak Failed");
  Serial.println();}
}



void checkPost()
{
            if (readString.indexOf("refrate=")>=0)
              {
                readString =readString.substring(readString.indexOf("refrate=")+8);
                refrate=readString.toInt();
                if (refrate==0)
                  {refrate=60;}
                if (refrate>=0 and refrate<10)
                  {refrate=10;}
                Serial.print("New refresh rate:");Serial.println(refrate);                 
                EEPROM.write(0,refrate / 256);
                EEPROM.write(1,refrate % 256);
                timer.deleteTimer(timerID);
                timerID = timer.setInterval((long)refrate*1000, sendSensor); //  new interval
              }
              }

void SendPage()
    {     Serial.println("Send Page");
          // send a response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  
          client.println("Refresh: " + String(refrate));   
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<form action='' method='post'>");
          
          client.print("Temperature: ");client.print(t);
          client.println("<br />");
          client.print("Humidity: ");client.print(h);
          client.println("<br />");
          client.print("Updated Qty: ");client.print(updateqty);
          client.println("<br />");
          client.print("Refresh rate (sec) min=10:");
          client.println("<input type='number' name='refrate' value='"+String(refrate) +"'>");          
          client.print("<input type='submit' value='Update'>");
          client.println("<br />");
          client.println("</form>");
          client.println("</html>");
       }



/*
void updateBlynk(float temp, float hum)
{
if (Blynk.connect())
  {  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V0, temp);
  
  if (Blynk.connected())
    {Serial.println("Connecting to Blynk…");
    Blynk.disconnect();}
  else
    {Serial.println("Connection to Blynk failed" );
    Serial.println(); }}
else
  { Serial.println("Connection to Blynk Failed ");
  Serial.println();}
}

*/       
