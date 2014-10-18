#include "SparkDebug.h"
#include "Sparky.h"
#include "JsonParser.h"
#include "Uri.h"
#include "rest_client.h"
#include "SwitchScheduler.h"

SwitchSchedulerTask::SwitchSchedulerTask(String startTime, String endTime, void (*pCallback)(int))
{
    startTime = startTime;
    endTime = endTime;
    callback = pCallback;
}

SwitchScheduler::SwitchScheduler(SwitchSchedulerConfiguration* config)
{
    configuration = new SwitchSchedulerConfiguration();

    // time gets automagically sync'd on start up
    lastTimeSync = millis();

    tasks = new SwitchSchedulerTask*[10];

    initialize(config);
}

void SwitchScheduler::initialize(SwitchSchedulerConfiguration* config)
{
    setTimezoneOffset(config->timezoneOffset);
    setAstronomyApiUrl(config->astronomyApiUrl);
    setAstronomyApiCheckTime(config->astronomyApiCheckTime);
}

void SwitchScheduler::setTimezoneOffset(int offset)
{
    bool isDst = rtc.isUSDST(Time.now());

    offset = (isDst ? offset + 1 : offset);
    Time.zone(offset);

    configuration->timezoneOffset = offset;

    // Build in some delay to make sure that the timezone gets set properly.
    delay(500);
}


void SwitchScheduler::setAstronomyApiUrl(String apiUrl)
{
    configuration->astronomyApiUrl = apiUrl;
}

void SwitchScheduler::setAstronomyApiCheckTime(String checkTime)
{
    configuration->astronomyApiCheckTime = checkTime;
}

bool SwitchScheduler::isUsingAstronomyData()
{
    return _isUsingAstronomyData;
}

time_t SwitchScheduler::getSunriseTime()
{
    return sunriseTime;
}

time_t SwitchScheduler::getSunsetTime()
{
    return sunsetTime;
}

unsigned long SwitchScheduler::getLastTimeSync()
{
    return lastTimeSync;
}

SwitchSchedulerConfiguration* SwitchScheduler::getConfiguration()
{
    return configuration;
}

SwitchSchedulerTask** SwitchScheduler::getTasks()
{
    return tasks;
}

int SwitchScheduler::getTasksLength()
{
    return tasksLength;
}

bool SwitchScheduler::shouldBeEnabled()
{
    time_t now = Time.now();

    for (int i = 0; i < tasksLength; i++)
    {
        time_t lightOn = getTime(tasks[i]->startTime);
        time_t lightOff = getTime(tasks[i]->endTime);

        if (now > lightOn && now < lightOff)
        {
            return true;
        }
    }

    return false;
}

void SwitchScheduler::addSchedulerTask(SwitchSchedulerTask* task)
{
    // check to see if we need to retrieve astronomy data
    if (task->startTime == "sunrise" ||
        task->startTime == "sunset" ||
        task->endTime == "sunrise" ||
        task->endTime == "sunset")
    {
        _isUsingAstronomyData = true;
    }

    tasks[tasksLength++] = task;
}

void SwitchScheduler::tock()
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
        syncTime();

        // Try to get the astronomy data if it's time.
        retrieveAstronomyData();

        checkSchedulerTasks();

        lastLoopCheck = now;
    }
}

void SwitchScheduler::checkSchedulerTasks()
{
    DEBUG_PRINT("Checking scheduled tasks... ");
    DEBUG_PRINT(Time.timeStr() + "\n");

    for (int i = 0; i < tasksLength; i++)
    {
        checkSchedulerTask(tasks[i], tasks[i]->startTime, StartEvent);
        checkSchedulerTask(tasks[i], tasks[i]->endTime, EndEvent);
    }
}

void SwitchScheduler::checkSchedulerTask(SwitchSchedulerTask* task, String timeString, SwitchSchedulerEvent event)
{
    int checkHour, checkMinute;
    time_t checkTime = getTime(timeString);
    Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);

    if (Time.hour() == checkHour && Time.minute() == checkMinute)
    {
        task->callback(static_cast<int>(event));
    }
}

void SwitchScheduler::syncTime()
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

void SwitchScheduler::retrieveAstronomyData()
{
    // Only retrieve sunset data if we actually need it.
    if (_isUsingAstronomyData)
    {
        int checkHour, checkMinute;
        Sparky::ParseTime((configuration->astronomyApiCheckTime).c_str(),
            &checkHour, &checkMinute);

        // If the check time has arrived and the last check time wasn't this
        // minute, or if we've never retrieved the sunset data yet.
        if ((Time.hour() == checkHour && Time.minute() == checkMinute) ||
            (sunriseTime == 0 || sunsetTime == 0))
        {
            DEBUG_PRINT("Retrieving sunset data... ");
            DEBUG_PRINT(Time.timeStr() + "\n");

            if (parseAndSetAstronomyData())
            {
                DEBUG_PRINT("Sunset data retrieved! ");
                DEBUG_PRINT(Time.timeStr() + "\n");
            }
            else
            {
                DEBUG_PRINT("Failed to retrieve sunset data, retrying in a few seconds...");
                delay(5000);
                retrieveAstronomyData();
            }
        }
    }
}

bool SwitchScheduler::parseAndSetAstronomyData()
{
    const char* response = getAstronomyDataResponse();

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
            sunsetTime = Sparky::ParseTimeFromToday(hour, minute, configuration->timezoneOffset);

            DEBUG_PRINT("Sunset time: " + Time.timeStr(sunsetTime) + "\n");

            hour = atoi(root["moon_phase"]["sunrise"]["hour"]);
            minute = atoi(root["moon_phase"]["sunrise"]["minute"]);
            sunriseTime = Sparky::ParseTimeFromToday(hour, minute, configuration->timezoneOffset);

            DEBUG_PRINT("Sunrise time: " + Time.timeStr(sunriseTime) + "\n");

            return true;
        }
    }

    return false;
}

const char* SwitchScheduler::getAstronomyDataResponse()
{
    Uri apiUri = Uri::Parse(configuration->astronomyApiUrl);

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

time_t SwitchScheduler::getTime(String timeString)
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
        configuration->timezoneOffset);
}
