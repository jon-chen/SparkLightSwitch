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

// TODO: can we chang this to time_t?
unsigned long SwitchScheduler::GetLastTimeSync()
{
    return lastTimeSync;
}

SwitchSchedulerConfiguration* SwitchScheduler::GetConfiguration()
{
    return configuration;
}

void SwitchScheduler::AddSchedulerTask(SwitchSchedulerTask* task)
{
    // check to see if we need to retrieve astronomy data
    if (task->StartTime == "sunrise" || task->StartTime == "sunset" ||
        task->EndTime == "sunrise" || task->EndTime == "sunset")
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

    // Sync the time daily.
    SyncTime();

    // Try to get the astronomy data if it's time.
    RetrieveAstronomyData();

    CheckSchedulerTasks();
}

void SwitchScheduler::CheckSchedulerTasks()
{
    unsigned long now = millis();

    if (now - lastCheckTask > checkTaskInterval)
    {
        DEBUG_PRINT("Checking scheduled tasks... ");
        DEBUG_PRINT(Time.timeStr() + "\n");

        for (int i = 0; i < tasksLength; i++)
        {
            CheckSchedulerTask(tasks[i], tasks[i]->StartTime, StartEvent);
            CheckSchedulerTask(tasks[i], tasks[i]->EndTime, EndEvent);
        }

        lastCheckTask = now;
    }
}

void SwitchScheduler::CheckSchedulerTask(SwitchSchedulerTask* task, String timeString, SwitchSchedulerEvent event)
{
    int checkHour, checkMinute;
    time_t now = Time.now();
    time_t checkTime = GetTime(timeString.c_str());
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
        time_t now = Time.now();
        int checkHour, checkMinute;
        Sparky::ParseTime((configuration->AstronomyApiCheckTime).c_str(),
            &checkHour, &checkMinute);

        // If the check time has arrived and the last check time wasn't this
        // minute, or if we've never retrieved the sunset data yet.
        if ((Time.hour() == checkHour && Time.minute() == checkMinute &&
            now / (1000 * 60) != lastAstronomyApiCheckTime / (1000 * 60)) ||
            lastAstronomyApiCheckTime == 0)
        {
            DEBUG_PRINT("Retrieving sunset data... ");
            DEBUG_PRINT(Time.timeStr() + "\n");

            if (ParseAndSetAstronomyData())
            {
                DEBUG_PRINT("Sunset data retrieved! ");
                DEBUG_PRINT(Time.timeStr() + "\n");

                lastAstronomyApiCheckTime = now;
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

    DEBUG_PRINT(apiUri.Protocol + "\n");
    DEBUG_PRINT(apiUri.Host + "\n");
    DEBUG_PRINT(apiUri.Port + "\n");
    DEBUG_PRINT(apiUri.Path + "\n");
    DEBUG_PRINT(apiUri.QueryString + "\n");

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

time_t SwitchScheduler::GetTime(const char* timeString)
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
        timeString,
        configuration->TimezoneOffset);
}
