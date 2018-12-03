#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>

// Auth Token in the Blynk App.
char auth[] = "d8bfff1ed6cb40ab8197ffa5ac07c6bb";

#define BLYNK_PRINT Serial
#define W5100_CS  10   //Ethernet board CS pin
#define SDCARD_CS 4    //witch SDCard  CS pin

#define DHTPIN 2          // Hőmérő pin

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

#define DHTTYPE DHT21     // AM2301
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "P5JTDZFCHQTUQDL8";    // Write API Key for a ThingSpeak Channel

EthernetClient client;

void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  updateBlynk(t,h);
  updateThingSpeak("&field1="+String(t)+"&field2="+String(h));
  
}

void setup()
{
  // Soros port beállítása
  Serial.begin(115200);

  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH); // Deselect the SD card
  Serial.print(F("Starting ethernet..."));
  
  //Ip címet kér a DHCP szervertől
  if(!Ethernet.begin(mac)) 
    Serial.println(F("DHCP request failed"));
  else 
    Serial.println(Ethernet.localIP());

  Blynk.config(auth);  //beállítja a key-t
  
  dht.begin();

  // Setup a function to be called every 10 second
  timer.setInterval(10000L, sendSensor);
}

void loop()
{
  timer.run();
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
    {Serial.println("Connecting to ThingSpeak…");
    Serial.println(tsData);
    client.stop();}
  else
    {Serial.println("Connection to ThingSpeak Failed");
    Serial.println(); }}
else
  {Serial.println("Connection to ThingSpeak Failed");
  Serial.println();}
}



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

