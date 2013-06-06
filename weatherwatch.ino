//#########################################################################
//########################    WeatherWatch    #############################
//#########################################################################
//
// (c) Copyright 2013 MCQN Ltd.
// Released under Apache License, version 2.0
//
// WeatherWatch
//
// A sketch to get the latest weather forecast from the Met Office
// and display some of the key information from it

//################# LIBRARIES ################
#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <Servo.h>
#include "api_key.h"

//################ PINS USED ######################
//Cannot use Pins 10, 11, 12, 13
//(these are used by the Ethernet chip)
const int kLEDone = 2;
const int kLEDtwo = 3;
const int kLEDthree = A0;
const int kLEDfour = A5;
const int kLEDfive = 5;
const int kServoPin = 6;

//################ VARIABLES ################

//------ NETWORK VARIABLES---------
// Name of the server we want to connect to
const char kHostname[] = "datapoint.metoffice.gov.uk";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
//--MAKE CHANGE HERE--MAKE CHANGE HERE--MAKE CHANGE HERE--MAKE CHANGE HERE--MAKE CHANGE HERE--    
// At present it will get the weather forecast for central Liverpool.  You can comment out the next
// line and uncomment one of the others to get the forecast for Bodmin in Cornwall or Prabost on
// the Isle of Skye.  Or you could find out the right station ID for the forecast nearest to you!
const char kPath[] = "/public/data/val/wxfcs/all/json/310012?res=3hourly&key=" APIKEY;  // Central Liverpool
//const char kPath[] = "/public/data/val/wxfcs/all/json/322041?res=3hourly&key=" APIKEY;  // Bodmin, Cornwall
//const char kPath[] = "/public/data/val/wxfcs/all/json/371633?res=3hourly&key=" APIKEY;  // Prabost, Skye
/* To find the right weather station ID for your nearest weather station, go to
 * http://www.metoffice.gov.uk/public/weather/forecast/ and find the right forecast there
 * Then find the "view source" option in your web browser, and search for "locationId"
 * You should find something like this (this example is for Liverpool):
<script type="text/javascript">
var staticPage = { locationId: 310012};
</script>
 * The number after the locationId part is the station ID for your weather station
 */
//const char kPath[] = "/public/data/val/wxfcs/all/json/INSERT_WEATHER_STATION_ID_HERE?res=3hourly&key=" APIKEY;

// This needs to be unique on your local network (so if you have more than one Shrimp/Arduino then they need to be different for each one)
//--MAKE CHANGE HERE--MAKE CHANGE HERE--MAKE CHANGE HERE--MAKE CHANGE HERE--MAKE CHANGE HERE--    
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

//------ OUTPUT VARIABLES---------
Servo myServo;

//------ WEATHER VARIABLES---------
int windSpeed = 0;
int temperature = 0;
int chanceOfRain = 0;
int weatherType = 0;


/*
 * setup() - this function runs once when you turn your Arduino on
 */
void setup()
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  Serial.println("Hello!");
  Serial.println("Getting ready to watch the weather.");

  setupCircuit();
  testCircuit();
  setupEthernet();
}


/*
* loop() - this function will start after setup finishes and then repeat
*/
void loop()
{
  // Check the forecast
  downloadForecast();
  
  // Update the weather display using the information we got from the forecast
  temperatureToServo();
  // At the moment it will display the % chance of rain on the LEDs.  If you'd
  // rather it displays what sort of weather (sunny/overcast/rainy/snowy/etc.)
  // then comment out the next line and uncomment the one after
  //chanceOfRainToLEDs();
  weatherTypeToLEDs();

  // And wait for a bit before checking the forecast again
  delay(10000);
}
  

/*
 * setupCircuit() - this function sets all the pins to their correct mode
 */  
void setupCircuit()
{
  // Set up the LED pins
  pinMode(kLEDone, OUTPUT);
  pinMode(kLEDtwo, OUTPUT);
  pinMode(kLEDthree, OUTPUT);
  pinMode(kLEDfour, OUTPUT);
  pinMode(kLEDfive, OUTPUT);
  // And now the servo
  myServo.attach(kServoPin);
}


