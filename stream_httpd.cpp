// ESP32 has two cores: APPlication core and PROcess core (the one that runs ESP32 SDK stack)
#define APP_CPU 1
#define PRO_CPU 0

#include "Arduino.h"
#include "src/OV2640.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <esp_bt.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "myconfig.h"
#include "camera_pins.h"

extern IPAddress espIP;
extern bool updating;

/*
  Next one is an include with wifi credentials.
  This is what you need to do:

  1. Create a file called "home_wifi_multi.h" in the same folder   OR   under a separate subfolder of the "libraries" folder of Arduino IDE. (You are creating a "fake" library really - I called it "MySettings").
  2. Place the following text in the file:
  #define SSID1 "replace with your wifi ssid"
  #define PWD1 "replace your wifi password"
  3. Save.

  Should work then
*/

#if !defined(STREAM_PORT)
#define STREAM_PORT 81
#endif
int streamPort = STREAM_PORT;
WebServer server(streamPort);
char streamURL[64] = {"Undefined"};

// ===== rtos task handles =========================
// Streaming is implemented with 3 tasks:
TaskHandle_t tMjpeg;  // handles client connections to the webserver
TaskHandle_t tCam;    // handles getting picture frames from the camera and storing them locally
TaskHandle_t tStream; // actually streaming frames to all connected clients

// frameSync semaphore is used to prevent streaming buffer as it is replaced with the next frame
SemaphoreHandle_t frameSync = NULL;

// Queue stores currently connected clients to whom we are streaming
QueueHandle_t streamingClients;
UBaseType_t activeClients;

// We will try to achieve 25 FPS frame rate
const int FPS = 10;

// We will handle web client requests every 50 ms (20 Hz)
const int WSINTERVAL = 100;

// def
void mjpegCB(void *pvParameters);
void camCB(void *pvParameters);
char *allocateMemory(char *aPtr, size_t aSize);
void handleJPGSstream(void);
void streamCB(void *pvParameters);
void handleJPG(void);
void handleNotFound();
void startStreamServer();

// ==== Memory allocator that takes advantage of PSRAM if present =======================
char *allocateMemory(char *aPtr, size_t aSize)
{

    //  Since current buffer is too smal, free it
    if (aPtr != NULL)
        free(aPtr);

    size_t freeHeap = ESP.getFreeHeap();
    char *ptr = NULL;

    // If memory requested is more than 2/3 of the currently free heap, try PSRAM immediately
    if (aSize > freeHeap * 2 / 3)
    {
        if (psramFound() && ESP.getFreePsram() > aSize)
        {
            ptr = (char *)ps_malloc(aSize);
        }
    }
    else
    {
        //  Enough free heap - let's try allocating fast RAM as a buffer
        ptr = (char *)malloc(aSize);

        //  If allocation on the heap failed, let's give PSRAM one more chance:
        if (ptr == NULL && psramFound() && ESP.getFreePsram() > aSize)
        {
            ptr = (char *)ps_malloc(aSize);
        }
    }

    // Finally, if the memory pointer is NULL, we were not able to allocate any memory, and that is a terminal condition.
    if (ptr == NULL)
    {
        ESP.restart();
    }
    return ptr;
}

