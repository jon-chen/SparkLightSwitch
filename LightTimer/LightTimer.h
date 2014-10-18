#include "SwitchScheduler.h"

class LightTimer
{
    public:
        // ctor.
        LightTimer(SwitchSchedulerConfiguration*);

        // Method that configures the initial state of the app. This method
        // assumes that the parameters have been configured previously.
        void initialize();

        void addSchedule(SwitchSchedulerTask*);

        // Initailize the pin used to control the outlet switch.
        void setOutletSwitchPin(int pin);

        void tick();

        // Initialize when teh app turns on the outlet switch.
        // void setOutletSwitchOnTime(String timeString);

        // Initalize when the app turns off the outlet switch.
        // void setOutletSwitchOffTime(String timeString);

        // Gets the current state of the app and sets the string variable
        // passed in to the method.
        const char* getCurrentState();

        // Accepts a JSON string and parses out configuration options that
        // can be set remotely. See configKeys for a list of settings that
        // can be configured.
        int configureHandler(String command);

        // Check to see if the outlet switch needs to be turned on. If so,
        // turn it on!
        // void toggleOutletSwitchOn();

        // Check ot see if the outlet switch needs to be turned off. If so,
        // turn it off!
        // void toggleOutletSwitchOff();

        // Toggle the outlet switch on or off based on the parameter. True
        // for on, false otherwise.
        void toggleOutletSwitch(bool isEnabled);

        // Get the current state of the outlet switch. True for on, false
        // otherwise.
        bool getOutletSwitchState();

        // Checks to see if the outlet switch should be toggled on.
        // bool shouldOutletSwitchBeEnabled();

        void schedulerCallback(SwitchSchedulerEvent);
    private:
        SwitchScheduler* scheduler;

        // Reference to SparkTime. Mostly using it just for the DST testing
        // capabilities.
        // SparkTime rtc;

        // The timezone offset for the device.
        // int timezoneOffset;

        // The time that the sunset will occur for today.
        // time_t sunsetTime;

        // The last time the outlet switch was set to on.
        // time_t lastToggleOutletSwitchOnTime;

        // The last time the outlet switch was set to off.
        // time_t lastToggleOutletSwitchOffTime;

        // // The last time the sunset API was checked.
        // time_t lastSunsetApiCheckTime;

        time_t lastToggleOutletSwitchTime;

        // When the app checks for sunset data.
        // String sunsetApiCheckTime;

        // When the app turns on the outlet switch.
        // String outletSwitchOnTime;

        // When the app turns off the outlet switch.
        // String outletSwitchOffTime;

        // Pin used to control whether the outlet switch is enabled or not.
        int outletSwitchPin;

        // The current state of the outlet switch.
        bool outletSwitchState;

        // Weather API url strings.
        // String weatherApiBaseUrl;
        // String sunsetApiUrl;

        // Used by the code to pull out the right configuration key from
        // the configKeys string array.
        enum LightTimerConfig
        {
            OutletSwitchState = 0,
            TimezoneOffset,
            SunsetApiUrl,
            SunsetApiCheckTime
            // OutletSwitchOffTime
        };

        // Configuration keys for the JSON object used to configure settings
        // remotely.
        const char* configKeys[5] =
        {
            "OutletSwitchState",
            "TimezoneOffset",
            "SunsetApiUrl",
            "SunsetApiCheckTime"
            // "OutletSwitchOffTime"
        };

        // True if the app should turn on the outlet switch by using the
        // sunset time or if it should use a time specified by the
        // configuration.
        // bool useSunsetOnTime();

        // Gets the switch off time as a Unix timestamp.
        // time_t getOutletSwitchOffTime();

        // Gets the switch on time as a Unix timestamp.
        // time_t getOutletSwitchOnTime();
};