/*
 * testCircuit() - this function tests your circuit, turning the LEDs on 
 * in turn and outputting which one it should be to the serial monitor
 * and check the range of the servo
 */
void testCircuit()
{
  // Test the LEDs are wired up right
  int testGap = 300;
  Serial.println("Testing LEDs...");
  Serial.println("One");
  digitalWrite(kLEDone, HIGH);
  delay(testGap);
  digitalWrite(kLEDone, LOW);
  Serial.println("Two");
  digitalWrite(kLEDtwo, HIGH);
  delay(testGap);
  digitalWrite(kLEDtwo, LOW);
  Serial.println("Three");
  digitalWrite(kLEDthree, HIGH);
  delay(testGap);
  digitalWrite(kLEDthree, LOW);
  Serial.println("Four");
  digitalWrite(kLEDfour, HIGH);
  delay(testGap);
  digitalWrite(kLEDfour, LOW);
  digitalWrite(kLEDfive, HIGH);
  delay(testGap);
  digitalWrite(kLEDfive, LOW);

  // And now the servo
  Serial.println("Testing servo over full range");
  for (int i =0; i <= 170; i++)
  {
    Serial.print(".");
    myServo.write(i);
    delay(20);
  }
  myServo.write(90);
  Serial.println();
}


/*
 * setupEthernet() - Sets up your ethernet connection
 */
void setupEthernet()
{ 
  Serial.print("Configuring Ethernet...");
  // Set up the networking, so we can talk to the Internet
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }  
  Serial.println("online!");
}


/*
 * downloadForecast() - Connect to the Met Office website, and
 * retrieve the latest 3-hour forecast
 */
