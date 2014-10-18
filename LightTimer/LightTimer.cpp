#include "application.h"
#include "SparkDebug.h"
#include "Sparky.h"
#include "JsonGenerator.h"
#include "JsonParser.h"
#include "LightTimer.h"

LightTimer::LightTimer(SwitchSchedulerConfiguration* config)
{
    scheduler = new SwitchScheduler(config);
}

void LightTimer::Initialize()
{
    // If the switch is off but should be on by our calculations.
    if (scheduler->ShouldBeEnabled() && !getOutletSwitchState())
    {
        DEBUG_PRINT("Enabling switch since it should be on.\n");
        toggleOutletSwitch(true);
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

void LightTimer::setOutletSwitchOnTime(String timeString)
{
    outletSwitchOnTime = timeString;
}

void LightTimer::setOutletSwitchOffTime(String timeString)
{
    outletSwitchOffTime = timeString;
}

const char* LightTimer::GetCurrentState()
{
    ArduinoJson::Generator::JsonObject<10> root;
    SwitchSchedulerConfiguration* config = scheduler->GetConfiguration();

    // current time
    root["time"] = Time.now();

    // switch schedules
    ArduinoJson::Generator::JsonArray<10> tasksArray;
    ArduinoJson::Generator::JsonObject<2> taskObjects[10];
    int tasksLength = scheduler->GetTasksLength();
    SwitchSchedulerTask** tasks = scheduler->GetTasks();

    for (int i = 0; i < tasksLength; i++)
    {
        taskObjects[i]["startTime"] = tasks[i]->StartTime.c_str();
        taskObjects[i]["endTime"] = tasks[i]->EndTime.c_str();
        tasksArray.add(taskObjects[i]);
    }

    root["schedules"] = tasksArray;

    // is using astronomy data?
    root["useAstronomyData"] = scheduler->IsUsingAstronomyData();
    root["sunsetTime"] = scheduler->GetSunsetTime();
    root["sunriseTime"] = scheduler->GetSunriseTime();

    // timezone offset
    root["timezoneOffset"] = config->TimezoneOffset;

    // last time sync
    // root["lastTimeSync"] = utoa(scheduler->GetLastTimeSync());

    root["currentSwitchState"] = getOutletSwitchState();

    root["shouldSwitchBeEnabled"] = scheduler->ShouldBeEnabled();

    //
    // DEBUG_PRINT("Current time: ");
    // DEBUG_PRINT(Time.timeStr());
    //
    // DEBUG_PRINT("Time On: ");
    // DEBUG_PRINT(Time.timeStr(getOutletSwitchOnTime()));
    //
    // DEBUG_PRINT("Time Off: ");
    // DEBUG_PRINT(Time.timeStr(getOutletSwitchOffTime()));
    //
    // DEBUG_PRINT("Use astronomy data: ");
    // DEBUG_PRINT(scheduler->IsUsingAstronomyData());
    // DEBUG_PRINT("\n");
    //
    // // DEBUG_PRINT("Sunset time: ");
    // // DEBUG_PRINT(Time.timeStr(sunsetTime));
    //
    // DEBUG_PRINT("Timezone offset: ");
    // DEBUG_PRINT(scheduler->GetConfiguration()->TimezoneOffset);
    // DEBUG_PRINT("\n");
    //
    // DEBUG_PRINT("Last time sync: ");
    // DEBUG_PRINT(Time.timeStr(scheduler->GetLastTimeSync()));
    //
    // // DEBUG_PRINT("Last sunset API check: ");
    // // DEBUG_PRINT(Time.timeStr(Sparky::ParseTimeFromString(
    // //     sunsetApiCheckTime.c_str(), scheduler->GetConfiguration()->TimezoneOffset)));
    //
    // DEBUG_PRINT("Last toggle on time: ");
    // DEBUG_PRINT(Time.timeStr(lastToggleOutletSwitchOnTime));
    //
    // DEBUG_PRINT("Last toggle off time: ");
    // DEBUG_PRINT(Time.timeStr(lastToggleOutletSwitchOffTime));
    //
    // DEBUG_PRINT("Current switch state: ");
    // DEBUG_PRINT(getOutletSwitchState());
    // DEBUG_PRINT("\n");
    //
    // DEBUG_PRINT("Should switch be on: ");
    // DEBUG_PRINT(shouldOutletSwitchBeEnabled());
    // DEBUG_PRINT("\n");
    //
    // DEBUG_PRINT("\n");

    char buffer[StringVariableMaxLength];
    root.printTo(buffer, sizeof(buffer));
    return String(buffer).c_str();
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
    }

    value = root[configKeys[OutletSwitchOffTime]];
    if (value.success())
    {
        setOutletSwitchOffTime((const char*)value);
    }

    return 1;
}

// void LightTimer::toggleOutletSwitchOn()
// {
//     int checkHour, checkMinute;
//     time_t now = Time.now();
//     time_t checkTime = getOutletSwitchOnTime();
//     Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);
//
//     // If the check time has arrived and the last check time wasn't this
//     // minute (so we don't try to turn it on more than once).
//     if (Time.hour() == checkHour && Time.minute() == checkMinute &&
//         now / (1000 * 60) != lastToggleOutletSwitchOnTime / (1000 * 60))
//     {
//         DEBUG_PRINT("Toggling outlet switch on... ");
//         DEBUG_PRINT(Time.timeStr() + "\n");
//
//         toggleOutletSwitch(true);
//         lastToggleOutletSwitchOnTime = now;
//     }
// }
//
// void LightTimer::toggleOutletSwitchOff()
// {
//     int checkHour, checkMinute;
//     time_t now = Time.now();
//     time_t checkTime = getOutletSwitchOffTime();
//     Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);
//
//     // If the check time has arrived and the last check time wasn't this
//     // minute (so we don't try to turn it off more than once).
//     if (Time.hour() == checkHour && Time.minute() == checkMinute &&
//         now / (1000 * 60) != lastToggleOutletSwitchOffTime / (1000 * 60))
//     {
//         DEBUG_PRINT("Toggling outlet switch off... ");
//         DEBUG_PRINT(Time.timeStr() + "\n");
//
//         toggleOutletSwitch(false);
//         lastToggleOutletSwitchOffTime = now;
//     }
// }

void LightTimer::toggleOutletSwitch(bool isEnabled)
{
    int value = isEnabled ? HIGH : LOW;
    digitalWrite(outletSwitchPin, value);
    outletSwitchState = isEnabled;
    lastToggleOutletSwitchTime = Time.now();
}

bool LightTimer::getOutletSwitchState()
{
    return outletSwitchState;
}

// bool LightTimer::shouldOutletSwitchBeEnabled()
// {
//     time_t now = Time.now();
//     time_t lightOn = getOutletSwitchOnTime();
//     time_t lightOff = getOutletSwitchOffTime();
//
//     return  (now > lightOn) && (now < lightOff);
// }