// ======== Server Connection Handler Task ==========================
void mjpegCB(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(WSINTERVAL);

    // Creating frame synchronization semaphore and initializing it
    frameSync = xSemaphoreCreateBinary();
    xSemaphoreGive(frameSync);

    // Creating a queue to track all connected clients
    streamingClients = xQueueCreate(10, sizeof(WiFiClient *));
    activeClients = 0;

    //=== setup section  ==================

    //  Creating RTOS task for grabbing frames from the camera
    xTaskCreatePinnedToCore(
        camCB,    // callback
        "cam",    // name
        4096,     // stacj size
        NULL,     // parameters
        2,        // priority
        &tCam,    // RTOS task handle
        tskNO_AFFINITY); // core

    //  Creating task to push the stream to all connected clients
    xTaskCreatePinnedToCore(
        streamCB,
        "strmCB",
        4 * 1024,
        NULL, //(void*) handler,
        2,
        &tStream,
        tskNO_AFFINITY);

    //  Registering webserver handling routines
    server.on("/mjpeg/1", HTTP_GET, handleJPGSstream);
    server.on("/jpg", HTTP_GET, handleJPG);
    server.onNotFound(handleNotFound);

    //  Starting webserver
    server.begin();

    //=== loop() section  ===================
    xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        server.handleClient();

        //  After every server client handling request, we let other tasks run and then pause
        taskYIELD();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Commonly used variables:
volatile size_t camSize; // size of the current frame, byte
volatile char *camBuf;   // pointer to the current frame

// ==== RTOS task to grab frames from the camera =========================
void camCB(void *pvParameters)
{

    TickType_t xLastWakeTime;

    //  A running interval associated with currently desired frame rate
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / FPS);

    // Mutex for the critical section of swithing the active frames around
    portMUX_TYPE xSemaphore = portMUX_INITIALIZER_UNLOCKED;

    //=== loop() section  ===================
    xLastWakeTime = xTaskGetTickCount();

    //  Pointers to the 2 frames, their respective sizes and index of the current frame
    char *fbs[2] = {NULL, NULL};
    size_t fSize[2] = {0, 0};
    int ifb = 0;

    camera_fb_t *fb = NULL;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;

    for (;;)
    {

        //  Grab a frame from the camera and query its size
        // cam.run();
        // size_t s = cam.getSize();
        fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
        }
        else
        {

            if (fb->format != PIXFORMAT_JPEG)
            {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted)
                {
                    Serial.println("JPEG compression failed");
                }
            }
            else
            {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }

            //  If frame size is more that we have previously allocated - request  125% of the current frame space
            if (_jpg_buf_len > fSize[ifb])
            {
                fSize[ifb] = _jpg_buf_len * 4 / 3;
                fbs[ifb] = allocateMemory(fbs[ifb], fSize[ifb]);
            }

            //  Copy current frame into local buffer
            memcpy(fbs[ifb], _jpg_buf, _jpg_buf_len);
        }

        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        //  Let other tasks run and wait until the end of the current frame rate interval (if any time left)
        taskYIELD();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        //  Only switch frames around if no frame is currently being streamed to a client
        //  Wait on a semaphore until client operation completes
        xSemaphoreTake(frameSync, portMAX_DELAY);

        //  Do not allow interrupts while switching the current frame
        portENTER_CRITICAL(&xSemaphore);
        camBuf = fbs[ifb];
        camSize = _jpg_buf_len;
        ifb++;
        ifb &= 1; // this should produce 1, 0, 1, 0, 1 ... sequence
        portEXIT_CRITICAL(&xSemaphore);

        //  Let anyone waiting for a frame know that the frame is ready
        xSemaphoreGive(frameSync);

        //  Technically only needed once: let the streaming task know that we have at least one frame
        //  and it could start sending frames to the clients, if any
        xTaskNotifyGive(tStream);

        //  Immediately let other (streaming) tasks run
        taskYIELD();

        //  If streaming task has suspended itself (no active clients to stream to)
        //  there is no need to grab frames from the camera. We can save some juice
        //  by suspedning the tasks
        if (eTaskGetState(tStream) == eSuspended)
        {
            vTaskSuspend(NULL); // passing NULL means "suspend yourself"
        }
    }
}

// ==== STREAMING ======================================================
const char HEADER[] = "HTTP/1.1 200 OK\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

