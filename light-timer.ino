#include "LightTimer.h"
#include <math.h>

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

LightTimer timer;
char* currentState = "tmp";

void setup()
{
    // Initialize LightTimer.
    timer.setOutletSwitchPin(outletSwitchPin);
    timer.setSunsetApiUrl(weatherApiBaseUrl, sunsetApiUrl);
    timer.setSunsetApiCheckTime(sunsetApiCheckTime);
    timer.setOutletSwitchOnTime(outletSwitchOnTime);
    timer.setOutletSwitchOffTime(outletSwitchOffTime);
    timer.setTimezoneOffset(timezoneOffset);

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

    timer.initialize();
}

void loop()
{
    // Sync the time daily.
    timer.syncTime();

    // Try to get the sunset data if it's time.
    timer.retrieveSunsetData();

    // Turn on the outlet switch if it's time.
    timer.toggleOutletSwitchOn();

    // Turn off the outlet switch if it's time.
    timer.toggleOutletSwitchOff();

    // TODO: This fails to get set correctly
    timer.getCurrentState(currentState);

    // DEBUG_PRINT(currentState);

    // Wait 10 seconds
    delay(10000);
}

int configureHandler(String command)
{
    return timer.configureHandler(command);
}

int identifyHandler(String command)
{
    RGB.control(true);

    // Make it rainbow!
    // http://krazydad.com/tutorials/makecolors.php
    double frequency = 2 * 3.14 / 6;
    int r, g, b;

    for (int i = 0; i < 92; ++i)
    {
        r = sin(frequency * i + 0) * 127 + 128;
        g = sin(frequency * i + 2) * 127 + 128;
        b = sin(frequency * i + 4) * 127 + 128;
        RGB.color(r,g,b);
        delay(50);
    }

    RGB.control(false);

    return 1;
}
