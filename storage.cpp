#include "esp_camera.h"
#include "src/jsonlib/jsonlib.h"
#include "storage.h"

// These are defined in the main .ino file
extern void flashLED(int flashtime);
extern int myRotation;              // Rotation
extern int lampVal;                 // The current Lamp value
extern int autoLamp;                // Automatic lamp mode
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

void loadPrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    // read file into a string
    String prefs;
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
    while (file.available()) prefs += char(file.read());
    // get sensor reference
    sensor_t * s = esp_camera_sensor_get();
    // process all the settings
    lampVal = jsonExtract(prefs, "lamp").toInt();
    autoLamp = jsonExtract(prefs, "autolamp").toInt();
    s->set_framesize(s, (framesize_t)jsonExtract(prefs, "framesize").toInt());
    s->set_quality(s, jsonExtract(prefs, "quality").toInt());
    s->set_brightness(s, jsonExtract(prefs, "brightness").toInt());
    s->set_contrast(s, jsonExtract(prefs, "contrast").toInt());
    s->set_saturation(s, jsonExtract(prefs, "saturation").toInt());
    s->set_special_effect(s, jsonExtract(prefs, "special_effect").toInt());
    s->set_wb_mode(s, jsonExtract(prefs, "wb_mode").toInt());
    s->set_whitebal(s, jsonExtract(prefs, "awb").toInt());
    s->set_awb_gain(s, jsonExtract(prefs, "awb_gain").toInt());
    s->set_exposure_ctrl(s, jsonExtract(prefs, "aec").toInt());
    s->set_aec2(s, jsonExtract(prefs, "aec2").toInt());
    s->set_ae_level(s, jsonExtract(prefs, "ae_level").toInt());
    s->set_aec_value(s, jsonExtract(prefs, "aec_value").toInt());
    s->set_gain_ctrl(s, jsonExtract(prefs, "agc").toInt());
    s->set_agc_gain(s, jsonExtract(prefs, "agc_gain").toInt());
    s->set_gainceiling(s, (gainceiling_t)jsonExtract(prefs, "gainceiling").toInt());
    s->set_bpc(s, jsonExtract(prefs, "bpc").toInt());
    s->set_wpc(s, jsonExtract(prefs, "wpc").toInt());
    s->set_raw_gma(s, jsonExtract(prefs, "raw_gma").toInt());
    s->set_lenc(s, jsonExtract(prefs, "lenc").toInt());
    s->set_vflip(s, jsonExtract(prefs, "vflip").toInt());
    s->set_hmirror(s, jsonExtract(prefs, "hmirror").toInt());
    s->set_dcw(s, jsonExtract(prefs, "dcw").toInt());
    s->set_colorbar(s, jsonExtract(prefs, "colorbar").toInt());
    detection_enabled = jsonExtract(prefs, "face_detect").toInt();
    recognition_enabled = jsonExtract(prefs, "face_recognize").toInt();
    myRotation = jsonExtract(prefs, "rotate").toInt();
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
  p+=sprintf(p, "\"autolamp\":%u,", autoLamp);
  p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p+=sprintf(p, "\"quality\":%u,", s->status.quality);
  p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
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

void saveFaceDB(fs::FS &fs) {
  // Stub!
  return;
}
void loadFaceDB(fs::FS &fs) {
  // Stub!
  return;
}
void removeFaceDB(fs::FS &fs) {
  // Stub!
  return;
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
