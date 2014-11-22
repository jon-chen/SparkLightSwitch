#include "application.h"
#include "SparkDebug.h"
#include "SparkTime.h"
#include "Sparky.h"
#include "SwitchScheduler.h"
#include "JsonGenerator.h"
#include "JsonParser.h"
#include "LightSwitch.h"

LightSwitch::LightSwitch(SwitchSchedulerConfiguration* config, SparkTime* rtc)
{
    scheduler = new SwitchScheduler(config, rtc);
    this->rtc = rtc;
}

void LightSwitch::initialize()
{
    // If the switch is off but should be on by our calculations.
    if (scheduler->isSchedulerEnabled() &&
        scheduler->shouldBeToggled() && !getOutletSwitchState())
    {
        DEBUG_PRINT("Enabling switch since it should be on.\n");
        toggleOutletSwitch(true);
    }
}

void LightSwitch::addSchedule(SwitchSchedulerTask* task)
{
    scheduler->addSchedulerTask(task);
}

void LightSwitch::tick()
{
    scheduler->tock();
}

void LightSwitch::setOutletSwitchPin(int pin)
{
    outletSwitchPin = pin;

    // Initialize pin.
    pinMode(outletSwitchPin, OUTPUT);
    toggleOutletSwitch(false);
}

const char* LightSwitch::getCurrentState()
{
    ArduinoJson::Generator::JsonObject<15> root;
    SwitchSchedulerConfiguration* config = scheduler->getConfiguration();

    // current time
    String now = rtc->ISODateString(rtc->now());
    root["time"] = now.c_str();

    root["isDst"] = rtc->isUSDST(rtc->now());

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
    // root["useAstronomyData"] = scheduler->isUsingAstronomyData();
    String sunset = Sparky::ISODateString(rtc, scheduler->getSunsetTime());
    root["sunsetTime"] = sunset.c_str();

    String sunrise = Sparky::ISODateString(rtc, scheduler->getSunriseTime());
    root["sunriseTime"] = sunrise.c_str();

    // timezone offset
    // root["timezoneOffset"] = config->timezoneOffset;

    // last time sync
    // root["lastTimeSync"] = utoa(scheduler->getLastTimeSync());

    root["currentSwitchState"] = getOutletSwitchState();

    root["shouldSwitchBeToggled"] = scheduler->shouldBeToggled();

    root["isSchedulerEnabled"] = config->isEnabled;

    root["isHomeOnlyModeEnabled"] = config->homeOnlyModeEnabled;

    root["currentHomeCount"] = scheduler->getCurrentHomeCount();

    char buffer[StringVariableMaxLength];
    root.printTo(buffer, sizeof(buffer));

    return String(buffer).c_str();
}

int LightSwitch::configureHandler(String command)
{
    #ifdef SPARK_DEBUG
    Serial.println("Outlet Control API request received...");
    #endif

    ArduinoJson::Parser::JsonParser<64> parser;
    ArduinoJson::Parser::JsonObject root = parser.parse(const_cast<char*>(command.c_str()));

    ArduinoJson::Parser::JsonValue value, mobileId;
    value = root[configKeys[LightSwitchConfig::IsToggled]];
    if (value.success())
    {
        toggleOutletSwitch(value);
    }

    // value = root[configKeys[LightSwitchConfig::TimezoneOffset]];
    // if (value.success())
    // {
    //     scheduler->setTimezoneOffset(atoi(value));
    // }

    value = root[configKeys[LightSwitchConfig::SunsetApiUrl]];
    if (value.success())
    {
        scheduler->setAstronomyApiUrl((const char*)value);
    }

    value = root[configKeys[LightSwitchConfig::SunsetApiCheckTime]];
    if (value.success())
    {
        scheduler->setAstronomyApiCheckTime((const char*)value);
    }

    value = root[configKeys[LightSwitchConfig::IsSchedulerEnabled]];
    if (value.success())
    {
        scheduler->setIsEnabled(value);
    }

    value = root[configKeys[LightSwitchConfig::IsHomeOnlyModeEnabled]];
    if (value.success())
    {
        scheduler->setHomeOnlyModeEnabled(value);
    }

    value = root[configKeys[LightSwitchConfig::HomeStatus]];
    mobileId = root[configKeys[LightSwitchConfig::MobileId]];
    if (value.success() && mobileId.success())
    {
        switch ((int)value)
        {
            case HomeStatus::Home:
                scheduler->setHomeStatus((const char*)mobileId);
                break;
            case HomeStatus::Away:
                scheduler->setAwayStatus((const char*)mobileId);
                break;
            case HomeStatus::Reset:
                scheduler->resetHomeStatus();
                break;
        }
    }

    // value = root[configKeys[OutletSwitchOffTime]];
    // if (value.success())
    // {
    //     setOutletSwitchOffTime((const char*)value);
    // }

    // TODO: modify schedule from mobile app
    // set on/off time - delete all scheduled times
    // and add array of scheduled times from request
    // mobile app should have a schedule list view
    // where schedules can be added/removed/modified

    return 1;
}

void LightSwitch::toggleOutletSwitch(bool isToggled)
{
    int value = isToggled ? HIGH : LOW;
    digitalWrite(outletSwitchPin, value);
    outletSwitchState = isToggled;
    Spark.publish("ToggleState", String(isToggled));
    lastToggleOutletSwitchTime = rtc->now();
}

bool LightSwitch::getOutletSwitchState()
{
    return outletSwitchState;
}

void LightSwitch::schedulerCallback(SwitchSchedulerEvent::SwitchSchedulerEventEnum state)
{
    toggleOutletSwitch(state == SwitchSchedulerEvent::StartEvent);
}
