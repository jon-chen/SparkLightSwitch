#include "SparkDebug.h"
#include "Sparky.h"
#include "LightTimer.h"

#ifdef SPARK_DEBUG
const String astronomyApiUrl = "http://spark-light-timer.googlecode.com/git/sample-astronomy.json";
#else
const String astronomyApiUrl = "http://api.wunderground.com/api/91329161b08b1483/astronomy/q/30329.json";
#endif

LightTimer* timer;
SwitchSchedulerConfiguration* config;
char currentState[StringVariableMaxLength] = "{\"error\":\"not initialized\"}";

void setup()
{
    config = new SwitchSchedulerConfiguration();
    config->astronomyApiUrl = astronomyApiUrl;
    config->astronomyApiCheckTime = "03:00";
    config->timezoneOffset = -5; // EST

    timer = new LightTimer(config);
    timer->setOutletSwitchPin(D7);
    timer->addSchedule(new SwitchSchedulerTask("sunset", "23:59", &schedulerHandler));
    // timer->addSchedule(new SwitchSchedulerTask("10:42", "10:43", &schedulerHandler));
    // timer->addSchedule(new SwitchSchedulerTask("10:44", "10:45", &schedulerHandler));
    // timer->addSchedule(new SwitchSchedulerTask("10:46", "10:47", &schedulerHandler));

    Spark.function("configure", configureHandler);
    Spark.function("identify", identifyHandler);
    Spark.variable("current", &currentState, STRING);

    #ifdef SPARK_DEBUG
    RGB.control(true);
    RGB.color(255,0,0);

    Serial.begin(9600);

    while(!Serial.available())
    {
        SPARK_WLAN_Loop();
    }

    RGB.control(false);
    Serial.print("System started...");
    Serial.print(Time.timeStr());
    #endif

    timer->initialize();
}

void loop()
{
    timer->tick();

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
