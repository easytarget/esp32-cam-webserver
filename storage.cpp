#include "esp_camera.h"
#include "src/jsonlib/jsonlib.h"
#include "storage.h"

// These are defined in the main .ino file
extern void flashLED(int flashtime);
extern int myRotation;    // Rotation
extern int lampVal;       // The current Lamp value
extern bool autoLamp;     // Automatic lamp mode
extern int xclk;          // Camera module clock speed
extern int minFrameTime;  // Limits framerate

/*
 * Useful utility when debugging...
 */

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing SPIFFS directory: %s\r\n", dirname);

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
    int countSize = 0;
    while (file.available() && countSize <= PREFERENCES_MAX_SIZE) {
        Serial.print(char(file.read()));
        countSize++;
    }
    Serial.println("");
    file.close();
  } else {
    Serial.printf("%s not found, nothing to dump.\r\n", PREFERENCES_FILE);
  }
}

void loadPrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    // read file into a string
    String prefs;
    Serial.printf("Loading preferences from file %s\r\n", PREFERENCES_FILE);
    File file = fs.open(PREFERENCES_FILE, FILE_READ);
    if (!file) {
      Serial.println("Failed to open preferences file for reading, maybe corrupt, removing");
      removePrefs(SPIFFS);
      return;
    }
    size_t size = file.size();
    if (size > PREFERENCES_MAX_SIZE) {
      Serial.println("Preferences file size is too large, maybe corrupt, removing");
      removePrefs(SPIFFS);
      return;
    }
    while (file.available()) {
        prefs += char(file.read());
        if (prefs.length() > size) {
          // corrupted SPIFFS files can return data beyond their declared size.
          Serial.println("Preferences file failed to load properly, appears to be corrupt, removing");
          removePrefs(SPIFFS);
          return;
        }
    }
    // get sensor reference
    sensor_t * s = esp_camera_sensor_get();

    // process local settings
    if (lampVal >= 0) {
        int lampValPref = jsonExtract(prefs, "lamp").toInt();
        if (lampValPref >= 0) lampVal = lampValPref;
    }
    minFrameTime = jsonExtract(prefs, "min_frame_time").toInt();
    if (jsonExtract(prefs, "autolamp").toInt() == 0) autoLamp = false; else autoLamp = true;
    int xclkPref = jsonExtract(prefs, "xclk").toInt();
    if (xclkPref >= 2) xclk = xclkPref;
    myRotation = jsonExtract(prefs, "rotate").toInt();

    // process camera settings
    s->set_framesize(s, (framesize_t)jsonExtract(prefs, "framesize").toInt());
    s->set_quality(s, jsonExtract(prefs, "quality").toInt());
    s->set_xclk(s, LEDC_TIMER_0, xclk);
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
    // close the file
    file.close();
    dumpPrefs(SPIFFS);
  } else {
    Serial.printf("Preference file %s not found; using system defaults.\r\n", PREFERENCES_FILE);
  }
}

void savePrefs(fs::FS &fs){
  if (fs.exists(PREFERENCES_FILE)) {
    Serial.printf("Updating %s\r\n", PREFERENCES_FILE);
  } else {
    Serial.printf("Creating %s\r\n", PREFERENCES_FILE);
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
  p+=sprintf(p, "\"xclk\":%u,", xclk);
  p+=sprintf(p, "\"min_frame_time\":%d,", minFrameTime);
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
  Serial.println("Starting internal SPIFFS filesystem");
  while ( !SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED) ) {
    // if we sit in this loop something is wrong;
    // if no existing spiffs partition exists one should be automagically created.
    Serial.println("SPIFFS Mount failed, this can happen on first-run initialisation");
    Serial.println("If it happens repeatedly check if a SPIFFS partition is present for your board?");
    for (int i=0; i<10; i++) {
      flashLED(100); // Show SPIFFS failure
      delay(100);
    }
    delay(1000);
    Serial.println("Retrying..");
  }
  listDir(SPIFFS, "/", 0);
}
