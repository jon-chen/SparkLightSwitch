#include <math.h>
#include "application.h"
#include "Sparky.h"

void Sparky::DoTheRainbow()
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
}

time_t Sparky::ParseTimeFromToday(int hour, int minute, int offset)
{
    time_t t = Time.now();
    struct tm* timeinfo;

    // Alter the timestamp with the given parameters.
    timeinfo = localtime(&t);
    timeinfo->tm_hour = hour - offset;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = 0;

    // Return unix timestamp of sunset.
    return mktime(timeinfo);
}

time_t Sparky::ParseTimeFromString(const char* data, int offset)
{
    int hour, minute;
    ParseTime(data, &hour, &minute);
    return ParseTimeFromToday(hour, minute, offset);
}

void Sparky::ParseTime(const char* data, int* hour, int* minute)
{
    sscanf(data, "%d:%d", hour, minute);
}

void Sparky::ParseTimestamp(time_t timestamp, int* hour, int* minute)
{
    *hour = Time.hour(timestamp);
    *minute = Time.minute(timestamp);
}
