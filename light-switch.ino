#include "SparkDebug.h"
#include "Sparky.h"
#include "SparkTime.h"
#include "LightSwitch.h"

#ifdef SPARK_DEBUG
const String astronomyApiUrl = "http://spark-light-timer.googlecode.com/git/sample-astronomy.json";
#else
const String astronomyApiUrl = "http://api.wunderground.com/api/91329161b08b1483/astronomy/q/30329.json";
#endif

LightSwitch* ls;
UDP* udpClient;
SparkTime* rtc;
SwitchSchedulerConfiguration* config;
char currentState[StringVariableMaxLength] = "{\"error\":\"not initialized\"}";

void setup()
{
    udpClient = new UDP();
    rtc = new SparkTime();

    Time.zone(-5);
    rtc->setTimeZone(-5); // est
    rtc->begin(udpClient, "north-america.pool.ntp.org");

    // TODO: retrieve intial configuration from the web
    config = new SwitchSchedulerConfiguration();
    config->astronomyApiUrl = astronomyApiUrl;
    config->astronomyApiCheckTime = "03:00";
    config->isEnabled = true;
    config->homeOnlyModeEnabled = false;
    // config->homeOnlyModeEnabled = true;

    ls = new LightSwitch(config, rtc);
    ls->setOutletSwitchPin(D7);
    ls->addSchedule(new SwitchSchedulerTask("sunset", "23:59", &schedulerHandler));
    // ls->addSchedule(new SwitchSchedulerTask("sunset", "02:30", &schedulerHandler));
    // ls->addSchedule(new SwitchSchedulerTask("23:35", "23:40", &schedulerHandler));
    // ls->addSchedule(new SwitchSchedulerTask("23:45", "23:50", &schedulerHandler));
    // ls->addSchedule(new SwitchSchedulerTask("23:55", "00:00", &schedulerHandler));

    Spark.function("configure", configureHandler);
    Spark.function("identify", identifyHandler);
    Spark.variable("current", &currentState, STRING);

    #ifdef SPARK_DEBUG
    RGB.control(true);
    RGB.color(255,255,0);

    Serial.begin(9600);

    while(!Serial.available())
    {
        SPARK_WLAN_Loop();
    }

    RGB.control(false);
    Serial.print("System started...");
    Serial.print(rtc->ISODateString(rtc->now()));
    #endif


    while (!rtc->hasSynced())
    {
        DEBUG_PRINT("Has synced: ");
        DEBUG_PRINT(rtc->hasSynced());
        DEBUG_PRINT("\n");

        rtc->now();
        SPARK_WLAN_Loop();
        delay(1000);
    }

    ls->initialize();
}

void loop()
{
    ls->tick();

    char* tmp = const_cast<char*>(ls->getCurrentState());
    strcpy(currentState, tmp);

    // DEBUG_PRINT(currentState);
    // DEBUG_PRINT("\n");

    // Wait 10 seconds
    // delay(10000);
}

void schedulerHandler(int state)
{
    ls->schedulerCallback(
        static_cast<SwitchSchedulerEvent::SwitchSchedulerEventEnum>(state));
}

int configureHandler(String command)
{
    return ls->configureHandler(command);
}

int identifyHandler(String command)
{
    Sparky::DoTheRainbow();
    return 1;
}
