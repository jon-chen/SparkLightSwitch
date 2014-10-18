#ifndef SPARK_DEBUG_H_
#define SPARK_DEBUG_H_

#ifdef SPARK_DEBUG
#define DEBUG_PRINT(string) (Serial.print(string))
#endif

#ifndef SPARK_DEBUG
#define DEBUG_PRINT(string)
#endif

#endif // SPARK_DEBUG_H_
