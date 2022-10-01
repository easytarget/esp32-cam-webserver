#ifndef app_cam_h
#define app_cam_h

#include <esp_camera.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include "app_component.h"
#include "camera_pins.h"

#define LAMP_DEFAULT 80         // initial lamp value, range 0-100

#if defined(LED_DISABLE)
    #undef LED_PIN              // undefining this disables the notification LED
#endif

class CLAppCam : public CLAppComponent {
    public:

        CLAppCam();

        int start();
        int stop(); 
        int loadPrefs();
        int savePrefs();

        void setLamp(int newVal = LAMP_DEFAULT);
        int getLamp() {return lampVal;};

        void setAutoLamp(bool val) {autoLamp = val;};
        bool isAutoLamp() { return autoLamp;};

        int getSensorPID() {return sensorPID;};
        String getErr() {return critERR;};

        int getFrameRate() {return frameRate;};
        void setFrameRate(int newFrameRate) {frameRate = newFrameRate;};

        void setXclk(int val) {xclk = val;};
        int getXclk() {return xclk;};

        void setRotation(int val) {myRotation = val;};
        int getRotation() {return myRotation;};

    private:
        // Camera config structure
        camera_config_t config;

        // This will be set to the sensors PID (identifier) during initialisation
        //camera_pid_t sensorPID;
        int sensorPID;

        // Camera module bus communications frequency.
        // Originally: config.xclk_freq_mhz = 20000000, but this lead to visual artifacts on many modules.
        // See https://github.com/espressif/esp32-camera/issues/150#issuecomment-726473652 et al.
        // Initial setting is configured in /default_prefs.json
        int xclk = 8;

        // frame rate in FPS
        // default can be set in /default_prefs.json
        int frameRate = 25;

        // Flash LED lamp parameters.
        bool autoLamp = false;         // Automatic lamp (auto on while camera running)
        
        // Illumination LAMP and status LED
        #if defined(LAMP_DISABLE)
            int lampVal = -1; // lamp is disabled in config
        #elif defined(LAMP_PIN)
            int lampVal = 0; 
        #else
            int lampVal = -1; // no lamp pin assigned
        #endif

        int lampChannel = 7;           // a free PWM channel (some channels used by camera)
        const int pwmfreq = 50000;     // 50K pwm frequency
        const int pwmresolution = 9;   // duty cycle bit range
        const int pwmMax = pow(2,pwmresolution)-1;

        // Critical error string; if set during init (camera hardware failure) it
        // will be returned for stream and still image requests
        String critERR = "";

        // initial rotation
        // default can be set in /default_prefs.json
        int myRotation = 0;


};

extern CLAppCam AppCam;

#endif