#include <ESP8266WiFi.h>
#include "WifiLocation.h"
#define GOOGLE_KEY "AIzaSyDP4SOZgPpJCEYormxf53cA9tnFIPoxArk" // Clave API Google Geolocation
#define SSID "PATRICIA" // SSID de tu red WiFi
#define PASSWD "qaws78ed" // Clave de tu red WiFi
//#define HOSTFIREBASE "proyecto1-8852c.firebaseio.com" // Host o url de Firebase
#define HOSTFIREBASE "probando-nodemcu.firebaseio.com" // Host o url de Firebase
#define LOC_PRECISION 7 // Precision de latitud y longitud
// Llamada a la API de Google
WifiLocation location(GOOGLE_KEY);
location_t loc; // Estructura de datos que devuelve la librera WifiLocation

// Variables
byte mac[6];
String macStr = "";
String nombreComun = "NodeMCU";
String carld = "Cualquiera";
int timestamp = 1;
int fuelLevel = 2;
int km = 1;
int aceleration = 0;
int rpm = 30;
int kmh = 50;
int lane = 1;

String eventType= "none";
int oldLane= 1;
int newLane= 0;
bool airbagActive= false;
int failureCode=0;


/**********ciclo de eventos****/

void mf(){
  eventType = "mechanicFailure";
  //r = sensorAuto() <---------permite saber a través de sensores que falla mecánica tiene
  r = 1;
  if (r==1){
    failureCode=1;  //falla de motor
  }else{
    failureCode=2;  //falla de rueda
  }
}

void choque(){
  eventType = "crash";
  aceleration = 0;
  kmh = 0;
  ariBagActive = true;
}

void carril(){
  evetnType="laneChanged";
  //oldLane = getLane();  <-----obtiene carril usado (Lane)
  oldLane = 1;
  //newLane= getNewLane();  <-----obtiene carril de cambio
  newLane= 2;
}



/******************************/


// Cliente WiFi
WiFiClientSecure client;

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
void loop() {
  // Obtenemos la geolocalizacion WiFi
  loc = location.getGeoFromWiFi();
  // Mostramos la informacion en el monitor serie
  Serial.println("Location request data");
  Serial.println(location.getSurroundingWiFiJson());
  Serial.println("Latitude: " + String(loc.lat, 7));
  Serial.println("Longitude: " + String(loc.lon, 7));
  Serial.println("Accuracy: " + String(loc.accuracy));
  // Hacemos la peticion HTTP mediante el metodo PUT
  peticionPut();
  // Esperamos 15 segundos
  delay(15000);
}
/********** FUNCION PARA OBTENER MAC COMO STRING **********/
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
/********** FUNCION QUE REALIZA LA PETICION PUT **************/
void peticionPut() {
  // Cerramos cualquier conexion antes de enviar una nueva peticion
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
    //incorporar sentencias if para registrar los estados
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
