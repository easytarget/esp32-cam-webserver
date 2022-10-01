#ifndef app_httpd_h
#define app_httpd_h

#include <esp_camera.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <freertos/timers.h>

#include "ESPAsyncWebServer.h"
#include "storage.h"
#include "app_conn.h"
#include "app_cam.h"

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

enum capture_mode {CAPTURE_STILL, CAPTURE_STREAM};

String processor(const String& var);
void onSystemStatus(AsyncWebServerRequest *request);
void onStatus(AsyncWebServerRequest *request);
void onInfo(AsyncWebServerRequest *request);
void onControl(AsyncWebServerRequest *request);
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void onSnapTimer(TimerHandle_t pxTimer);

void dumpSystemStatusToJson(char * buf, size_t size);

class CLAppHttpd : public CLAppComponent {
    public:
        CLAppHttpd();

        int start();
        int loadPrefs();

        uint32_t getClientId() {return client_id;};

        int8_t getStreamCount() {return streamCount;};
        long getStreamsServed() {return streamsServed;};
        unsigned long getImagesServed() {return imagesServed;};
        void incImagesServed(){imagesServed++;};

        void setStreamMode(capture_mode mode) {streammode = mode;};
        capture_mode getStreamMode() {return streammode;};

        esp_err_t snapToStream(bool debug = false);
        esp_err_t startStream(uint32_t id);
        esp_err_t stopStream(uint32_t id);

        void updateSnapTimer(int frameRate);

        void serialSendCommand(const char * cmd);

        int getSketchSize(){ return sketchSize;};
        int getSketchSpace() {return sketchSpace;};
        String getSketchMD5() {return sketchMD5;};

        String getVersion() {return version;};

        char * getName() {return myName;};

        char * getSerialBuffer() {return serialBuffer;};


    private:

        // Name of the application used in web interface
        // Can be re-defined in the httpd.json file
        char myName[20] = CAM_NAME;

        char serialBuffer[64];

        AsyncWebServer *server;
        AsyncWebSocket *ws; 
        uint32_t client_id = 0;
        TimerHandle_t snap_timer = NULL;

        int8_t streamCount=0;
        long streamsServed=0;
        unsigned long imagesServed;

        // mode of the image capture
        capture_mode streammode = CAPTURE_STILL;
        
        // Sketch Info
        int sketchSize ;
        int sketchSpace ;
        String sketchMD5;

        const String version = __DATE__ " @ " __TIME__;

};



extern CLAppHttpd AppHttpd;

#endif