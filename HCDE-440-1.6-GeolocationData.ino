/*A sketch to get the ESP8266 on the network and connect to some open services via HTTP to
 * get our external IP address and (approximate) geolocative information in the getGeo()
 * function. To do this we will connect to http://freegeoip.net/json/, an endpoint which
 * requires our external IP address after the last slash in the endpoint to return location
 * data, thus http://freegeoip.net/json/XXX.XXX.XXX.XXX
 *
 * This sketch also introduces the flexible type definition struct, which allows us to define
 * more complex data structures to make receiving larger data sets a bit cleaner/clearer.
 *
 * jeg 2017
 *
 * updated to new API format for Geolocation data from ipistack.com
 * brc 2019
 *
 * Alex Banh, HCDE 440 Spring 2019
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> //provides the ability to parse and construct JSON objects

const char* ssid = "Tell my Wi-fi love her";
const char* pass = "thirstybanana";
const char* key = "1f05870a9580401660bcc2a80bbd7b65"; //API key for ipstack API call
const char* metkey = "ff7a21252e1cec7a9c1ac3475354704d"; //API key for openweathermap API call
String ipaddress;

typedef struct { //here we create a new data type definition, a box to hold other data types
  String ip;    //
  String cc;    //for each name:value pair coming in from the service, we will create a slot
  String cn;    //in our structure to hold our data
  String rc;
  String rn;
  String cy;
  String ln;
  String lt;
} GeoData;     //then we give our new data structure a name so we can use it in our code

typedef struct { // creates a new data type called MetData which holds name:value pairs
  String temp;   // of Strings representing temperature, humidity, windspeed, wind
  String humid;  // direction, and cloud data.
  String windsp;
  String winddir;
  String cloud;
} MetData;     //then we give our new data structure a name so we can use it in our code

MetData meteo; // Creates an instance of a MetData type called "meteo"

GeoData location; //we have created a GeoData type, but not an instance of that type,
                  //so we create the variable 'location' of type GeoData
void setup() {
  Serial.begin(115200);

  // Prints file name and compile time
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));

  delay(10);
  Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());
  ipaddress = getIP();
  getGeo();
  Serial.println("Your external IP address is " + location.ip);
  Serial.print("Your ESP is currently in " + location.cn + " (" + location.cc + "),");
  Serial.println(" in or near " + location.cy + ", " + location.rc + ".");
  Serial.println("and located at (roughly) ");
  Serial.println(location.lt + " latitude by " + location.ln + " longitude.");
  getMet();
  // Prints out collected data from openweathermap API call
  Serial.println("In Seattle, the temperature is currently " + meteo.temp + " degrees fahrenheit.");
  Serial.println("Humidity is " + meteo.humid + "%");
  Serial.println("Windspeed is " + meteo.windsp + " miles per hour at " + meteo.winddir + " degrees.");
  Serial.println("Cloud is: " + meteo.cloud);
}

void loop() {
  //if we put getIP() here, it would ping the endpoint over and over . . . DOS attack?
}

String getIP() {
  HTTPClient theClient;
  String ipAddress;
  theClient.begin("http://api.ipify.org/?format=json");
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      ipAddress = root["ip"].as<String>();
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
      return "error";
    }
  }
  return ipAddress;
}

void getGeo() {
  HTTPClient theClient;
  Serial.println("Making HTTP request");
  theClient.begin("http://api.ipstack.com/" + ipaddress + "?access_key=" + key); //return IP as .json object
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      //Some debugging lines below:
      //      Serial.println(payload);
      //      root.printTo(Serial);
      //Using .dot syntax, we refer to the variable "location" which is of
      //type GeoData, and place our data into the data structure.
      location.ip = root["ip"].as<String>();            //we cast the values as Strings b/c
      location.cc = root["country_code"].as<String>();  //the 'slots' in GeoData are Strings
      location.cn = root["country_name"].as<String>();
      location.rc = root["region_code"].as<String>();
      location.rn = root["region_name"].as<String>();
      location.cy = root["city"].as<String>();
      location.lt = root["latitude"].as<String>();
      location.ln = root["longitude"].as<String>();
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}

// getMet makes a request to the openweathermap API which returns a json object.
// The json object is then parsed to extract meteorological data.
void getMet() {
  HTTPClient theClient;
  Serial.println("Making HTTP request for meteorology data");
  //returns meteorological data as a json payload
  theClient.begin("http://api.openweathermap.org/data/2.5/weather?q=Seattle&units=imperial&appid=" + String(metkey));
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    // If received an HTTP code of 200 indicating success, continue
    if (httpCode == 200) {
      Serial.println("Received HTTP payload for meteorology.");
      // If JSON payload was successfully received, creates a dynamic buffer
      // to process the data contained within the json payload
      DynamicJsonBuffer jsonBuffer;
      // Sets the payload to the json string obtained by the HTTP client
      String payload = theClient.getString();
      Serial.println("Parsing meteorology data...");
      // Parses the string representing the json data
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      // Sets the temperature value of the MetData object based on the temp value given by the json object
      meteo.temp = root["main"]["temp"].as<String>();
      // Sets the humidity value of the MetData object based on the humidity value given by the json object
      meteo.humid = root["main"]["humidity"].as<String>();
      // Sets the wind speed value of the MetData object based on the wind speed value given by the json object
      meteo.windsp = root["wind"]["speed"].as<String>();
      // Sets the wind direction value of the MetData object based on the wind degree value given by the json object
      meteo.winddir = root["wind"]["deg"].as<String>();
      // Sets the cloud value of the MetData object based on the cloud value given by the json object
      meteo.cloud = root["clouds"]["all"].as<String>();

    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}
