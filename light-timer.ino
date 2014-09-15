#include "rest_client.h"
#include "LightTimer.h"

LightTimer timer;

void setup()
{
    // Make sure your Serial Terminal app is closed before powering your Core
    Serial.begin(9600);
    // Now open your Serial Terminal, and hit any key to continue!
    while(!Serial.available()) SPARK_WLAN_Loop();

    timer.retrieveSunsetData();
}

void loop()
{
    timer.syncTime();
    // timer.retrieveSunsetData();
    delay(5000);
}
