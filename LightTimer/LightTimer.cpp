#include "LightTimer.h"
using namespace ArduinoJson::Parser;

LightTimer::LightTimer()
{
    // time gets automagically sync'd on start up
    lastTimeSync = millis();
}

void LightTimer::initialize()
{
    // If the switch is off but should be on by our calculations.
    if (shouldOutletSwitchBeEnabled() && !getOutletSwitchState())
    {
        DEBUG_PRINT("Enabling switch since it should be on.\n");
        toggleOutletSwitch(true);
        lastToggleOutletSwitchOnTime = Time.now();
    }
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
    // Build in some delay to make sure that the timezone gets set properly.
    delay(500);
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

void LightTimer::setOutletSwitchOnTime(String timeString)
{
    outletSwitchOnTime = timeString;
}

void LightTimer::setOutletSwitchOffTime(String timeString)
{
    outletSwitchOffTime = timeString;
}

void LightTimer::getCurrentState(char* currentState)
{
    // TODO: Use JsonGenerator to create a JSON object and serialize it to
    // the currentState string.

    DEBUG_PRINT("Current time: ");
    DEBUG_PRINT(Time.timeStr());

    DEBUG_PRINT("Time On: ");
    DEBUG_PRINT(Time.timeStr(getOutletSwitchOnTime()));

    DEBUG_PRINT("Time Off: ");
    DEBUG_PRINT(Time.timeStr(getOutletSwitchOffTime()));

    DEBUG_PRINT("Use sunset on time: ");
    DEBUG_PRINT(useSunsetOnTime());
    DEBUG_PRINT("\n");

    DEBUG_PRINT("Sunset time: ");
    DEBUG_PRINT(Time.timeStr(sunsetTime));

    DEBUG_PRINT("Timezone offset: ");
    DEBUG_PRINT(timezoneOffset);
    DEBUG_PRINT("\n");

    DEBUG_PRINT("Last time sync: ");
    DEBUG_PRINT(Time.timeStr(lastTimeSync));

    DEBUG_PRINT("Last sunset API check: ");
    DEBUG_PRINT(Time.timeStr(LightTimer::parseTimeFromString(
        sunsetApiCheckTime.c_str(), timezoneOffset)));

    DEBUG_PRINT("Last toggle on time: ");
    DEBUG_PRINT(Time.timeStr(lastToggleOutletSwitchOnTime));

    DEBUG_PRINT("Last toggle off time: ");
    DEBUG_PRINT(Time.timeStr(lastToggleOutletSwitchOffTime));

    DEBUG_PRINT("Current switch state: ");
    DEBUG_PRINT(getOutletSwitchState());
    DEBUG_PRINT("\n");

    DEBUG_PRINT("Should switch be on: ");
    DEBUG_PRINT(shouldOutletSwitchBeEnabled());
    DEBUG_PRINT("\n");

    DEBUG_PRINT("\n");
}

int LightTimer::configureHandler(String command)
{
    #ifdef SPARK_DEBUG
    Serial.println("Outlet Control API request received...");
    #endif

    JsonParser<64> parser;
    JsonObject root = parser.parse(const_cast<char*>(command.c_str()));

    JsonValue value;
    value = root[configKeys[OutletSwitchState]];
    if ((int)value != 0)
    {
        toggleOutletSwitch(value);
        value = JsonValue();
    }

    value = root[configKeys[TimezoneOffset]];
    if ((int)value != 0)
    {
        setTimezoneOffset(value);
        value = JsonValue();
    }

    value = root[configKeys[SunsetApiUrl]];
    if ((int)value != 0)
    {
        // TODO: Split the url into the base and relative url
        // setSunsetApiUrl
        value = JsonValue();
    }

    value = root[configKeys[SunsetApiCheckTime]];
    if ((int)value != 0)
    {
        setSunsetApiCheckTime((const char*)value);
        value = JsonValue();
    }

    value = root[configKeys[OutletSwitchOffTime]];
    if ((int)value != 0)
    {
        setOutletSwitchOffTime((const char*)value);
        value = JsonValue();
    }

    return 1;
}

void LightTimer::toggleOutletSwitchOn()
{
    int checkHour, checkMinute;
    time_t now = Time.now();
    time_t checkTime = getOutletSwitchOnTime();
    LightTimer::parseTimestamp(checkTime, &checkHour, &checkMinute);

    // If the check time has arrived and the last check time wasn't this
    // minute (so we don't try to turn it on more than once).
    if (Time.hour() == checkHour && Time.minute() == checkMinute &&
        now / (1000 * 60) != lastToggleOutletSwitchOnTime / (1000 * 60))
    {
        DEBUG_PRINT("Toggling outlet switch on... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        toggleOutletSwitch(true);
        lastToggleOutletSwitchOnTime = now;
    }
}

void LightTimer::toggleOutletSwitchOff()
{
    int checkHour, checkMinute;
    time_t now = Time.now();
    time_t checkTime = getOutletSwitchOffTime();
    LightTimer::parseTimestamp(checkTime, &checkHour, &checkMinute);

    // If the check time has arrived and the last check time wasn't this
    // minute (so we don't try to turn it off more than once).
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
    time_t lightOn = getOutletSwitchOnTime();
    time_t lightOff = getOutletSwitchOffTime();

    return  (now > lightOn) && (now < lightOff);
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
    // Only retrieve sunset data if we actually need it.
    if (useSunsetOnTime())
    {
        time_t now = Time.now();
        int checkHour, checkMinute;
        LightTimer::parseTime(sunsetApiCheckTime.c_str(), &checkHour, &checkMinute);

        // If the check time has arrived and the last check time wasn't this
        // minute, or if we've never retrieved the sunset data yet.
        if ((Time.hour() == checkHour && Time.minute() == checkMinute &&
            now / (1000 * 60) != lastSunsetApiCheckTime / (1000 * 60)) ||
            lastSunsetApiCheckTime == 0)
        {
            DEBUG_PRINT("Retrieving sunset data... ");
            DEBUG_PRINT(Time.timeStr() + "\n");

            time_t time = getAndParseSunsetData();
            if (time != -1)
            {
                DEBUG_PRINT("Sunset data retrieved! ");
                DEBUG_PRINT(Time.timeStr() + "\n");
                DEBUG_PRINT("Sunset is at: ");
                DEBUG_PRINT(Time.timeStr(time) + " ");
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
}

time_t LightTimer::getAndParseSunsetData()
{
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
            return LightTimer::parseTimeFromToday(hour, minute, timezoneOffset);
        }
    }

    return -1;
}

time_t LightTimer::getOutletSwitchOffTime()
{
    return LightTimer::parseTimeFromString(
        outletSwitchOffTime.c_str(), timezoneOffset);
}

bool LightTimer::useSunsetOnTime()
{
    return outletSwitchOnTime == "sunset";
}

time_t LightTimer::getOutletSwitchOnTime()
{
    if (useSunsetOnTime())
    {
        return sunsetTime;
    }

    return LightTimer::parseTimeFromString(
        outletSwitchOnTime.c_str(), timezoneOffset);
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

time_t LightTimer::parseTimeFromToday(int hour, int minute, int offset)
{
    // For simplicity, we're assuming that the times are for today.
    time_t t = Time.now();
    struct tm* timeinfo;

    // Alter the timestamp with the sunset time.
    timeinfo = localtime(&t);
    // For some reason, the date gets parsed out a day ahead than it really is,
    // so adjust it.
    timeinfo->tm_mday -= 1;
    timeinfo->tm_hour = hour - offset;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = 0;

    // Return unix timestamp of sunset.
    return mktime(timeinfo);
}

time_t LightTimer::parseTimeFromString(const char* data, int offset)
{
    int hour, minute;
    LightTimer::parseTime(data, &hour, &minute);
    return LightTimer::parseTimeFromToday(hour, minute, offset);
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
