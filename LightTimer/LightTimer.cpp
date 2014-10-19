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

void LightTimer::initialize()
{
    // If the switch is off but should be on by our calculations.
    if (scheduler->shouldBeEnabled() && !getOutletSwitchState())
    {
        DEBUG_PRINT("Enabling switch since it should be on.\n");
        toggleOutletSwitch(true);
    }
}

void LightTimer::addSchedule(SwitchSchedulerTask* task)
{
    scheduler->addSchedulerTask(task);
}

void LightTimer::tick()
{
    scheduler->tock();
}

void LightTimer::setOutletSwitchPin(int pin)
{
    outletSwitchPin = pin;

    // Initialize pin.
    pinMode(outletSwitchPin, OUTPUT);
    toggleOutletSwitch(false);
}

const char* LightTimer::getCurrentState()
{
    ArduinoJson::Generator::JsonObject<10> root;
    SwitchSchedulerConfiguration* config = scheduler->getConfiguration();

    // current time
    root["time"] = Time.now();

    // switch schedules
    ArduinoJson::Generator::JsonArray<10> tasksArray;
    ArduinoJson::Generator::JsonObject<2> taskObjects[10];
    int tasksLength = scheduler->getTasksLength();
    SwitchSchedulerTask** tasks = scheduler->getTasks();

    for (int i = 0; i < tasksLength; i++)
    {
        taskObjects[i]["startTime"] = tasks[i]->startTime.c_str();
        taskObjects[i]["endTime"] = tasks[i]->endTime.c_str();
        tasksArray.add(taskObjects[i]);
    }

    root["schedules"] = tasksArray;

    // is using astronomy data?
    root["useAstronomyData"] = scheduler->isUsingAstronomyData();
    root["sunsetTime"] = scheduler->getSunsetTime();
    root["sunriseTime"] = scheduler->getSunriseTime();

    // timezone offset
    root["timezoneOffset"] = config->timezoneOffset;

    // last time sync
    // root["lastTimeSync"] = utoa(scheduler->getLastTimeSync());

    root["currentSwitchState"] = getOutletSwitchState();

    root["shouldSwitchBeEnabled"] = scheduler->shouldBeEnabled();

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
        scheduler->setTimezoneOffset(value);
    }

    value = root[configKeys[SunsetApiUrl]];
    if (value.success())
    {
        scheduler->setAstronomyApiUrl((const char*)value);
    }

    value = root[configKeys[SunsetApiCheckTime]];
    if (value.success())
    {
        scheduler->setAstronomyApiCheckTime((const char*)value);
    }

    // value = root[configKeys[OutletSwitchOffTime]];
    // if (value.success())
    // {
    //     setOutletSwitchOffTime((const char*)value);
    // }

    return 1;
}

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

void LightTimer::schedulerCallback(SwitchSchedulerEvent state)
{
    toggleOutletSwitch(state == StartEvent);
}
