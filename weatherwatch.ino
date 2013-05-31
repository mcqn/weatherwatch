// (c) Copyright 2013 MCQN Ltd.
// Released under Apache License, version 2.0
//
// WeatherWatch
//
// A sketch to get the latest weather forecast from the Met Office
// and display some of the key information from it

#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <Servo.h>
#include "api_key.h"

// Name of the server we want to connect to
const char kHostname[] = "datapoint.metoffice.gov.uk";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
// FIXME Explain how to find the right weather station ID
const char kPath[] = "/public/data/val/wxfcs/all/json/310012?res=3hourly&key=" APIKEY;  // Central Liverpool
// FIXME another weather station - Cornwall, Skye?
//const char kPath[] = "/public/data/val/wxfcs/all/json/INSERT_WEATHER_STATION_ID_HERE?res=3hourly&key=" APIKEY;

// This needs to be unique on your local network (so if you have more than one Shrimp/Arduino then they need to be different for each one)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

const int kLEDone = 2;
const int kLEDtwo = 3;
const int kLEDthree = A0;
const int kLEDfour = A5;
const int kLEDfive = 5;

const int kServoPin = 6;

Servo myServo;

int windSpeed = 0;
int temperature = 0;
int chanceOfRain = 0;
int weatherType = 0;

void setup()
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  Serial.println("Hello!");
  Serial.println("Getting ready to watch the weather.");

  // Set up lights to show chance of rain
  pinMode(kLEDone, OUTPUT);
  pinMode(kLEDtwo, OUTPUT);
  pinMode(kLEDthree, OUTPUT);
  pinMode(kLEDfour, OUTPUT);
  pinMode(kLEDfive, OUTPUT);
  
  myServo.attach(kServoPin);
  
  // Test the circuit is wired up right
  int testGap = 100;
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

  // Set up the networking, so we can talk to the Internet
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }  
}

void loop()
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
          if (http.find("Pp"))
          {
            // We've got the first "% chance of rain" reading
            // Read in the value for it
            chanceOfRain = http.parseInt();
            Serial.print("Chance of rain: ");
            Serial.println(chanceOfRain);
            // Now look for the wind speed
            if (http.find("S"))
            {
              // Found it
              windSpeed = http.parseInt();
              Serial.print("    Wind speed: ");
              Serial.println(windSpeed);
              // And now the temperature
              if (http.find("T"))
              {
                // Got it
                temperature = http.parseInt();
                Serial.print("   Temperature: ");
                Serial.println(temperature);
                // Finally look for the weather type
                if (http.find("W"))
                {
                  // And we have it
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

  // Update the weather
  temperatureToServo();
  chanceOfRainToLEDs();

  // And wait for a bit before checking the forecast again
  delay(10000);
}

void temperatureToServo()
{
  // FIXME
  // Servo runs between 0 and 170 degrees
  // temperature ranges from 0 to 35
  // Map the temperature onto the right servo position
  int pos = map(temperature, 0, 35, 0, 170);
  myServo.write(pos);
}

void chanceOfRainToLEDs()
{
  if (chanceOfRain > 20)
  {
    digitalWrite(kLEDone, HIGH);
  }
  else
  {
    digitalWrite(kLEDone, LOW);
  }
  if (chanceOfRain > 40)
  {
    digitalWrite(kLEDtwo, HIGH);
  }
  else
  {
    digitalWrite(kLEDtwo, LOW);
  }
  if (chanceOfRain > 60)
  {
    digitalWrite(kLEDthree, HIGH);
  }
  else
  {
    digitalWrite(kLEDthree, LOW);
  }
  if (chanceOfRain > 80)
  {
    digitalWrite(kLEDfour, HIGH);
  }
  else
  {
    digitalWrite(kLEDfour, LOW);
  }
}

