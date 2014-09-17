#ifndef LightTimerIncludes_h
#define LightTimerIncludes_h

#define SPARK_DEBUG

#include "application.h"
#include "JsonParser.h"
#include "rest_client.h"
#include "SparkTime.h"

#ifdef SPARK_DEBUG
#define DEBUG_PRINT(string) (Serial.print(string))
#endif

#ifndef SPARK_DEBUG
#define DEBUG_PRINT(string)
#endif

#endif /* LightTimerIncludes_h */