int downloadForecast()
{
  int err =0;
  
  EthernetClient c;
  HttpClient http(c);
  
  err = http.get(kHostname, kPath);
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 200 && err < 300)
    {
      // It's a 2xx response code, which is a success
      err = http.skipResponseHeaders();
      
      if (err >= 0)
      {
        // Now we're into the data being returned.  We need to search
        // through it for the bits of the forecast we're interested in
        if (http.find("Day"))
        {
          // We've skipped past all the stuff at the start we don't care about

          // Now look for the first part we want to check - the chance of rain.
          // That's marked as "Pp" in the forecast information
          if (http.find("Pp"))
          {
            // We've found the first "% chance of rain" reading
            // Read in the value for it
            chanceOfRain = http.parseInt();
            Serial.print("Chance of rain: ");
            Serial.println(chanceOfRain);
            // Now look for the wind speed, that's marked as "S"
            if (http.find("S"))
            {
              // Found it
              // Read in the value for it
              windSpeed = http.parseInt();
              Serial.print("    Wind speed: ");
              Serial.println(windSpeed);
              // And now the temperature, which is marked as "T"
              if (http.find("T"))
              {
                // Got it
                // Read in the temperature, which will be the next Integer (whole number)
                temperature = http.parseInt();
                Serial.print("   Temperature: ");
                Serial.println(temperature);
                // Finally look for the weather type
                if (http.find("W"))
                {
                  // And we have it
                  // Again read in the value
                  weatherType = http.parseInt();
                  Serial.print("Weather Type: ");
                  Serial.println(weatherType);
                }
              }
            }
          }
        }
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else {
      Serial.print("Problem reading page.  Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
      
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout) )
        {
            if (http.available())
            {
                c = http.read();
                // Print out this character
                Serial.print(c);
               
                bodyLen--;
                // We read something, reset the timeout counter
                timeoutStart = millis();
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(kNetworkDelay);
            }
        }
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
  
  // Let the code that called us know whether things went okay or not
  return err;
}


/*
 * temperatureToServo() - takes the temperature value retrieved from the
 * Met Office, and works out where to position the servo to display it
 */
void temperatureToServo()
{
  // Servo runs between 0 and 170 degrees
  // temperature ranges from 0 to 35
  // Map the temperature onto the right servo position
  // If it's less than 10 degrees, that's cold
  int pos;
  temperature = 35;
  if (temperature < 10)
  {
    // It's cold, show the cold person
    pos = map(temperature, -2, 10, 0, 80);
  }
  else if (temperature > 15)
  {
    // It's hot, show the hot person
    pos = map(temperature, 15, 35, 90, 170);
  }
  else
  {
    // It's neither hot nor cold, have them both show equally
    pos = 85;
  }
  // Now we know what position the servo should be in, move it there
  myServo.write(pos);
}


/*
 * chanceOfRainToLEDs() - takes the % chance of rain value retrieved from
 * the Met Office, and lights up the LEDs to display that.  Higher chance
 * of rains means more LEDs get lit
 */
void chanceOfRainToLEDs()
{
  if (chanceOfRain > 17)
  {
    digitalWrite(kLEDone, HIGH);
  }
  else
  {
    digitalWrite(kLEDone, LOW);
  }
  if (chanceOfRain > 33)
  {
    digitalWrite(kLEDtwo, HIGH);
  }
  else
  {
    digitalWrite(kLEDtwo, LOW);
  }
  if (chanceOfRain > 5)
  {
    digitalWrite(kLEDthree, HIGH);
  }
  else
  {
    digitalWrite(kLEDthree, LOW);
  }
  if (chanceOfRain > 67)
  {
    digitalWrite(kLEDfour, HIGH);
  }
  else
  {
    digitalWrite(kLEDfour, LOW);
  }
  if (chanceOfRain > 83)
  {
    digitalWrite(kLEDfive, HIGH);
  }
  else
  {
    digitalWrite(kLEDfive, LOW);
  }
}


/*
 * weatherTypeToLEDs() - take the weather type retrieved from the Met
 * Office, and light up the right LED to indicate what the weather is
 * forecast to be
 */
void weatherTypeToLEDs()
{
  // Weather types are defined at http://www.metoffice.gov.uk/datapoint/support/code-definitions

  if ( (weatherType >=0) && (weatherType <=1) )
  {
    // Clear and sunny
    digitalWrite(kLEDone, HIGH);
    digitalWrite(kLEDtwo, LOW);
    digitalWrite(kLEDthree, LOW);
    digitalWrite(kLEDfour, LOW);
    digitalWrite(kLEDfive, LOW);
  }
  else if ( (weatherType >= 2) && (weatherType <= 4) )
  {
    // Some cloud, but dry
    digitalWrite(kLEDone, LOW);
    digitalWrite(kLEDtwo, HIGH);
    digitalWrite(kLEDthree, LOW);
    digitalWrite(kLEDfour, LOW);
    digitalWrite(kLEDfive, LOW);
  }
  else if ( (weatherType >= 5) && (weatherType <= 8) )
  {
    // Misty or overcast, but dry
    digitalWrite(kLEDone, LOW);
    digitalWrite(kLEDtwo, LOW);
    digitalWrite(kLEDthree, HIGH);
    digitalWrite(kLEDfour, LOW);
    digitalWrite(kLEDfive, LOW);
  }
  else if ( (weatherType >= 9) && (weatherType <= 15) )
  {
    // Rain
    digitalWrite(kLEDone, LOW);
    digitalWrite(kLEDtwo, LOW);
    digitalWrite(kLEDthree, LOW);
    digitalWrite(kLEDfour, HIGH);
    digitalWrite(kLEDfive, LOW);
  }
  else if ( (weatherType >= 16) && (weatherType <= 27) )
  {
    // Hail, sleet or snow
    digitalWrite(kLEDone, LOW);
    digitalWrite(kLEDtwo, LOW);
    digitalWrite(kLEDthree, LOW);
    digitalWrite(kLEDfour, LOW);
    digitalWrite(kLEDfive, HIGH);
  }
  else if ( (weatherType >= 28) && (weatherType <= 30) )
  {
    // Thunder - treat it the same as rain
    digitalWrite(kLEDone, LOW);
    digitalWrite(kLEDtwo, LOW);
    digitalWrite(kLEDthree, LOW);
    digitalWrite(kLEDfour, HIGH);
    digitalWrite(kLEDfive, LOW);
  }
}

