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
#include "api_key.h"

// Name of the server we want to connect to
const char kHostname[] = "datapoint.metoffice.gov.uk";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
const char kPath[] = "/public/data/val/wxfcs/all/json/310012?res=3hourly&key=" APIKEY;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

void setup()
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  Serial.println("Hello!");
  Serial.println("Getting ready to watch the weather.");

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
            int chanceOfRain = http.parseInt();
            Serial.print("Chance of rain: ");
            Serial.println(chanceOfRain);
            // Now look for the wind speed
            if (http.find("S"))
            {
              // Found it
              int windSpeed = http.parseInt();
              Serial.print("    Wind speed: ");
              Serial.println(windSpeed);
              // And now the temperature
              if (http.find("T"))
              {
                // Got it
                int temperature = http.parseInt();
                Serial.print("   Temperature: ");
                Serial.println(temperature);
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

  // And wait for a bit before checking the forecast again
  delay(10000);
}

