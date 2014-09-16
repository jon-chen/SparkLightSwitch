#include "application.h"
#include "SparkTime.h"

class LightTimer
{
    public:
        // Constructor.
        LightTimer();

        // Initailize the pin used to control the outlet switch.
        void setOutletSwitchPin(int pin);

        // Initialize the timezone offset.
        void setTimezoneOffset(int offset);

        // Initialize the sunset API urls.
        void setSunsetApiUrl(String baseUrl, String apiUrl);

        // Initialize when the app checks for sunset data.
        void setSunsetApiCheckTime(String timeString);

        // Initalize when the app turns off the outlet switch.
        void setTimerOffTime(String timeString);

        // Check to see if the outlet switch needs to be turned on. If so,
        // turn it on!
        void toggleOutletSwitchOn();

        // Check ot see if the outlet switch needs to be turned off. If so,
        // turn it off!
        void toggleOutletSwitchOff();

        // Try to sync the time. It will only sync once a day.
        void syncTime();

        // Try to get the sunset data. It will only get it at the configured
        // time.
        void retrieveSunsetData();

        // Toggle the outlet switch on or off based on the parameter. True
        // for on, false otherwise.
        void toggleOutletSwitch(bool isEnabled);

        // Get the current state of the outlet switch. True for on, false
        // otherwise.
        bool getOutletSwitchState();

        // Checks to see if the outlet switch should be toggled on.
        bool shouldOutletSwitchBeEnabled();
    private:
        // Reference to SparkTime. Mostly using it just for the DST testing
        // capabilities.
        SparkTime rtc;

        // The last time the time was sync'd.
        unsigned long lastTimeSync;

        // The timezone offset for the device.
        int timezoneOffset;

        // The time that the sunset will occur for today.
        time_t sunsetTime;

        // The last time the outlet switch was set to on.
        time_t lastToggleOutletSwitchOnTime;

        // The last time the outlet switch was set to off.
        time_t lastToggleOutletSwitchOffTime;

        // The last time the sunset API was checked.
        time_t lastSunsetApiCheckTime;

        // When the app checks for sunset data.
        String sunsetApiCheckTime;

        // When the app turns off the outlet switch.
        String timerOffTime;

        // Pin used to control whether the outlet switch is enabled or not.
        int outletSwitchPin;

        // The current state of the outlet switch.
        bool outletSwitchState;

        // Weather API url strings.
        String weatherApiBaseUrl;
        String sunsetApiUrl;

        // A day in milliseconds.
        const unsigned long dayInMilliseconds = (24 * 60 * 60 * 1000);

        // Retrieve the sunset data from the API and parse it into
        // a unix timestamp.
        time_t getAndParseSunsetData();

        // Retrieve the sunset data from the API.
        const char* getSunsetDataResponse();

        // Parse the time into a unix timestamp.
        static time_t parseTimeFromToday(int hour, int minute);

        // Parse the time from a string to a unix timestamp.
        static time_t parseTimeFromString(const char* data);

        // Parses a time out of the string in the format h:mm
        static void parseTime(const char* data, int* hour, int* minute);

        // Parse a unix timestamp and retrieve the hour and minute.
        static void parseTimestamp(time_t timestamp, int* hour, int* minute);
};
