/*
* Arduino JSON library
* Benoit Blanchon 2014 - MIT License
*/

#include "SparkDebug.h"
#include "JsonParserBase.h"
#include "JsonToken.h"

using namespace ArduinoJson::Parser;

JsonValue JsonParserBase::parse(char* json)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    jsmnerr_t status = jsmn_parse(&parser, json, tokens, maxTokens);

    DEBUG_PRINT("JSON parse status: ");
    DEBUG_PRINT(status);
    DEBUG_PRINT("\n");

    if (JSMN_SUCCESS != status)
        return JsonToken::null();

    return JsonToken(json, tokens);
}
