#include "LightTimer.h"
#include "rest_client.h"
#include "JsonParser.h"

#ifdef DEBUG
#define DEBUG_PRINT(string) (Serial.print(string))
#endif

#ifndef DEBUG
#define DEBUG_PRINT(string)
#endif

LightTimer::LightTimer()
{
    lastTimeSync = millis();
}

void LightTimer::syncTime()
{
    unsigned long now = millis();

    if (now - lastTimeSync > dayInMilliseconds)
    {
        Spark.syncTime();
        lastTimeSync = now;
    }
}

unsigned long LightTimer::retrieveSunsetData()
{
    using namespace ArduinoJson::Parser;
    const char* response = getSunsetDataResponse();

    // parse response
    if (response != "")
    {
        JsonParser<64> parser;
        JsonObject root = parser.parse(const_cast<char*>(response));

        if(!root.success())
        {
            DEBUG_PRINT("parse failed.");
            return 0;
        }

        char* version = root["response"]["version"];
        char* hour   = root["moon_phase"]["sunset"]["hour"];
        char* minute = root["moon_phase"]["sunset"]["minute"];
        DEBUG_PRINT("Sunset: ");
        DEBUG_PRINT(hour);
        DEBUG_PRINT(":");
        DEBUG_PRINT(minute);
        DEBUG_PRINT("\n");
        DEBUG_PRINT("version: ");
        DEBUG_PRINT(version);
        DEBUG_PRINT("\n");

        return 0;
    }
}

const char* LightTimer::getSunsetDataResponse()
{
    RestClient client = RestClient(weatherApiBaseUrl.c_str());
    String response = "";

    int statusCode = client.get(sunsetApiUrl.c_str(), &response);

    if (statusCode == 200)
    {
        DEBUG_PRINT("Successfully retrieved JSON response\n");
        return response.c_str();
    }
    else
    {
        DEBUG_PRINT("Failed to retrieve sunset data with status: ");
        DEBUG_PRINT(statusCode);
        DEBUG_PRINT("\n");
        return "";
    }
}
