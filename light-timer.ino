#define ARDUINO
#define SPARK_DEBUG

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
    config->AstronomyApiUrl = astronomyApiUrl;
    config->AstronomyApiCheckTime = sunsetApiCheckTime;
    config->TimezoneOffset = timezoneOffset;

    timer = new LightTimer(config);
    timer->setOutletSwitchPin(outletSwitchPin);
    timer->AddSchedule(new SwitchSchedulerTask(outletSwitchOnTime, outletSwitchOffTime));
    timer->AddSchedule(new SwitchSchedulerTask("19:40", "19:41"));
    timer->AddSchedule(new SwitchSchedulerTask("19:42", "19:43"));

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

    timer->Initialize();
}

void loop()
{
    timer->Tick();

    // // Try to get the sunset data if it's time.
    // timer.retrieveSunsetData();

    // Turn on the outlet switch if it's time.
    // timer->toggleOutletSwitchOn();

    // Turn off the outlet switch if it's time.
    // timer->toggleOutletSwitchOff();

    char* tmp = const_cast<char*>(timer->GetCurrentState());
    strcpy(currentState, tmp);

    // DEBUG_PRINT(currentState);
    // DEBUG_PRINT("\n");

    // Wait 10 seconds
    delay(10000);
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
