#include "storage.h"

// These are defined in the main .ino file
extern void flashLED(int flashtime);

extern char *myRotation;           // Rotation
extern int lampVal;                 // The current Lamp value
extern int8_t detection_enabled;    // Face detection enable
extern int8_t recognition_enabled;  // Face recognition enable


/*
 * Useful utility when debugging... 
 */

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing SPIFFS directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

//  DEBUG
void dumpPrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    // Dump contents for debug
    File file = fs.open(PREFERENCES_FILE, FILE_READ);
    while (file.available()) Serial.print(char(file.read()));
    Serial.println("");
    file.close();
  } else {
    Serial.printf("%s not found, nothing to dump.\n", PREFERENCES_FILE);
  }
}
// /DEBUG

void loadPrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    Serial.printf("Loading preferences from file %s\n", PREFERENCES_FILE);
    File file = fs.open(PREFERENCES_FILE, FILE_READ);
    if (!file) {
      Serial.println("Failed to open preferences file");
      return;
    }
    size_t size = file.size();
    if (size > 800) {
      Serial.println("Preferences file size is too large, maybe corrupt");
      return;
    }
    // Allocate the memory for deserialisation
    StaticJsonDocument<1023> doc;
    // Parse the prefs file
    DeserializationError err=deserializeJson(doc, file);
    if(err) {
      Serial.print(F("deserializeJson() failed with code: "));
      Serial.println(err.c_str());
      return;
    }
    // Sensor reference
    sensor_t * s = esp_camera_sensor_get();
    // process all the settings we save
    lampVal = doc["lamp"];
    s->set_framesize(s, (framesize_t)doc["framesize"]);
    s->set_quality(s, doc["quality"]);
    s->set_contrast(s, doc["contrast"]);
    s->set_brightness(s, doc["brightness"]);
    s->set_saturation(s, doc["saturation"]);
    s->set_special_effect(s, doc["special_effect"]);
    s->set_hmirror(s, doc["hmirror"]);
    s->set_vflip(s, doc["vflip"]);
    // close the file
    file.close();
    dumpPrefs(SPIFFS);
  } else {
    Serial.printf("Preference file %s not found; using system defaults.\n", PREFERENCES_FILE);
  }
}

void savePrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    Serial.printf("Updating %s\n", PREFERENCES_FILE);
  } else {
    Serial.printf("Creating %s\n", PREFERENCES_FILE);
  }
  File file = fs.open(PREFERENCES_FILE, FILE_WRITE);
  static char json_response[1024];
  sensor_t * s = esp_camera_sensor_get();
  char * p = json_response;
  *p++ = '{';
  p+=sprintf(p, "\"lamp\":%i,", lampVal);
  p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p+=sprintf(p, "\"quality\":%u,", s->status.quality);
  p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
  p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
  p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
  p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
  p+=sprintf(p, "\"awb\":%u,", s->status.awb);
  p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
  p+=sprintf(p, "\"aec\":%u,", s->status.aec);
  p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
  p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
  p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
  p+=sprintf(p, "\"agc\":%u,", s->status.agc);
  p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
  p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
  p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
  p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
  p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
  p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
  p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
  p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
  p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
  p+=sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
  p+=sprintf(p, "\"face_detect\":%u,", detection_enabled);
  p+=sprintf(p, "\"face_recognize\":%u,", recognition_enabled);
  p+=sprintf(p, "\"rotate\":\"%d\"", myRotation);
  *p++ = '}';
  *p++ = 0;
  file.print(json_response);
  file.close();
  dumpPrefs(SPIFFS);
}

void removePrefs(fs::FS &fs) {
  if (fs.exists(PREFERENCES_FILE)) {
    Serial.printf("Removing %s\r\n", PREFERENCES_FILE);
    if (!fs.remove(PREFERENCES_FILE)) {
      Serial.println("Error removing preferences");
    }
  } else {
    Serial.println("No saved preferences file to remove");
  }
}

void filesystemStart(){
    while ( !SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED) ) {
    // if we sit in this loop something is wrong; 
    // if no existing spiffs partition exists one should be automagically created.
    Serial.println("SPIFFS Mount failed, this can happen on first-run initialisation.");
    Serial.println("If it happens repeatedly check if a SPIFFS partition is present for your board?");
    for (int i=0; i<10; i++) {
      flashLED(100); // Show SPIFFS failure
      delay(100);
    }
    delay(1000);
    Serial.println("Retrying..");
  }
  Serial.println("Internal filesystem contents");
  listDir(SPIFFS, "/", 0);
}
