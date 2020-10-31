#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_bt.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "myconfig.h"
#include "esp_camera.h"
#include "camera_pins.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>

#include "src/html/firmware.h"
#include "src/html/index_ov2640.h"
#include "src/html/index_ov3660.h"
#include "src/html/index_simple.h"
#include "src/html/css.h"
#include "src/favicons.h"
#include "src/logo.h"
#include "storage.h"

// Functions from the main .ino
extern void flashLED(int flashtime);
extern void setLamp(int newVal);
extern void resetWiFiConfig();

// External variables declared in the main .ino
extern char camera_name[];
extern char myVer[];
extern char baseVersion[];
extern IPAddress espIP;
extern IPAddress espSubnet;
extern IPAddress espGateway;
extern char streamURL[];
extern char default_index[];
extern int myRotation;
extern int lampVal;
extern bool filesystem;
extern bool debugData;
extern int sketchSize;
extern int sketchSpace;
extern String sketchMD5;

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

#if !defined(HTTP_PORT)
#define HTTP_PORT 80
#endif
int httpPort = HTTP_PORT;
char httpURL[64] = {"Undefined"};

AsyncWebServer appServer(httpPort);

bool updating;

void appServerCB(void *pvParameters);
size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len);
void capture_handler(AsyncWebServerRequest *request);
void cmd_handler(AsyncWebServerRequest *request);
void status_handler(AsyncWebServerRequest *request);
void favicon_16x16_handler(AsyncWebServerRequest *request);
void favicon_32x32_handler(AsyncWebServerRequest *request);
void favicon_ico_handler(AsyncWebServerRequest *request);
void logo_svg_handler(AsyncWebServerRequest *request);
void dump_handler(AsyncWebServerRequest *request);
void style_handler(AsyncWebServerRequest *request);
void handleIndex(AsyncWebServerRequest *request);
void handleFirmware(AsyncWebServerRequest *request);
void handleCameraClient();
void startCameraServer();

#define PART_BOUNDARY "123456789000000000000987654321";

