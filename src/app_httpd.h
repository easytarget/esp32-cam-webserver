#ifndef app_httpd_h
#define app_httpd_h

#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <freertos/timers.h>

#include "esp32pwm.h"
#include "ESPAsyncWebServer.h"
#include "storage.h"
#include "app_conn.h"
#include "app_cam.h"

#define MAX_URI_MAPPINGS                32

#define PWM_DEFAULT_FREQ                50
#define PWM_DEFAULT_RESOLUTION_BITS     10

#define DEFAULT_uS_LOW                  544
#define DEFAULT_uS_HIGH                 2400

#define DEFAULT_FLASH                   0xFF

#define RESET_ALL_PWM                   0

#define SERIAL_BUFFER_SIZE              64

#define MAX_VIDEO_STREAMS               5


enum CaptureModeEnum {CAPTURE_STILL, CAPTURE_STREAM};
enum StreamResponseEnum {STREAM_SUCCESS, 
                         STREAM_NUM_EXCEEDED, 
                         STREAM_CLIENT_REGISTER_FAILED,
                         STREAM_TIMER_NOT_INITIALIZED,
                         STREAM_MODE_NOT_SUPPORTED, 
                         STREAM_IMAGE_CAPTURE_FAILED,
                         STREAM_CLIENT_NOT_FOUND};

String processor(const String& var);
void onSystemStatus(AsyncWebServerRequest *request);
void onStatus(AsyncWebServerRequest *request);
void onInfo(AsyncWebServerRequest *request);
void onControl(AsyncWebServerRequest *request);
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void onSnapTimer(TimerHandle_t pxTimer);



/**
 * @brief Static URI to path mapping
 * 
 */
struct UriMapping { char uri[32]; char path[32];};


/** 
 * @brief WebServer Manager
 * Class for handling web server requests. The web pages are assumed to be stored in the file system (can be SD card or LittleFS).  
 * 
 */
class CLAppHttpd : public CLAppComponent {
    public:
        CLAppHttpd();

        int start();
        int loadPrefs();
        int savePrefs();

        void cleanupWsClients();

        // register a client streaming video
        int addStreamClient(uint32_t client_id);
        int removeStreamClient(uint32_t client_id);

        uint32_t getControlClient() {return control_client;};
        void setControlClient(uint32_t id) {control_client = id;};

        int8_t getStreamCount() {return streamCount;};
        long getStreamsServed() {return streamsServed;};
        unsigned long getImagesServed() {return imagesServed;};
        int getPwmCount() {return pwmCount;};
        void incImagesServed(){imagesServed++;};
        
        // capture image and send it to the clients
        int snapToStream(bool debug = false);
        // start stream
        StreamResponseEnum startStream(uint32_t id, CaptureModeEnum stream_mode);
        //terminate stream
        StreamResponseEnum stopStream(uint32_t id);

        void updateSnapTimer(int frameRate);

        void serialSendCommand(const char * cmd);

        int getSketchSize(){ return sketchSize;};
        int getSketchSpace() {return sketchSpace;};
        
        const char * getSketchMD5() {return sketchMD5.c_str();};

        const char * getVersion() {return version.c_str();};

        char * getName() {return myName;};

        char * getSerialBuffer() {return serialBuffer;};

        void setAutoLamp(bool val) {autoLamp = val;};
        bool isAutoLamp() { return autoLamp;};   
        int getFlashLamp() {return flashLamp;}; 
        void setFlashLamp(int newVal) {flashLamp = newVal;};

        void setLamp(int newVal = DEFAULT_FLASH);
        int getLamp() {return lampVal;};    

        void dumpSystemStatusToJson(char * buf, size_t size);
        void dumpCameraStatusToJson(char * buf, size_t size, bool full = true);

        /**
         * @brief attaches a new PWM/servo and returns its ID in case of success, or OS_FAIL otherwise
         * 
         * @param pin 
         * @param freq
         * @param resolution_bits
         * @return int 
         */
        int attachPWM(uint8_t pin, double freq = PWM_DEFAULT_FREQ, uint8_t resolution_bits = PWM_DEFAULT_RESOLUTION_BITS);
        
        /**
         * @brief writes an angle value to PWM/Servo.
         * 
         * @param pin 
         * @param value 
         * @param min_v
         * @param max_v
         * @return int 
         */
        int writePWM(uint8_t pin, int value, int min_v = DEFAULT_uS_LOW, int max_v = DEFAULT_uS_HIGH);

        /**
         * @brief Set all PWM to its default value. If the default was not defined, it will be reset to 0
         * 
         * @param pin 
         */
        void resetPWM(uint8_t pin = RESET_ALL_PWM);

        uint8_t getTemp() {return temperatureRead();};
        
    private:

        UriMapping *mappingList[MAX_URI_MAPPINGS]; 
        int mappingCount=0;

        ESP32PWM *pwm[NUM_PWM];

        int pwmCount = 0;

        // Name of the application used in web interface
        // Can be re-defined in the httpd.json file
        char myName[32] = CAM_NAME;

        char serialBuffer[SERIAL_BUFFER_SIZE]="";

        AsyncWebServer *server;
        AsyncWebSocket *ws; 
        
        // array of clients currently streaming video 
        uint32_t stream_clients[MAX_VIDEO_STREAMS];

        uint32_t control_client;
        
        TimerHandle_t snap_timer = NULL;
        
        // Flash LED lamp parameters.
        // should be defined in the 1st line of the pwm collection in the httpd prefs (httpd.json)
        bool autoLamp = false;         // Automatic lamp (auto on while camera running)
        int lampVal = -1;              // Lamp brightness
        int flashLamp = 80;            // Flash brightness when taking still images or capturing streams
        uint8_t lamppin = 0;           // Lamp pin, not defined by default
        int pwmMax = 1;                // pwmMax = pow(2,pwmresolution)-1;

        int8_t streamCount=0;

        long streamsServed=0;
        long imagesServed=0;

        // maximum number of parallel video streams supported. This number can range from 1 to MAX_VIDEO_STREAMS
        int max_streams=2;
        
        // Sketch Info
        int sketchSize ;
        int sketchSpace ;
        String sketchMD5;

        const String version = __DATE__ " @ " __TIME__;

};


extern CLAppHttpd AppHttpd;

#endif