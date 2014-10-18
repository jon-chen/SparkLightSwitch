#include "SparkDebug.h"
#include "Sparky.h"
#include "JsonParser.h"
#include "Uri.h"
#include "rest_client.h"
#include "SwitchScheduler.h"

SwitchSchedulerTask::SwitchSchedulerTask(String startTime, String endTime)
{
    StartTime = startTime;
    EndTime = endTime;
}

SwitchScheduler::SwitchScheduler(SwitchSchedulerConfiguration* config)
{
    configuration = new SwitchSchedulerConfiguration();

    // time gets automagically sync'd on start up
    lastTimeSync = millis();

    tasks = new SwitchSchedulerTask*[10];

    Initialize(config);
}

void SwitchScheduler::Initialize(SwitchSchedulerConfiguration* config)
{
    SetTimezoneOffset(config->TimezoneOffset);
    SetAstronomyApiUrl(config->AstronomyApiUrl);
    SetAstronomyApiCheckTime(config->AstronomyApiCheckTime);
}

void SwitchScheduler::SetTimezoneOffset(int offset)
{
    bool isDst = rtc.isUSDST(Time.now());

    offset = (isDst ? offset + 1 : offset);
    Time.zone(offset);

    configuration->TimezoneOffset = offset;

    // Build in some delay to make sure that the timezone gets set properly.
    delay(500);
}


void SwitchScheduler::SetAstronomyApiUrl(String apiUrl)
{
    configuration->AstronomyApiUrl = apiUrl;
}

void SwitchScheduler::SetAstronomyApiCheckTime(String checkTime)
{
    configuration->AstronomyApiCheckTime = checkTime;
}

bool SwitchScheduler::IsUsingAstronomyData()
{
    return isUsingAstronomyData;
}

time_t SwitchScheduler::GetSunriseTime()
{
    return sunriseTime;
}

time_t SwitchScheduler::GetSunsetTime()
{
    return sunsetTime;
}

unsigned long SwitchScheduler::GetLastTimeSync()
{
    return lastTimeSync;
}

SwitchSchedulerConfiguration* SwitchScheduler::GetConfiguration()
{
    return configuration;
}

SwitchSchedulerTask** SwitchScheduler::GetTasks()
{
    return tasks;
}

int SwitchScheduler::GetTasksLength()
{
    return tasksLength;
}

bool SwitchScheduler::ShouldBeEnabled()
{
    time_t now = Time.now();

    for (int i = 0; i < tasksLength; i++)
    {
        time_t lightOn = GetTime(tasks[i]->StartTime);
        time_t lightOff = GetTime(tasks[i]->EndTime);

        if (now > lightOn && now < lightOff)
        {
            return true;
        }
    }

    return false;
}

void SwitchScheduler::AddSchedulerTask(SwitchSchedulerTask* task)
{
    // check to see if we need to retrieve astronomy data
    if (task->StartTime == "sunrise" ||
        task->StartTime == "sunset" ||
        task->EndTime == "sunrise" ||
        task->EndTime == "sunset")
    {
        isUsingAstronomyData = true;
    }

    tasks[tasksLength++] = task;
}

void SwitchScheduler::Tock()
{
    if (configuration == NULL)
    {
        DEBUG_PRINT("Configuration not set.");
        return;
    }

    unsigned long now = millis();

    if (now - lastLoopCheck > checkLoopInterval ||
        lastLoopCheck == 0)
    {
        // Sync the time daily.
        SyncTime();

        // Try to get the astronomy data if it's time.
        RetrieveAstronomyData();

        CheckSchedulerTasks();

        lastLoopCheck = now;
    }
}

void SwitchScheduler::CheckSchedulerTasks()
{
    DEBUG_PRINT("Checking scheduled tasks... ");
    DEBUG_PRINT(Time.timeStr() + "\n");

    for (int i = 0; i < tasksLength; i++)
    {
        CheckSchedulerTask(tasks[i], tasks[i]->StartTime, StartEvent);
        CheckSchedulerTask(tasks[i], tasks[i]->EndTime, EndEvent);
    }
}