void capture_handler(AsyncWebServerRequest *request)
{
    camera_fb_t *fb = NULL;

    Serial.println("Capture Requested");

    flashLED(75); // little flash of status LED

    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        request->send(500, "text/plain", "Camera Capture Failed");
        return;
    }

    size_t fb_len = 0;
    if (fb->format == PIXFORMAT_JPEG)
    {
        fb_len = fb->len;
        request->send(200, "image/jpeg", (const char *)fb->buf);
    }
    else
    {
        request->send(500, "text/plain", "Camera Capture Failed");
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    Serial.printf("JPG: %uB %ums\r\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
}

void cmd_handler(AsyncWebServerRequest *request)
{
    Serial.println("Command received!");
    flashLED(75);

    if (!request->hasArg("var") || !request->hasArg("val"))
    {
        Serial.println("No var or val");
        request->send(404, "text/plain", "Invalid parameters");
        return;
    }

    String variable = request->arg("var").c_str();
    String value = request->arg("val").c_str();

    Serial.print("Var ");
    Serial.println(variable);

    int val = atoi(value.c_str());
    sensor_t *s = esp_camera_sensor_get();
    int res = 0;
    if (variable.compareTo("framesize") == 0)
    {
        if (s->pixformat == PIXFORMAT_JPEG)
            res = s->set_framesize(s, (framesize_t)val);
    }
    else if (variable.compareTo("quality") == 0)
        res = s->set_quality(s, val);
    else if (variable.compareTo("contrast") == 0)
        res = s->set_contrast(s, val);
    else if (variable.compareTo("brightness") == 0)
        res = s->set_brightness(s, val);
    else if (variable.compareTo("saturation") == 0)
        res = s->set_saturation(s, val);
    else if (variable.compareTo("gainceiling") == 0)
        res = s->set_gainceiling(s, (gainceiling_t)val);
    else if (variable.compareTo("colorbar") == 0)
        res = s->set_colorbar(s, val);
    else if (variable.compareTo("awb") == 0)
        res = s->set_whitebal(s, val);
    else if (variable.compareTo("agc") == 0)
        res = s->set_gain_ctrl(s, val);
    else if (variable.compareTo("aec") == 0)
        res = s->set_exposure_ctrl(s, val);
    else if (variable.compareTo("hmirror") == 0)
        res = s->set_hmirror(s, val);
    else if (variable.compareTo("vFlip") == 0)
        res = s->set_vflip(s, val);
    else if (variable.compareTo("awb_gain") == 0)
        res = s->set_awb_gain(s, val);
    else if (variable.compareTo("agc_gain") == 0)
        res = s->set_agc_gain(s, val);
    else if (variable.compareTo("aec_value") == 0)
        res = s->set_aec_value(s, val);
    else if (variable.compareTo("aec2") == 0)
        res = s->set_aec2(s, val);
    else if (variable.compareTo("dcw") == 0)
        res = s->set_dcw(s, val);
    else if (variable.compareTo("bpc") == 0)
        res = s->set_bpc(s, val);
    else if (variable.compareTo("wpc") == 0)
        res = s->set_wpc(s, val);
    else if (variable.compareTo("raw_gma") == 0)
        res = s->set_raw_gma(s, val);
    else if (variable.compareTo("lenc") == 0)
        res = s->set_lenc(s, val);
    else if (variable.compareTo("special_effect") == 0)
        res = s->set_special_effect(s, val);
    else if (variable.compareTo("wb_mode") == 0)
        res = s->set_wb_mode(s, val);
    else if (variable.compareTo("ae_level") == 0)
        res = s->set_ae_level(s, val);
    else if (variable.compareTo("rotate") == 0)
        myRotation = val;
    else if (variable.compareTo("lamp") == 0)
    {
        lampVal = constrain(val, 0, 100);
        setLamp(lampVal);
    }
    else if (variable.compareTo("save_prefs") == 0)
    {
        if (filesystem)
            savePrefs(SPIFFS);
    }
    else if (variable.compareTo("clear_prefs") == 0)
    {
        if (filesystem)
            removePrefs(SPIFFS);
    }
    else if (variable.compareTo("reboot") == 0)

    {
        request->send(200, "text/plain", "Rebooting...");
        Serial.print("REBOOT requested");
        for (int i = 0; i < 20; i++)
        {
            flashLED(50);
            delay(150);
            Serial.print('.');
        }
        Serial.printf(" Thats all folks!\r\n\r\n");
        ESP.restart();
    }
    else if (variable.compareTo("clear_wifi") == 0)
    {
        request->send(200, "text/plain", "Reseting WiFi...");
        Serial.println("Wifi reset requested");
        resetWiFiConfig();
    }
    else
    {
        res = -1;
    }
    if (res)
    {
        Serial.print("Unable to determine command: ");
        Serial.println(variable);

        request->send(404, "text/plain", "Invalid parameters");
        return;
    }
    request->send(200);
    return;
}

void status_handler(AsyncWebServerRequest *request)
{
    char json_response[1024];
    sensor_t *s = esp_camera_sensor_get();
    char *p = json_response;
    *p++ = '{';
    p += sprintf(p, "\"lamp\":%d,", lampVal);
    p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p += sprintf(p, "\"quality\":%u,", s->status.quality);
    p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p += sprintf(p, "\"awb\":%u,", s->status.awb);
    p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p += sprintf(p, "\"aec\":%u,", s->status.aec);
    p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p += sprintf(p, "\"agc\":%u,", s->status.agc);
    p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
    p += sprintf(p, "\"cam_name\":\"%s\",", camera_name);
    p += sprintf(p, "\"code_ver\":\"%s\",", myVer);
    p += sprintf(p, "\"rotate\":\"%d\",", myRotation);
    p += sprintf(p, "\"stream_url\":\"%s\"", streamURL);
    *p++ = '}';
    *p++ = 0;

    request->send(200, "application/json", json_response);
}

void favicon_16x16_handler(AsyncWebServerRequest *request)
{
    request->send(200, "image/png", (const char *)favicon_16x16_png);
}

void favicon_32x32_handler(AsyncWebServerRequest *request)
{
    request->send(200, "image/png", (const char *)favicon_32x32_png);
}

void favicon_ico_handler(AsyncWebServerRequest *request)
{
    request->send(200, "image/png", (const char *)favicon_ico);
}

void logo_svg_handler(AsyncWebServerRequest *request)
{
    request->send(200, "image/svg+xml", (const char *)logo_svg);
}

void dump_handler(AsyncWebServerRequest *request)
{
    flashLED(75);
    Serial.println("\r\nDump Requested");
    Serial.print("Preferences file: ");
    dumpPrefs(SPIFFS);
    char dumpOut[1200] = "";
    char *d = dumpOut;
    // Header
    d += sprintf(d, "<html><head><meta charset=\"utf-8\">\r\n");
    d += sprintf(d, "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\r\n");
    d += sprintf(d, "<title>%s - Status</title>\r\n", camera_name);
    d += sprintf(d, "<link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/favicon-32x32.png\">\r\n");
    d += sprintf(d, "<link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/favicon-16x16.png\">\r\n");
    d += sprintf(d, "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\r\n");
    d += sprintf(d, "</head>\r\n<body>\r\n");
    d += sprintf(d, "<img src=\"/logo.svg\" style=\"position: relative; float: right;\">\r\n");
    d += sprintf(d, "<h1>ESP32 Cam Webserver</h1>\r\n");
    // Module
    d += sprintf(d, "Name: %s<br>\r\n", camera_name);
    Serial.printf("Name: %s\r\n", camera_name);
    d += sprintf(d, "Firmware: %s (base: %s)<br>\r\n", myVer, baseVersion);
    Serial.printf("Firmware: %s (base: %s)\r\n", myVer, baseVersion);
    float sketchPct = 100 * sketchSize / sketchSpace;
    d += sprintf(d, "Sketch Size: %i (total: %i, %.1f%% used)<br>\r\n", sketchSize, sketchSpace, sketchPct);
    Serial.printf("Sketch Size: %i (total: %i, %.1f%% used)\r\n", sketchSize, sketchSpace, sketchPct);
    d += sprintf(d, "MD5: %s<br>\r\n", sketchMD5.c_str());
    Serial.printf("MD5: %s\r\n", sketchMD5.c_str());
    d += sprintf(d, "ESP sdk: %s<br>\r\n", ESP.getSdkVersion());
    Serial.printf("ESP sdk: %s\r\n", ESP.getSdkVersion());
    // Network
    d += sprintf(d, "<h2>WiFi</h2>\r\n");
    d += sprintf(d, "Mode: Client<br>\r\n");
    Serial.printf("Mode: Client\r\n");
    String ssidName = WiFi.SSID();
    d += sprintf(d, "SSID: %s<br>\r\n", ssidName.c_str());
    Serial.printf("Ssid: %s\r\n", ssidName.c_str());
    d += sprintf(d, "Rssi: %i<br>\r\n", WiFi.RSSI());
    Serial.printf("Rssi: %i\r\n", WiFi.RSSI());
    String bssid = WiFi.BSSIDstr();
    d += sprintf(d, "BSSID: %s<br>\r\n", bssid.c_str());
    Serial.printf("BSSID: %s\r\n", bssid.c_str());

    d += sprintf(d, "IP address: %d.%d.%d.%d<br>\r\n", espIP[0], espIP[1], espIP[2], espIP[3]);
    Serial.printf("IP address: %d.%d.%d.%d\r\n", espIP[0], espIP[1], espIP[2], espIP[3]);
    d += sprintf(d, "Http port: %i<br>\r\n", httpPort);
    Serial.printf("Http port: %i\r\n", httpPort);
    byte mac[6];
    WiFi.macAddress(mac);
    d += sprintf(d, "MAC: %02X:%02X:%02X:%02X:%02X:%02X<br>\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // System
    d += sprintf(d, "<h2>System</h2>\r\n");
    int64_t sec = esp_timer_get_time() / 1000000;
    int64_t upDays = int64_t(floor(sec / 86400));
    int upHours = int64_t(floor(sec / 3600)) % 24;
    int upMin = int64_t(floor(sec / 60)) % 60;
    int upSec = sec % 60;
    d += sprintf(d, "Up: %" PRId64 ":%02i:%02i:%02i (d:h:m:s)<br>\r\n", upDays, upHours, upMin, upSec);
    Serial.printf("Up: %" PRId64 ":%02i:%02i:%02i (d:h:m:s)\r\n", upDays, upHours, upMin, upSec);
    d += sprintf(d, "Freq: %i MHz<br>\r\n", ESP.getCpuFreqMHz());
    Serial.printf("Freq: %i MHz\r\n", ESP.getCpuFreqMHz());
    d += sprintf(d, "Heap: %i, free: %i, min free: %i, max block: %i<br>\r\n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    Serial.printf("Heap: %i, free: %i, min free: %i, max block: %i\r\n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    d += sprintf(d, "Psram: %i, free: %i, min free: %i, max block: %i<br>\r\n", ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
    Serial.printf("Psram: %i, free: %i, min free: %i, max block: %i\r\n", ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
    if (filesystem)
    {
        d += sprintf(d, "Spiffs: %i, used: %i<br>\r\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
        Serial.printf("Spiffs: %i, used: %i\r\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
    }

    // Footer
    d += sprintf(d, "<br><div class=\"input-group\">\r\n");
    d += sprintf(d, "<button title=\"Refresh this page\" onclick=\"location.replace(document.URL)\">Refresh</button>\r\n");
    d += sprintf(d, "<button title=\"Close this page\" onclick=\"javascript:window.close()\">Close</button>\r\n");
    d += sprintf(d, "</div>\r\n</body>\r\n</html>\r\n");
    *d++ = 0;

    request->send(200, "image/html", dumpOut);
}

void style_handler(AsyncWebServerRequest *request)
{
    request->send(200, "text/css", (const char *)style_css);
}

void handleIndex(AsyncWebServerRequest *request)
{

    flashLED(150);
    // See if we have a specific target (full/simple/portal) and serve as appropriate
    String view = default_index;

    if (request->hasArg("view"))
    {
        view = request->arg("view").c_str();
    }

    if (strncmp(view.c_str(), "simple", sizeof(view)) == 0)
    {
        Serial.println("Simple index page requested");
        request->send_P(200, "text/html", (const char *)index_simple_html);
        return;
    }
    else if (strncmp(view.c_str(), "full", sizeof(view)) == 0)
    {
        
        sensor_t *s = esp_camera_sensor_get();

        if (s->id.PID == OV3660_PID)
        {
            Serial.println("Full OV3660 index page requested");
            request->send_P(200, "text/html", (const char *)index_ov3660_html);
            return;
        }

        Serial.println("Full OV2640 index page requested");
        request->send_P(200, "text/html",(const char *)index_ov2640_html);
        
        return;
    }

    Serial.print("Unknown page requested: ");
    Serial.println(view);
    request->send(404, "text/plain", "Unknown page requested");
}

void handleFirmware(AsyncWebServerRequest *request)
{

    flashLED(75);
    // See if we have a specific target (full/simple/portal) and serve as appropriate

    Serial.println("Firmware page requested");
    request->send(200, "text/html", firmware_html.c_str());
}

String getHTMLHead()
{
    String header = F("<!DOCTYPE html><html lang=\"en\"><head>");
    header += F("<link href=\"/local.css\" rel=\"stylesheet\">");
    header += F("</head>");
    header += F("<body>");
    return header;
}

String getHTMLFoot()
{
    return F("</body></html>");
}
void handleUpdate(AsyncWebServerRequest *request)
{
    String response_message;
    response_message.reserve(1000);
    response_message = getHTMLHead();
    response_message += "<script> function notify_update() {document.getElementById(\"update\").innerHTML = \"<h2>Updating...</h2>\"\; } </script>";
    response_message += "Firmware = *.esp32.bin<br>SPIFFS = *.spiffs.bin<br> \
  <form method='POST' action='/doUpdate' enctype='multipart/form-data' target='_self' onsubmit='notify_update()'> \
  <input type='file' name='update'><br> \
  <input type='submit' value='Do update'></form> \
  <div id=\"update\"></div>";
    response_message += getHTMLFoot();
    request->send(200, "text/html", response_message);
};

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    updating = true;
    delay(500);

    if (!index)
    {
        // check file names for type
        int cmd = (filename.indexOf(F(".spiffs.bin")) > -1) ? U_SPIFFS : U_FLASH;
        if (cmd == U_FLASH && !(filename.indexOf(F("esp32.bin")) > -1))
            return; // wrong image for ESP32
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
        {
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len)
    {
        Update.printError(Serial);
    }

    if (final)
    {
        if (!Update.end(true))
        {
            Update.printError(Serial);
        }
        else
        {
            String response_message;
            response_message.reserve(1000);
            response_message = getHTMLHead();
            response_message += "<h2>Please wait while the device reboots</h2> <meta http-equiv=\"refresh\" content=\"20;url=/\" />";
            response_message += getHTMLFoot();
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html", response_message);
            response->addHeader("Refresh", "20");
            response->addHeader("Location", "/");
            request->send(response);
            delay(100);
            ESP.restart();
        }
    }
}

void startCameraServer()
{
    Serial.printf("Starting web server on port: '%d'\r\n", httpPort);

    appServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        handleIndex(request);
    });
    appServer.on("/firmware", HTTP_GET, handleFirmware);
    appServer.on("/status", HTTP_GET, status_handler);
    appServer.on("/control", HTTP_GET, cmd_handler);
    appServer.on("/capture", HTTP_GET, capture_handler);
    appServer.on("/style.css", HTTP_GET, style_handler);
    appServer.on("/favicon-16x16.png", HTTP_GET, favicon_16x16_handler);
    appServer.on("/favicon-32x32.png", HTTP_GET, favicon_32x32_handler);
    appServer.on("/favicon.ico", HTTP_GET, favicon_ico_handler);
    appServer.on("/logo.svg", HTTP_GET, logo_svg_handler);
    appServer.on("/dump", HTTP_GET, dump_handler);
    /*handling uploading firmware file */
    appServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        handleUpdate(request);
    });
    appServer.on(
        "/doUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) { handleDoUpdate(request, filename, index, data, len, final); });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    appServer.begin();
    Serial.println("Web server started!");

    sprintf(httpURL, "http://%d.%d.%d.%d:%d/", espIP[0], espIP[1], espIP[2], espIP[3], httpPort);
}
