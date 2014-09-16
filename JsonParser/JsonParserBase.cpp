/*
* Arduino JSON library
* Benoit Blanchon 2014 - MIT License
*/

#include "JsonParserBase.h"
#include "JsonToken.h"

using namespace ArduinoJson::Parser;

JsonValue JsonParserBase::parse(char* json)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    jsmnerr_t status = jsmn_parse(&parser, json, tokens, maxTokens);

    #ifdef SPARK_DEBUG
    Serial.print("JSON parse status: ");
    Serial.println(status);
    #endif

    if (JSMN_SUCCESS != status)
        return JsonToken::null();

    return JsonToken(json, tokens);
}
