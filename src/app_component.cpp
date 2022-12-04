#include "app_component.h"

char * CLAppComponent::getPrefsFileName(bool forsave) {
    if(tag) {
        sprintf(prefs, "/%s.json", tag);
        if(Storage.exists(prefs) || forsave)
            return prefs;
        else {
            Serial.printf("Pref file %s not found, falling back to default\r\n", prefs);
            sprintf(prefs, "/default_%s.json", tag);
            return prefs;
        }
    }
    else
        return prefs;
}

void CLAppComponent::dumpPrefs() {
    char *prefs_file = getPrefsFileName(); 
    String s;
    if(Storage.readFileToString(prefs_file, &s) != OK) {
        Serial.printf("Preference file %s not found.\r\n", prefs_file);
        return;
    }
    Serial.println(s);
}

int CLAppComponent::readJsonIntVal(jparse_ctx_t *jctx_ptr, char* token) {
  int res=0;
  if(json_obj_get_int(jctx_ptr, token, &res) == OS_SUCCESS)
    return res;

  return 0;
}

int CLAppComponent::removePrefs() {
  char *prefs_file = getPrefsFileName(true);  
  if (Storage.exists(prefs_file)) {
    Serial.printf("Removing %s\r\n", prefs_file);
    if (!Storage.remove(prefs_file)) {
      sprintf("Error removing %s preferences\r\n", tag);
      return OS_FAIL;
    }
  } else {
    Serial.printf("No saved %s preferences to remove\r\n", tag);
  }
  return OS_SUCCESS;
}

int CLAppComponent::parsePrefs(jparse_ctx_t *jctx) {
  char *conn_file = getPrefsFileName(); 

  String conn_json;

  if(Storage.readFileToString(conn_file, &conn_json) != OK) {
      Serial.printf("Failed to open the connection settings from %s \r\n", conn_file);
      return OS_FAIL;
  }

  char *cn_ptr = const_cast<char*>(conn_json.c_str());

  int ret = json_parse_start(jctx, cn_ptr, conn_json.length());
  if(ret != OS_SUCCESS) {
      Serial.printf("Preference file %s could not be parsed; using system defaults.\r\n", conn_file);
      return OS_FAIL;
  }

  return ret;
}