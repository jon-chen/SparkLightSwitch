#include "SparkDebug.h"
#include "Sparky.h"
#include "LightTimer.h"

const int outletSwitchPin = D7;
#ifdef SPARK_DEBUG
const String astronomyApiUrl = "http://spark-light-timer.googlecode.com/git/sample-astronomy.json";
#else
const String astronomyApiUrl = "http://api.wunderground.com/api/91329161b08b1483/astronomy/q/30329.json";
#endif
const String sunsetApiCheckTime = "15:00";
const String outletSwitchOnTime = "sunset";
const String outletSwitchOffTime = "23:59";
const int timezoneOffset = -5; // EST

LightTimer* timer;
SwitchSchedulerConfiguration* config;
char currentState[StringVariableMaxLength] = "{\"error\":\"not initialized\"}";

void setup()
{
    config = new SwitchSchedulerConfiguration();
    config->astronomyApiUrl = astronomyApiUrl;
    config->astronomyApiCheckTime = sunsetApiCheckTime;
    config->timezoneOffset = timezoneOffset;

    timer = new LightTimer(config);
    timer->setOutletSwitchPin(outletSwitchPin);
    timer->addSchedule(new SwitchSchedulerTask(outletSwitchOnTime, outletSwitchOffTime, &schedulerHandler));
    // timer->addSchedule(new SwitchSchedulerTask("10:42", "10:43", &schedulerHandler));
    // timer->addSchedule(new SwitchSchedulerTask("10:44", "10:45", &schedulerHandler));
    // timer->addSchedule(new SwitchSchedulerTask("10:46", "10:47", &schedulerHandler));

    Spark.function("configure", configureHandler);
    Spark.function("identify", identifyHandler);
    Spark.variable("current", &currentState, STRING);

    #ifdef SPARK_DEBUG
    Serial.begin(9600);

    while(!Serial.available())
    {
        SPARK_WLAN_Loop();
    }

    Serial.print("System started...");
    Serial.print(Time.timeStr());
    #endif

    timer->initialize();
}

void loop()
{
    timer->tick();

    // // Try to get the sunset data if it's time.
    // timer.retrieveSunsetData();

    // Turn on the outlet switch if it's time.
    // timer->toggleOutletSwitchOn();

    // Turn off the outlet switch if it's time.
    // timer->toggleOutletSwitchOff();

    char* tmp = const_cast<char*>(timer->getCurrentState());
    strcpy(currentState, tmp);

    // DEBUG_PRINT(currentState);
    // DEBUG_PRINT("\n");

    // Wait 10 seconds
    delay(10000);
}

void schedulerHandler(int state)
{
    timer->schedulerCallback(static_cast<SwitchSchedulerEvent>(state));
}

int configureHandler(String command)
{
    return timer->configureHandler(command);
}

int identifyHandler(String command)
{
    Sparky::DoTheRainbow();
    return 1;
}
