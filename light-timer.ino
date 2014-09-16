#define SPARK_DEBUG

#include "rest_client.h"
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
const String timerOffTime = "23:59";
const int timezoneOffset = -5; // EST

LightTimer timer;

void setup()
{
    // Initialize LightTimer.
    timer.setOutletSwitchPin(outletSwitchPin);
    timer.setSunsetApiUrl(weatherApiBaseUrl, sunsetApiUrl);
    timer.setSunsetApiCheckTime(sunsetApiCheckTime);
    timer.setTimerOffTime(timerOffTime);

    Spark.function("outletControl", outletControl);

    #ifdef SPARK_DEBUG
    Serial.begin(9600);

    while(!Serial.available())
    {
        SPARK_WLAN_Loop();
    }

    Serial.print("System started... ");
    Serial.print(Time.timeStr());
    #endif
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

    delay(5000);
}

// Outlet control handler.
int outletControl(String command)
{
    #ifdef SPARK_DEBUG
    Serial.println("Outlet Control API request received...");
    #endif

    bool enable = (command == "true" ||
        command == "1" ||
        command == "enable");
    bool disable = (command == "false" ||
        command == "0" ||
        command == "disable");

    if (enable)
    {
        #ifdef SPARK_DEBUG
        Serial.println("Enabling outlet switch...");
        #endif
        timer.toggleOutletSwitch(true);
        return 1;
    }
    else if (disable)
    {
        #ifdef SPARK_DEBUG
        Serial.println("Disabling outlet switch...");
        #endif
        timer.toggleOutletSwitch(off);
        return 1;
    }

    return -1;
}
