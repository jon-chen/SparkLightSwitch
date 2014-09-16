#include "LightTimer.h"
#include "rest_client.h"
#include "JsonParser.h"

#ifdef SPARK_DEBUG
#define DEBUG_PRINT(string) (Serial.print(string))
#endif

#ifndef SPARK_DEBUG
#define DEBUG_PRINT(string)
#endif

LightTimer::LightTimer()
{
    // time gets automagically sync'd on start up
    lastTimeSync = millis();
}

void LightTimer::setOutletSwitchPin(int pin)
{
    outletSwitchPin = pin;

    // Initialize pin.
    pinMode(outletSwitchPin, OUTPUT);
    toggleOutletSwitch(false);
}

void LightTimer::setTimezoneOffset(int offset)
{
    bool isDst = rtc.isUSDST(Time.now());
    timezoneOffset = (isDst ? offset + 1 : offset);
    Time.zone(timezoneOffset);
}

void LightTimer::setSunsetApiUrl(String baseUrl, String apiUrl)
{
    weatherApiBaseUrl = baseUrl;
    sunsetApiUrl = apiUrl;
}

void LightTimer::setSunsetApiCheckTime(String timeString)
{
    sunsetApiCheckTime = timeString;
}

void LightTimer::setTimerOffTime(String timeString)
{
    timerOffTime = timeString;
}

void LightTimer::toggleOutletSwitchOn()
{
    time_t now = Time.now();
    int checkHour, checkMinute;
    LightTimer::parseTimestamp(sunsetTime, &checkHour, &checkMinute);

    DEBUG_PRINT("should enabled? ");
    DEBUG_PRINT(shouldOutletSwitchBeEnabled());
    DEBUG_PRINT("\n");
    DEBUG_PRINT("switch state: ");
    DEBUG_PRINT(getOutletSwitchState());
    DEBUG_PRINT("\n");

    // If the check time has arrived and the last check time wasn't this
    // minute, or the switch is off but should be on by our calculations
    // and we haven't recently switched it off.
    if ((Time.hour() == checkHour && Time.minute() == checkMinute &&
        now / (1000 * 60) != lastToggleOutletSwitchOnTime / (1000 * 60)) ||
        (shouldOutletSwitchBeEnabled() && !getOutletSwitchState() &&
        now / (1000 * 60) != lastToggleOutletSwitchOffTime / (1000 * 60)))
    {
        DEBUG_PRINT("Toggling outlet switch on... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        toggleOutletSwitch(true);
        lastToggleOutletSwitchOnTime = now;
    }
}

void LightTimer::toggleOutletSwitchOff()
{
    time_t now = Time.now();
    int checkHour, checkMinute;
    LightTimer::parseTime(timerOffTime.c_str(), &checkHour, &checkMinute);

    // If the check time has arrived and the last check time wasn't this
    // minute.
    if (Time.hour() == checkHour && Time.minute() == checkMinute &&
        now / (1000 * 60) != lastToggleOutletSwitchOffTime / (1000 * 60))
    {
        DEBUG_PRINT("Toggling outlet switch off... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        toggleOutletSwitch(false);
        lastToggleOutletSwitchOffTime = now;
    }
}

void LightTimer::toggleOutletSwitch(bool isEnabled)
{
    int value = isEnabled ? HIGH : LOW;
    digitalWrite(outletSwitchPin, value);
    outletSwitchState = isEnabled;
}

bool LightTimer::getOutletSwitchState()
{
    return outletSwitchState;
}

bool LightTimer::shouldOutletSwitchBeEnabled()
{
    time_t now = Time.now();
    time_t lightOn = sunsetTime;
    time_t lightOff = LightTimer::parseTimeFromString(timerOffTime.c_str());

    return  (now < lightOn) && (now > lightOff);
}

void LightTimer::syncTime()
{
    unsigned long now = millis();

    // just sync the time once a day
    if (now - lastTimeSync > dayInMilliseconds)
    {
        DEBUG_PRINT("Syncing time... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        Spark.syncTime();
        lastTimeSync = now;
    }
}

void LightTimer::retrieveSunsetData()
{
    time_t now = Time.now();
    int checkHour, checkMinute;
    LightTimer::parseTime(sunsetApiCheckTime.c_str(), &checkHour, &checkMinute);

    // If the check time has arrived and the last check time wasn't this
    // minute, or if we've never retrieved the sunset data yet.
    if ((Time.hour() == checkHour && Time.minute() == checkMinute &&
        now / (1000 * 60) != lastSunsetApiCheckTime / (1000 * 60)) ||
        lastSunsetApiCheckTime == NULL)
    {
        DEBUG_PRINT("Retrieving sunset data... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        time_t time = getAndParseSunsetData();
        if (time != -1)
        {
            DEBUG_PRINT("Sunset data retrieved! ");
            DEBUG_PRINT(Time.timeStr() + "\n");
            DEBUG_PRINT("Sunset is at: ");
            DEBUG_PRINT(time);
            DEBUG_PRINT("\n");

            sunsetTime = time;
            lastSunsetApiCheckTime = now;
        }
        else
        {
            DEBUG_PRINT("Failed to retrieve sunset data, retrying in a few seconds...");
            delay(5000);
            retrieveSunsetData();
        }
    }
}

time_t LightTimer::getAndParseSunsetData()
{
    using namespace ArduinoJson::Parser;
    const char* response = getSunsetDataResponse();

    // Parse response if we have one.
    if (strlen(response) != 0)
    {
        JsonParser<64> parser;
        JsonObject root = parser.parse(const_cast<char*>(response));

        if(root.success())
        {
            int hour = atoi(root["moon_phase"]["sunset"]["hour"]);
            int minute = atoi(root["moon_phase"]["sunset"]["minute"]);
            return LightTimer::parseTimeFromToday(hour, minute);
        }
    }

    return -1;
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

time_t LightTimer::parseTimeFromToday(int hour, int minute)
{
    // For simplicity, we're assuming that the times are for today.
    time_t t = Time.now();

    struct tm* timeinfo;

    // Alter the timestamp with the sunset time.
    timeinfo = localtime(&t);
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = 0;

    // Return unix timestamp of sunset.
    return mktime(timeinfo);
}

time_t LightTimer::parseTimeFromString(const char* data)
{
    int hour, minute;
    LightTimer::parseTime(data, &hour, &minute);
    return LightTimer::parseTimeFromToday(hour, minute);
}

void LightTimer::parseTime(const char* data, int* hour, int* minute)
{
    sscanf(data, "%d:%d", hour, minute);
}

void LightTimer::parseTimestamp(time_t timestamp, int* hour, int* minute)
{
    *hour = Time.hour(timestamp);
    *minute = Time.minute(timestamp);
}