// ==== Handle connection request from clients ===============================
void handleJPGSstream(void)
{
    Serial.println("Handle stream request");
    //  Can only acommodate 10 clients. The limit is a default for WiFi connections
    if (!uxQueueSpacesAvailable(streamingClients))
        return;

    //  Create a new WiFi Client object to keep track of this one
    WiFiClient *client = new WiFiClient();
    *client = server.client();

    //  Immediately send this client a header
    client->write(HEADER, hdrLen);
    client->write(BOUNDARY, bdrLen);

    // Push the client to the streaming queue
    xQueueSend(streamingClients, (void *)&client, 0);

    // Wake up streaming tasks, if they were previously suspended:
    if (eTaskGetState(tCam) == eSuspended)
        vTaskResume(tCam);
    if (eTaskGetState(tStream) == eSuspended)
        vTaskResume(tStream);
}

// ==== Actually stream content to all connected clients ========================
void streamCB(void *pvParameters)
{
    Serial.println("Stream request");
    char buf[16];
    TickType_t xLastWakeTime;
    TickType_t xFrequency;

    //  Wait until the first frame is captured and there is something to send
    //  to clients
    ulTaskNotifyTake(pdTRUE,         /* Clear the notification value before exiting. */
                     portMAX_DELAY); /* Block indefinitely. */

    xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        // Default assumption we are running according to the FPS
        xFrequency = pdMS_TO_TICKS(1000 / FPS);

        //  Only bother to send anything if there is someone watching
        activeClients = uxQueueMessagesWaiting(streamingClients);

        if (activeClients)
        {
            // Adjust the period to the number of connected clients
            xFrequency /= activeClients;

            //  Since we are sending the same frame to everyone,
            //  pop a client from the the front of the queue
            WiFiClient *client;
            xQueueReceive(streamingClients, (void *)&client, 0);

            //  Check if this client is still connected.

            if (!client->connected())
            {
                //  delete this client reference if s/he has disconnected
                //  and don't put it back on the queue anymore. Bye!
                delete client;
            }
            else
            {

                //  Ok. This is an actively connected client.
                //  Let's grab a semaphore to prevent frame changes while we
                //  are serving this frame
                xSemaphoreTake(frameSync, portMAX_DELAY);

                client->write(CTNTTYPE, cntLen);
                sprintf(buf, "%d\r\n\r\n", camSize);
                client->write(buf, strlen(buf));
                client->write((char *)camBuf, (size_t)camSize);
                client->write(BOUNDARY, bdrLen);

                // Since this client is still connected, push it to the end
                // of the queue for further processing
                xQueueSend(streamingClients, (void *)&client, 0);

                //  The frame has been served. Release the semaphore and let other tasks run.
                //  If there is a frame switch ready, it will happen now in between frames
                xSemaphoreGive(frameSync);
                taskYIELD();
            }
        }
        else
        {
            //  Since there are no connected clients, there is no reason to waste battery running
            vTaskSuspend(NULL);
        }
        //  Let other tasks run after serving every client
        taskYIELD();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

const char JHEADER[] = "HTTP/1.1 200 OK\r\n"
                       "Content-disposition: inline; filename=capture.jpg\r\n"
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

// ==== Serve up one JPEG frame =============================================
void handleJPG(void)
{
    WiFiClient client = server.client();

    if (!client.connected())
        return;
    client.write(JHEADER, jhdLen);
    client.write((char *)camBuf, (size_t)camSize);
}

// ==== Handle invalid URL requests ============================================
void handleNotFound()
{
    String message = "Server is running!\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    server.send(200, "text / plain", message);
}

// ==== SETUP method ==================================================================
void startStreamServer()
{

    Serial.printf("Starting stream server on port: '%d'\n", streamPort);

    // Start mainstreaming RTOS task
    xTaskCreatePinnedToCore(
        mjpegCB,
        "mjpeg",
        4 * 1024,
        NULL,
        2,
        &tMjpeg,
        tskNO_AFFINITY);

    sprintf(streamURL, "http://%d.%d.%d.%d:%d/mjpeg/1/", espIP[0], espIP[1], espIP[2], espIP[3], streamPort);
}
