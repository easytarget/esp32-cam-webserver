#ifndef app_cam_h
#define app_cam_h

#define CAM_DUMP_BUFFER_SIZE   640

#include <esp_camera.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include "app_component.h"
#include "camera_pins.h"


/**
 * @brief Camera Manager
 * Manages all interactions with camera
 */
class CLAppCam : public CLAppComponent {
    public:

        CLAppCam();

        int start();
        int stop(); 
        int loadPrefs();
        int savePrefs();

        int getSensorPID() {return (sensor?sensor->id.PID:0);};
        sensor_t * getSensor() {return sensor;};
        String getErr() {return critERR;};

        int getFrameRate() {return frameRate;};
        void setFrameRate(int newFrameRate) {frameRate = newFrameRate;};

        void setXclk(int val) {xclk = val;};
        int getXclk() {return xclk;};

        void setRotation(int val) {myRotation = val;};
        int getRotation() {return myRotation;};

        int snapToBuffer();
        uint8_t * getBuffer() {return (fb?fb->buf:nullptr);};
        size_t getBufferSize() {return (fb?fb->len:0);};
        bool isJPEGinBuffer() {return (fb?fb->format == PIXFORMAT_JPEG:false);};
        void releaseBuffer(); 

        void dumpStatusToJson(json_gen_str_t * jstr, bool full_status = true);

    private:
        // Camera config structure
        camera_config_t config;

        // Camera module bus communications frequency.
        // Originally: config.xclk_freq_mhz = 20000000, but this lead to visual artifacts on many modules.
        // See https://github.com/espressif/esp32-camera/issues/150#issuecomment-726473652 et al.
        // Initial setting is configured in /default_prefs.json
        int xclk = 8;

        // frame rate in FPS
        // default can be set in /default_prefs.json
        int frameRate = 25;


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

        // camera buffer pointer
        camera_fb_t * fb = NULL;

        // camera sensor
        sensor_t * sensor;


};

extern CLAppCam AppCam;

#endif