#include "application.h"

class LightTimer
{
  public:
    LightTimer();
    // sync the time daily
    void syncTime();

    unsigned long retrieveSunsetData();
  private:
    unsigned long lastTimeSync;
    const String weatherApiBaseUrl = "spark-light-timer.googlecode.com";
    const String sunsetApiUrl = "/git/sample-astronomy.json";
    // const String weatherApiBaseUrl = "api.wunderground.com";
    // const String sunsetApiUrl = "/api/91329161b08b1483/astronomy/q/30329.json";
    const unsigned long dayInMilliseconds = (24 * 60 * 60 * 1000);

    const char* getSunsetDataResponse();
};
