#include "app_component.h"

char * CLAppComponent::getPrefsFileName(bool forsave) {
    if(tag) {
        snprintf(prefs, TAG_LENGTH, "/%s.json", tag);
        configured = Storage.exists(prefs);
        if(configured || forsave)
            return prefs;
        else {
            Serial.printf("Pref file %s not found, falling back to default\r\n", prefs);
            if(prefix)
              snprintf(prefs, TAG_LENGTH, "/%s_%s.json", prefix, tag);
            else
              snprintf(prefs, TAG_LENGTH, "/default_%s.json", tag);
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

int CLAppComponent::readJsonIntVal(jparse_ctx_t *jctx_ptr, const char* token) {
  int res=0;

  char * ptr = const_cast<char *>(token);

  if(json_obj_get_int(jctx_ptr, ptr, &res) == OS_SUCCESS)
    return res;

  return 0;
}

int CLAppComponent::removePrefs() {
  char *prefs_file = getPrefsFileName(true);  
  if (Storage.exists(prefs_file)) {
    Serial.printf("Removing %s\r\n", prefs_file);
    if (!Storage.remove(prefs_file)) {
      Serial.printf("Error removing %s preferences\r\n", tag);
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

int CLAppComponent::urlDecode(char * decoded, char * source, size_t len) {
  char temp[] = "0x00";
  int i=0;
  char * ptr = decoded;
  while (i < len){
    char decodedChar;
    char encodedChar = *(source+i);
    i++;
    if ((encodedChar == '%') && (i + 1 < len)){
      temp[2] = *(source+i); i++;
      temp[3] = *(source+i); i++;
      decodedChar = strtol(temp, NULL, 16);
    } else if (encodedChar == '+') {
      decodedChar = ' ';
    } else {
      decodedChar = encodedChar;  // normal ascii char
    }
    *ptr = decodedChar;
    ptr++;
    if(decodedChar == '\0') break;
  }
  return OS_SUCCESS;
}

int CLAppComponent::urlEncode(char * encoded, char * source, size_t len) {
  char c, code0, code1, code2;

  char * ptr = encoded;

  for(int i=0; i < len; i++) {
    c = *(source+i);
    if(c == '\0') {
      break;
    }
    else if(c == ' ') {
      *ptr = '+'; ptr++;
    }
    else if(isalnum(c)) {
      *ptr = c; ptr++;
    }
    else {

        code1 = (c & 0xf)+'0';
        if ((c & 0xf) > 9) {
            code1 = (c & 0xf) - 10 + 'A';
        }
        c = (c >> 4) & 0xf;
        code0 = c + '0';
        if (c > 9) {
            code0 = c - 10 + 'A';
        }

        *ptr = '%'; ptr++;
        *ptr = code0; ptr++;
        *ptr = code1; ptr++;

    }
  }
  *ptr = '\0';
  return OS_SUCCESS;
}