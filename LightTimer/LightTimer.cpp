#include "SparkDebug.h"
#include "Sparky.h"
#include "JsonParser.h"
#include "LightTimer.h"

LightTimer::LightTimer(SwitchSchedulerConfiguration* config)
{
    scheduler = new SwitchScheduler(config);
}

void LightTimer::Initialize()
{
    // If the switch is off but should be on by our calculations.
    if (shouldOutletSwitchBeEnabled() && !getOutletSwitchState())
    {
        DEBUG_PRINT("Enabling switch since it should be on.\n");
        toggleOutletSwitch(true);
        lastToggleOutletSwitchOnTime = Time.now();
    }
}

void LightTimer::AddSchedule(SwitchSchedulerTask* task)
{
    // TODO: Add callback handler to task
    scheduler->AddSchedulerTask(task);
}

void LightTimer::Tick()
{
    scheduler->Tock();
}

void LightTimer::setOutletSwitchPin(int pin)
{
    outletSwitchPin = pin;

    // Initialize pin.
    pinMode(outletSwitchPin, OUTPUT);
    toggleOutletSwitch(false);
}

// void LightTimer::setSunsetApiUrl(String baseUrl, String apiUrl)
// {
//     weatherApiBaseUrl = baseUrl;
//     sunsetApiUrl = apiUrl;
// }
//
// void LightTimer::setSunsetApiCheckTime(String timeString)
// {
//     sunsetApiCheckTime = timeString;
// }

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

    DEBUG_PRINT("Use astronomy data: ");
    DEBUG_PRINT(scheduler->IsUsingAstronomyData());
    DEBUG_PRINT("\n");

    // DEBUG_PRINT("Sunset time: ");
    // DEBUG_PRINT(Time.timeStr(sunsetTime));

    DEBUG_PRINT("Timezone offset: ");
    DEBUG_PRINT(scheduler->GetConfiguration()->TimezoneOffset);
    DEBUG_PRINT("\n");

    DEBUG_PRINT("Last time sync: ");
    DEBUG_PRINT(Time.timeStr(scheduler->GetLastTimeSync()));

    // DEBUG_PRINT("Last sunset API check: ");
    // DEBUG_PRINT(Time.timeStr(Sparky::ParseTimeFromString(
    //     sunsetApiCheckTime.c_str(), scheduler->GetConfiguration()->TimezoneOffset)));

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

    ArduinoJson::Parser::JsonParser<64> parser;
    ArduinoJson::Parser::JsonObject root = parser.parse(const_cast<char*>(command.c_str()));

    ArduinoJson::Parser::JsonValue value;
    value = root[configKeys[OutletSwitchState]];
    if (value.success())
    {
        toggleOutletSwitch(value);
    }

    value = root[configKeys[TimezoneOffset]];
    if (value.success())
    {
        scheduler->SetTimezoneOffset(value);
    }

    value = root[configKeys[SunsetApiUrl]];
    if (value.success())
    {
        scheduler->SetAstronomyApiUrl((const char*)value);
    }

    value = root[configKeys[SunsetApiCheckTime]];
    if (value.success())
    {
        scheduler->SetAstronomyApiCheckTime((const char*)value);
        // setSunsetApiCheckTime((const char*)value);
    }

    value = root[configKeys[OutletSwitchOffTime]];
    if (value.success())
    {
        setOutletSwitchOffTime((const char*)value);
    }

    return 1;
}

void LightTimer::toggleOutletSwitchOn()
{
    int checkHour, checkMinute;
    time_t now = Time.now();
    time_t checkTime = getOutletSwitchOnTime();
    Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);

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
    Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);

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

// void LightTimer::retrieveSunsetData()
// {
//     // Only retrieve sunset data if we actually need it.
//     if (useSunsetOnTime())
//     {
//         time_t now = Time.now();
//         int checkHour, checkMinute;
//         Sparky::ParseTime(sunsetApiCheckTime.c_str(), &checkHour, &checkMinute);
//
//         // If the check time has arrived and the last check time wasn't this
//         // minute, or if we've never retrieved the sunset data yet.
//         if ((Time.hour() == checkHour && Time.minute() == checkMinute &&
//             now / (1000 * 60) != lastSunsetApiCheckTime / (1000 * 60)) ||
//             lastSunsetApiCheckTime == 0)
//         {
//             DEBUG_PRINT("Retrieving sunset data... ");
//             DEBUG_PRINT(Time.timeStr() + "\n");
//
//             time_t time = getAndParseSunsetData();
//             if (time != -1)
//             {
//                 DEBUG_PRINT("Sunset data retrieved! ");
//                 DEBUG_PRINT(Time.timeStr() + "\n");
//                 DEBUG_PRINT("Sunset is at: ");
//                 DEBUG_PRINT(Time.timeStr(time) + " ");
//                 DEBUG_PRINT(time);
//                 DEBUG_PRINT("\n");
//
//                 sunsetTime = time;
//                 lastSunsetApiCheckTime = now;
//             }
//             else
//             {
//                 DEBUG_PRINT("Failed to retrieve sunset data, retrying in a few seconds...");
//                 delay(5000);
//                 retrieveSunsetData();
//             }
//         }
//     }
// }
//
// time_t LightTimer::getAndParseSunsetData()
// {
//     const char* response = getSunsetDataResponse();
//
//     // Parse response if we have one.
//     if (strlen(response) != 0)
//     {
//         JsonParser<64> parser;
//         JsonObject root = parser.parse(const_cast<char*>(response));
//
//         if(root.success())
//         {
//             int hour = atoi(root["moon_phase"]["sunset"]["hour"]);
//             int minute = atoi(root["moon_phase"]["sunset"]["minute"]);
//             return Sparky::ParseTimeFromToday(hour, minute, timezoneOffset);
//         }
//     }
//
//     return -1;
// }

time_t LightTimer::getOutletSwitchOffTime()
{
    return 0;
    // return Sparky::ParseTimeFromString(
    //     outletSwitchOffTime.c_str(),
    //     scheduler->GetConfiguration()->TimezoneOffset);
}

// bool LightTimer::useSunsetOnTime()
// {
//     return outletSwitchOnTime == "sunset";
// }

time_t LightTimer::getOutletSwitchOnTime()
{
    return 0;
    // if (scheduler->IsUsingAstronomyData())
    // {
    //     return sunsetTime;
    // }
    //
    // return Sparky::ParseTimeFromString(
    //     outletSwitchOnTime.c_str(),
    //     scheduler->GetConfiguration()->TimezoneOffset);
}

// const char* LightTimer::getSunsetDataResponse()
// {
//     RestClient client = RestClient(weatherApiBaseUrl.c_str());
//     String response = "";
//
//     int statusCode = client.get(sunsetApiUrl.c_str(), &response);
//
//     if (statusCode == 200)
//     {
//         DEBUG_PRINT("Successfully retrieved JSON response\n");
//         return response.c_str();
//     }
//     else
//     {
//         DEBUG_PRINT("Failed to retrieve sunset data with status: ");
//         DEBUG_PRINT(statusCode);
//         DEBUG_PRINT("\n");
//         return "";
//     }
// }
