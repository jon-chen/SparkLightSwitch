#ifndef SWITCH_SCHEDULER_H_
#define SWITCH_SCHEDULER_H_

// #include <functional>
#include "application.h"
#include "SparkTime.h"

struct SwitchSchedulerConfiguration
{
    int TimezoneOffset;
    String AstronomyApiUrl;
    String AstronomyApiCheckTime;
};

class SwitchSchedulerTask
{
    public:
        SwitchSchedulerTask(String,String);
        String StartTime;
        String EndTime;
        // function<void(String)> CallbackHandler;
    private:

};

enum SwitchSchedulerEvent
{
    StartEvent,
    EndEvent
};

class SwitchScheduler
{
    public:
        // ctor
        SwitchScheduler(SwitchSchedulerConfiguration*);

        void SetTimezoneOffset(int offset);

        void SetAstronomyApiUrl(String apiUrl);

        void SetAstronomyApiCheckTime(String checkTime);

        bool IsUsingAstronomyData();

        time_t GetSunriseTime();

        time_t GetSunsetTime();

        unsigned long GetLastTimeSync();

        SwitchSchedulerConfiguration* GetConfiguration();

        SwitchSchedulerTask** GetTasks();

        int GetTasksLength();

        bool ShouldBeEnabled();

        // Adds a task that the scheduler should keep track of.
        void AddSchedulerTask(SwitchSchedulerTask*);

        // To be called in the Spark loop.
        void Tock();
    private:
        // A day in milliseconds.
        const unsigned long dayInMilliseconds = (24 * 60 * 60 * 1000);

        // Reference to SparkTime. Mostly using it just for the DST testing
        // capabilities.
        SparkTime rtc;

        // The last time the time was sync'd.
        unsigned long lastTimeSync;

        unsigned long lastLoopCheck;

        unsigned long checkLoopInterval = (60 * 1000); // every 1 minute

        bool isUsingAstronomyData;

        time_t sunriseTime;

        // The time that the sunset will occur for today.
        time_t sunsetTime;

        SwitchSchedulerConfiguration* configuration;

        SwitchSchedulerTask** tasks;

        int tasksLength;

        void Initialize(SwitchSchedulerConfiguration*);

        void CheckSchedulerTasks();

        void CheckSchedulerTask(SwitchSchedulerTask*, String, SwitchSchedulerEvent);

        // Try to sync the time. It will only sync once a day.
        void SyncTime();

        // Try to get the astronomy data. It will only get it at the
        // configured time.
        void RetrieveAstronomyData();

        // Retrieve the sunset data from the API and parse it into
        // a unix timestamp.
        bool ParseAndSetAstronomyData();

        // Retrieve the sunset data from the API.
        const char* GetAstronomyDataResponse();

        time_t GetTime(String);
};

#endif // SWITCH_SCHEDULER_H_
