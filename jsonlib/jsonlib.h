#ifndef JSONLIB_H
#define JSONLIB_H

#include <Arduino.h>

// remove all white space from the json string... preserving strings
String jsonRemoveWhiteSpace(String json);

// index a json list
String jsonIndexList(String json, int idx);

// extract a json component from json
String jsonExtract(String json, String name);

#endif
