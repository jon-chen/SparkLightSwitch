#include "SparkDebug.h"
#include "Sparky.h"
#include "LightTimer.h"

const int outletSwitchPin = D7;
#ifdef SPARK_DEBUG
const String weatherApiBaseUrl = "spark-light-timer.googlecode.com";
const String sunsetApiUrl = "/git/sample-astronomy.json";
#else
const String weatherApiBaseUrl = "api.wunderground.com";
const String sunsetApiUrl = "/api/91329161b08b1483/astronomy/q/30329.json";
#endif
const String sunsetApiCheckTime = "15:00";
const String outletSwitchOnTime = "sunset";
const String outletSwitchOffTime = "23:59";
const int timezoneOffset = -5; // EST

LightTimer* timer;
SwitchSchedulerConfiguration* config = new SwitchSchedulerConfiguration();
char* currentState;

void setup()
{
    String astronomyApiUrl = "http://" + weatherApiBaseUrl + sunsetApiUrl;
    config->AstronomyApiUrl = astronomyApiUrl;
    config->AstronomyApiCheckTime = sunsetApiCheckTime;
    config->TimezoneOffset = timezoneOffset;

    timer = new LightTimer(config);

    // Initialize LightTimer.
    timer->setOutletSwitchPin(outletSwitchPin);
    // timer.setSunsetApiUrl(weatherApiBaseUrl, sunsetApiUrl);
    // timer.setSunsetApiCheckTime(sunsetApiCheckTime);
    // timer->setOutletSwitchOnTime(outletSwitchOnTime);
    // timer->setOutletSwitchOffTime(outletSwitchOffTime);
    // timer.setTimezoneOffset(timezoneOffset);

    timer->AddSchedule(new SwitchSchedulerTask(outletSwitchOnTime, outletSwitchOffTime));

    Spark.function("configure", configureHandler);
    Spark.function("identify", identifyHandler);
    // Spark.variable("current", currentState, STRING);

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

    // TODO: This fails to get set correctly
    // timer->getCurrentState(currentState);

    // DEBUG_PRINT(currentState);

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
