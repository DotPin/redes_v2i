//------LIBRERIAS-------

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include "WifiLocation.h"


#define GOOGLE_KEY "AIzaSyDP4SOZgPpJCEYormxf53cA9tnFIPoxArk" // Clave API Google Geolocation
#define SSID "redes_los_mejores" // SSID de tu red WiFi
#define PASSWD "" // Clave de tu red WiFi
#define HOSTFIREBASE "probando-nodemcu.firebaseio.com" // Host o url de Firebase
#define LOC_PRECISION 7 // Precision de latitud y longitud
// Llamada a la API de Google
WifiLocation location(GOOGLE_KEY);
location_t loc; // Estructura de datos que devuelve la librera WifiLocation


//-------Variables-------
byte mac[6];
String macStr = "";
String nombreComun = "NodeMCU";
String carld = "CAQR";
int timestamp = 1;
int fuelLevel = 2;
int km = 1;
int aceleration = 0;
int rpm = 30;
int kmh = 50;
int lane = 1;
int failureCode = 1;
boolean airBagsActivated = false;

// Cliente WiFi
WiFiClientSecure client;

//Your UTC Time Zone Differance  India +5:30
char HH = -3;
char MM = 0;

IPAddress timeServerIP; // time.nist.gov NTP server address

const char* ntpServerName = "ntp.shoa.cl";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

//=======================================================================
//                     SETUP
//=======================================================================
void setup() {
  Serial.begin(115200);
  // Conexion con la red WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(SSID);
    // Connect to WPA/WPA2 network:
    WiFi.begin(SSID, PASSWD);
    // wait 5 seconds for connection:
    delay(5000);
    Serial.print("Status = ");
    Serial.println(WiFi.status());
  }
  // Obtenemos la MAC como cadena de texto
  macStr = obtenerMac();
  Serial.print("MAC NodeMCU: ");
  Serial.println(macStr);
}

unsigned long sendNTPpacket(IPAddress& address){
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

//=======================================================================
//                        LOOP
//=======================================================================
void loop() {
  char hours, minutes, seconds;
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    minutes = ((epoch % 3600) / 60);
    minutes = minutes + MM; //Add UTC Time Zone
    
    hours = (epoch  % 86400L) / 3600;    
    if(minutes > 59)
    {      
      hours = hours + HH + 1; //Add UTC Time Zone  
      minutes = minutes - 60;
    }
    else
    {
      hours = hours + HH;
    }
    
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print(hours,DEC); // print the hour (86400 equals secs per day)
    Serial.print(':');
    
    if ( minutes < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }    
    Serial.print(minutes,DEC); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    
    seconds = (epoch % 60);
    if ( seconds < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(seconds,DEC); // print the second
  }
  // wait ten seconds before asking for the time again
  //delay(10000);

  // Obtenemos la geolocalizacion WiFi
  loc = location.getGeoFromWiFi();
  // Mostramos la informacion en el monitor serie
  Serial.println("Location request data");
  Serial.println(location.getSurroundingWiFiJson());
  Serial.println("Latitude: " + String(loc.lat, 7));
  Serial.println("Longitude: " + String(loc.lon, 7));
  Serial.println("Accuracy: " + String(loc.accuracy));
  // Hacemos la peticion HTTP mediante el metodo PUT
  peticionPut();
  // Esperamos 15 segundos
  //delay(15000);
}

/********** FUNCION PARA OBTENER MAC COMO STRING **********/
String obtenerMac() {
  // Obtenemos la MAC del dispositivo
  WiFi.macAddress(mac);
  // Convertimos la MAC a String
  String keyMac = "";
  for (int i = 0; i < 6; i++) {
    String pos = String((uint8_t)mac[i], HEX);
    if (mac[i] <= 0xF)
    pos = "0" + pos;
    pos.toUpperCase();
    keyMac += pos;
    if (i < 5)
    keyMac += ":";
  }
  // Devolvemos la MAC en String
  return keyMac;
}

/********** FUNCION QUE REALIZA LA PETICION PUT **************/
void peticionPut() {
  // Cerramos cualquier conexion antes de enviar una nueva peticion
  client.stop();
  client.flush();
  // Enviamos una peticion por SSL
  if (client.connect(HOSTFIREBASE, 443)) {
    // Peticion PUT JSON
    String toSend = "PUT /dispositivo/";
    toSend += macStr;
    toSend += ".json HTTP/1.1\r\n";
    toSend += "Host:";
    toSend += HOSTFIREBASE;
    toSend += "\r\n" ;
    toSend += "Content-Type: application/json\r\n";
    String payload = "{\"lat\":";
    payload += String(loc.lat, LOC_PRECISION);
    payload += ",";
    payload += "\"lon\":";
    payload += String(loc.lon, LOC_PRECISION);
    payload += ",";
    payload += "\"prec\":";
    payload += String(loc.accuracy);
    payload += ",";
    payload += "\"data\":";
    payload += "{\"fuelLevel\":";
    payload += String(fuelLevel);
    payload += ",";
    payload += "\"Km\":";
    payload += String(km);
    payload += ",";
    payload += "\"Aceleration\":";
    payload += String(aceleration);
    payload += ",";
    payload += "\"rpm\":";
    payload += String(rpm);
    payload += ",";
    payload += "\"kmh\":";
    payload += String(kmh);
    payload += ",";
    payload += "\"lane\":";
    payload += String(lane);
    payload += "}";
    payload += ",";
    payload += "\"carld\": \"";
    payload += carld;
    payload += "\"}";
    payload += "\r\n";
    toSend += "Content-Length: " + String(payload.length()) + "\r\n";
    toSend += "\r\n";
    toSend += payload;
    Serial.println(toSend);
    client.println(toSend);
    client.println();
    client.flush();
    client.stop();
    Serial.println("Todo OK");
  } else {
    // Si no podemos conectar
    client.flush();
    client.stop();
    Serial.println("Algo ha ido mal");
  }
}