void SwitchScheduler::CheckSchedulerTask(SwitchSchedulerTask* task, String timeString, SwitchSchedulerEvent event)
{
    int checkHour, checkMinute;
    time_t checkTime = GetTime(timeString);
    Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);

    // If the check time has arrived and the last check time wasn't this
    // minute (so we don't try to turn it on more than once).
    if (Time.hour() == checkHour && Time.minute() == checkMinute)
    {
        DEBUG_PRINT("Call callback!");
        // TODO: call callback handler
        // DEBUG_PRINT("Toggling outlet switch on... ");
        // DEBUG_PRINT(Time.timeStr() + "\n");
        //
        // toggleOutletSwitch(true);
        // lastToggleOutletSwitchOnTime = now;
    }
}

void SwitchScheduler::SyncTime()
{
    unsigned long now = millis();

    // just sync the time once a day
    if (now - lastTimeSync > dayInMilliseconds)
    {
        DEBUG_PRINT("Syncing time... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        Spark.syncTime();
        lastTimeSync = now;
    }
}

void SwitchScheduler::RetrieveAstronomyData()
{
    // Only retrieve sunset data if we actually need it.
    if (isUsingAstronomyData)
    {
        int checkHour, checkMinute;
        Sparky::ParseTime((configuration->AstronomyApiCheckTime).c_str(),
            &checkHour, &checkMinute);

        // If the check time has arrived and the last check time wasn't this
        // minute, or if we've never retrieved the sunset data yet.
        if ((Time.hour() == checkHour && Time.minute() == checkMinute) ||
            (sunriseTime == 0 || sunsetTime == 0))
        {
            DEBUG_PRINT("Retrieving sunset data... ");
            DEBUG_PRINT(Time.timeStr() + "\n");

            if (ParseAndSetAstronomyData())
            {
                DEBUG_PRINT("Sunset data retrieved! ");
                DEBUG_PRINT(Time.timeStr() + "\n");
            }
            else
            {
                DEBUG_PRINT("Failed to retrieve sunset data, retrying in a few seconds...");
                delay(5000);
                RetrieveAstronomyData();
            }
        }
    }
}

bool SwitchScheduler::ParseAndSetAstronomyData()
{
    const char* response = GetAstronomyDataResponse();

    // Parse response if we have one.
    if (strlen(response) != 0)
    {
        ArduinoJson::Parser::JsonParser<64> parser;
        ArduinoJson::Parser::JsonObject root = parser.parse(const_cast<char*>(response));

        if(root.success())
        {
            int hour, minute;

            hour = atoi(root["moon_phase"]["sunset"]["hour"]);
            minute = atoi(root["moon_phase"]["sunset"]["minute"]);
            sunsetTime = Sparky::ParseTimeFromToday(hour, minute, configuration->TimezoneOffset);

            DEBUG_PRINT("Sunset time: " + Time.timeStr(sunsetTime) + "\n");

            hour = atoi(root["moon_phase"]["sunrise"]["hour"]);
            minute = atoi(root["moon_phase"]["sunrise"]["minute"]);
            sunriseTime = Sparky::ParseTimeFromToday(hour, minute, configuration->TimezoneOffset);

            DEBUG_PRINT("Sunrise time: " + Time.timeStr(sunriseTime) + "\n");

            return true;
        }
    }

    return false;
}

const char* SwitchScheduler::GetAstronomyDataResponse()
{
    Uri apiUri = Uri::Parse(configuration->AstronomyApiUrl);

    RestClient client = RestClient(apiUri.Host.c_str());
    String response = "";

    int statusCode = client.get(apiUri.Path.c_str(), &response);

    if (statusCode == 200)
    {
        DEBUG_PRINT("Successfully retrieved JSON response\n");
        return response.c_str();
    }
    else
    {
        DEBUG_PRINT("Failed to retrieve sunset data with status: ");
        DEBUG_PRINT(statusCode);
        DEBUG_PRINT("\n");
        return "";
    }
}

time_t SwitchScheduler::GetTime(String timeString)
{
    if (timeString == "sunrise")
    {
        return sunriseTime;
    }
    else if (timeString == "sunset")
    {
        return sunsetTime;
    }

    return Sparky::ParseTimeFromString(
        timeString.c_str(),
        configuration->TimezoneOffset);
}
