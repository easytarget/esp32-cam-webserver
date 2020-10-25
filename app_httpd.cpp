// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"
#include <WiFi.h>

#include "index_ov2640.h"
#include "index_ov3660.h"
#include "index_other.h"
#include "css.h"
#include "src/favicons.h"
#include "src/logo.h"
#include "storage.h"

#if !defined(HTTP_PORT)
    #define HTTP_PORT 80
#endif
int httpPort = HTTP_PORT;


// Functions from the main .ino
extern void flashLED(int flashtime);
extern void setLamp(int newVal);

// External variables declared in the main .ino
extern char myName[];
extern char myVer[];
extern char baseVersion[];
extern IPAddress ip;
extern IPAddress net;
extern IPAddress gw;
extern bool accesspoint;
extern char apName[];
extern bool captivePortal;
extern char httpURL[];
extern char streamURL[];
extern char default_index[];
extern int myRotation;
extern int lampVal;
extern int8_t detection_enabled;
extern int8_t recognition_enabled;
extern bool filesystem;
extern bool debugData;
extern int sketchSize;
extern int sketchSpace;
extern String sketchMD5;

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

#define ENROLL_CONFIRM_TIMES 5
typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321";
httpd_handle_t camera_httpd = NULL;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index)
    {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
    {
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;

    Serial.println("Capture Requested");

    flashLED(75); // little flash of status LED

    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t out_len, out_width, out_height;
    uint8_t *out_buf;
    bool s;
    if (!detection_enabled || fb->width > 400)
    {
        size_t fb_len = 0;
        if (fb->format == PIXFORMAT_JPEG)
        {
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        }
        else
        {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
        return res;
    }

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix)
    {
        esp_camera_fb_return(fb);
        Serial.println("dl_matrix3du_alloc failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if (!s)
    {
        dl_matrix3du_free(image_matrix);
        Serial.println("to rgb888 failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    dl_matrix3du_free(image_matrix);
    if (!s)
    {
        Serial.println("JPEG compression failed");
        return ESP_FAIL;
    }

    int64_t fr_end = esp_timer_get_time();
    if (debugData)
    {
        Serial.printf("FACE: %uB %ums\n", (uint32_t)(jchunk.len), (uint32_t)((fr_end - fr_start) / 1000));
    }
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char variable[32] = {
        0,
    };
    char value[32] = {
        0,
    };
    flashLED(75);

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
            {
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t *s = esp_camera_sensor_get();
    int res = 0;
    if (!strcmp(variable, "framesize"))
    {
        if (s->pixformat == PIXFORMAT_JPEG)
            res = s->set_framesize(s, (framesize_t)val);
    }
    else if (!strcmp(variable, "quality"))
        res = s->set_quality(s, val);
    else if (!strcmp(variable, "contrast"))
        res = s->set_contrast(s, val);
    else if (!strcmp(variable, "brightness"))
        res = s->set_brightness(s, val);
    else if (!strcmp(variable, "saturation"))
        res = s->set_saturation(s, val);
    else if (!strcmp(variable, "gainceiling"))
        res = s->set_gainceiling(s, (gainceiling_t)val);
    else if (!strcmp(variable, "colorbar"))
        res = s->set_colorbar(s, val);
    else if (!strcmp(variable, "awb"))
        res = s->set_whitebal(s, val);
    else if (!strcmp(variable, "agc"))
        res = s->set_gain_ctrl(s, val);
    else if (!strcmp(variable, "aec"))
        res = s->set_exposure_ctrl(s, val);
    else if (!strcmp(variable, "hmirror"))
        res = s->set_hmirror(s, val);
    else if (!strcmp(variable, "vflip"))
        res = s->set_vflip(s, val);
    else if (!strcmp(variable, "awb_gain"))
        res = s->set_awb_gain(s, val);
    else if (!strcmp(variable, "agc_gain"))
        res = s->set_agc_gain(s, val);
    else if (!strcmp(variable, "aec_value"))
        res = s->set_aec_value(s, val);
    else if (!strcmp(variable, "aec2"))
        res = s->set_aec2(s, val);
    else if (!strcmp(variable, "dcw"))
        res = s->set_dcw(s, val);
    else if (!strcmp(variable, "bpc"))
        res = s->set_bpc(s, val);
    else if (!strcmp(variable, "wpc"))
        res = s->set_wpc(s, val);
    else if (!strcmp(variable, "raw_gma"))
        res = s->set_raw_gma(s, val);
    else if (!strcmp(variable, "lenc"))
        res = s->set_lenc(s, val);
    else if (!strcmp(variable, "special_effect"))
        res = s->set_special_effect(s, val);
    else if (!strcmp(variable, "wb_mode"))
        res = s->set_wb_mode(s, val);
    else if (!strcmp(variable, "ae_level"))
        res = s->set_ae_level(s, val);
    else if (!strcmp(variable, "rotate"))
        myRotation = val;
    else if (!strcmp(variable, "face_detect"))
    {
        detection_enabled = val;
        if (!detection_enabled)
        {
            recognition_enabled = 0;
        }
    }
    else if (!strcmp(variable, "lamp") && (lampVal != -1))
    {
        lampVal = constrain(val, 0, 100);
        setLamp(lampVal);
    }
    else if (!strcmp(variable, "save_prefs"))
    {
        if (filesystem)
            savePrefs(SPIFFS);
    }
    else if (!strcmp(variable, "clear_prefs"))
    {
        if (filesystem)
            removePrefs(SPIFFS);
    }
    else if (!strcmp(variable, "reboot"))
    {
        Serial.print("REBOOT requested");
        for (int i = 0; i < 20; i++)
        {
            flashLED(50);
            delay(150);
            Serial.print('.');
        }
        Serial.printf(" Thats all folks!\n\n");
        ESP.restart();
    }
    else
    {
        res = -1;
    }
    if (res)
    {
        return httpd_resp_send_500(req);
    }
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    static char json_response[1024];
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
    p += sprintf(p, "\"cam_name\":\"%s\",", myName);
    p += sprintf(p, "\"code_ver\":\"%s\",", myVer);
    p += sprintf(p, "\"rotate\":\"%d\",", myRotation);
    p += sprintf(p, "\"stream_url\":\"%s\"", streamURL);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t favicon_16x16_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)favicon_16x16_png, favicon_16x16_png_len);
}

static esp_err_t favicon_32x32_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)favicon_32x32_png, favicon_32x32_png_len);
}

static esp_err_t favicon_ico_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)favicon_ico, favicon_ico_len);
}

static esp_err_t logo_svg_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)logo_svg, logo_svg_len);
}

static esp_err_t dump_handler(httpd_req_t *req)
{
    flashLED(75);
    Serial.println("\nDump Requested");
    Serial.print("Preferences file: ");
    dumpPrefs(SPIFFS);
    static char dumpOut[1200] = "";
    char *d = dumpOut;
    // Header
    d += sprintf(d, "<html><head><meta charset=\"utf-8\">\n");
    d += sprintf(d, "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n");
    d += sprintf(d, "<title>%s - Status</title>\n", myName);
    d += sprintf(d, "<link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/favicon-32x32.png\">\n");
    d += sprintf(d, "<link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/favicon-16x16.png\">\n");
    d += sprintf(d, "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n");
    d += sprintf(d, "</head>\n<body>\n");
    d += sprintf(d, "<img src=\"/logo.svg\" style=\"position: relative; float: right;\">\n");
    d += sprintf(d, "<h1>ESP32 Cam Webserver</h1>\n");
    // Module
    d += sprintf(d, "Name: %s<br>\n", myName);
    Serial.printf("Name: %s\n", myName);
    d += sprintf(d, "Firmware: %s (base: %s)<br>\n", myVer, baseVersion);
    Serial.printf("Firmware: %s (base: %s)\n", myVer, baseVersion);
    float sketchPct = 100 * sketchSize / sketchSpace;
    d += sprintf(d, "Sketch Size: %i (total: %i, %.1f%% used)<br>\n", sketchSize, sketchSpace, sketchPct);
    Serial.printf("Sketch Size: %i (total: %i, %.1f%% used)\n", sketchSize, sketchSpace, sketchPct);
    d += sprintf(d, "MD5: %s<br>\n", sketchMD5.c_str());
    Serial.printf("MD5: %s\n", sketchMD5.c_str());
    d += sprintf(d, "ESP sdk: %s<br>\n", ESP.getSdkVersion());
    Serial.printf("ESP sdk: %s\n", ESP.getSdkVersion());
    // Network
    d += sprintf(d, "<h2>WiFi</h2>\n");
    if (accesspoint)
    {
        if (captivePortal)
        {
            d += sprintf(d, "Mode: AccessPoint with captive portal<br>\n");
            Serial.printf("Mode: AccessPoint with captive portal\n");
        }
        else
        {
            d += sprintf(d, "Mode: AccessPoint<br>\n");
            Serial.printf("Mode: AccessPoint\n");
        }
        d += sprintf(d, "SSID: %s<br>\n", apName);
        Serial.printf("SSID: %s\n", apName);
    }
    else
    {
        d += sprintf(d, "Mode: Client<br>\n");
        Serial.printf("Mode: Client\n");
        String ssidName = WiFi.SSID();
        d += sprintf(d, "SSID: %s<br>\n", ssidName.c_str());
        Serial.printf("Ssid: %s\n", ssidName.c_str());
        d += sprintf(d, "Rssi: %i<br>\n", WiFi.RSSI());
        Serial.printf("Rssi: %i\n", WiFi.RSSI());
        String bssid = WiFi.BSSIDstr();
        d += sprintf(d, "BSSID: %s<br>\n", bssid.c_str());
        Serial.printf("BSSID: %s\n", bssid.c_str());
    }
    d += sprintf(d, "IP address: %d.%d.%d.%d<br>\n", ip[0], ip[1], ip[2], ip[3]);
    Serial.printf("IP address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    if (!accesspoint)
    {
        d += sprintf(d, "Netmask: %d.%d.%d.%d<br>\n", net[0], net[1], net[2], net[3]);
        Serial.printf("Netmask: %d.%d.%d.%d\n", net[0], net[1], net[2], net[3]);
        d += sprintf(d, "Gateway: %d.%d.%d.%d<br>\n", gw[0], gw[1], gw[2], gw[3]);
        Serial.printf("Gateway: %d.%d.%d.%d\n", gw[0], gw[1], gw[2], gw[3]);
    }
    d += sprintf(d, "Http port: %i<br>\n", httpPort);
    Serial.printf("Http port: %i\n", httpPort);
    byte mac[6];
    WiFi.macAddress(mac);
    d += sprintf(d, "MAC: %02X:%02X:%02X:%02X:%02X:%02X<br>\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // System
    d += sprintf(d, "<h2>System</h2>\n");
    int64_t sec = esp_timer_get_time() / 1000000;
    int64_t upDays = int64_t(floor(sec / 86400));
    int upHours = int64_t(floor(sec / 3600)) % 24;
    int upMin = int64_t(floor(sec / 60)) % 60;
    int upSec = sec % 60;
    d += sprintf(d, "Up: %" PRId64 ":%02i:%02i:%02i (d:h:m:s)<br>\n", upDays, upHours, upMin, upSec);
    Serial.printf("Up: %" PRId64 ":%02i:%02i:%02i (d:h:m:s)\n", upDays, upHours, upMin, upSec);
    d += sprintf(d, "Freq: %i MHz<br>\n", ESP.getCpuFreqMHz());
    Serial.printf("Freq: %i MHz\n", ESP.getCpuFreqMHz());
    d += sprintf(d, "Heap: %i, free: %i, min free: %i, max block: %i<br>\n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    Serial.printf("Heap: %i, free: %i, min free: %i, max block: %i\n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    d += sprintf(d, "Psram: %i, free: %i, min free: %i, max block: %i<br>\n", ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
    Serial.printf("Psram: %i, free: %i, min free: %i, max block: %i\n", ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
    if (filesystem)
    {
        d += sprintf(d, "Spiffs: %i, used: %i<br>\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
        Serial.printf("Spiffs: %i, used: %i\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
    }

    // Footer
    d += sprintf(d, "<br><div class=\"input-group\">\n");
    d += sprintf(d, "<button title=\"Refresh this page\" onclick=\"location.replace(document.URL)\">Refresh</button>\n");
    d += sprintf(d, "<button title=\"Close this page\" onclick=\"javascript:window.close()\">Close</button>\n");
    d += sprintf(d, "</div>\n</body>\n</html>\n");
    *d++ = 0;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, dumpOut, strlen(dumpOut));
}

static esp_err_t style_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)style_css, style_css_len);
}

static esp_err_t index_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char view[32] = {
        0,
    };

    flashLED(75);
    // See if we have a specific target (full/simple/portal) and serve as appropriate
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "view", view, sizeof(view)) == ESP_OK)
            {
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        // no target specified; default.
        strcpy(view, default_index);
        // If captive portal is active send that instead
        if (captivePortal)
        {
            strcpy(view, "portal");
        }
    }

    if (strncmp(view, "simple", sizeof(view)) == 0)
    {
        Serial.println("Simple index page requested");
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Content-Encoding", "identity");
        return httpd_resp_send(req, (const char *)index_simple_html, index_simple_html_len);
    }
    else if (strncmp(view, "full", sizeof(view)) == 0)
    {
        Serial.println("Full index page requested");
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Content-Encoding", "identity");
        sensor_t *s = esp_camera_sensor_get();
        if (s->id.PID == OV3660_PID)
        {
            return httpd_resp_send(req, (const char *)index_ov3660_html, index_ov3660_html_len);
        }
        return httpd_resp_send(req, (const char *)index_ov2640_html, index_ov2640_html_len);
    }
    else if (strncmp(view, "portal", sizeof(view)) == 0)
    {
        //Prototype captive portal landing page.
        Serial.println("Portal page requested");
        std::string s(portal_html);
        size_t index;
        while ((index = s.find("<APPURL>")) != std::string::npos)
            s.replace(index, strlen("<APPURL>"), httpURL);
        while ((index = s.find("<STREAMURL>")) != std::string::npos)
            s.replace(index, strlen("<STREAMURL>"), streamURL);
        while ((index = s.find("<CAMNAME>")) != std::string::npos)
            s.replace(index, strlen("<CAMNAME>"), myName);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Content-Encoding", "identity");
        return httpd_resp_send(req, (const char *)s.c_str(), s.length());
    }
    else
    {
        Serial.print("Unknown page requested: ");
        Serial.println(view);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
}

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12; // we use more than the default 8 (on port 80)

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};
    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL};
    httpd_uri_t cmd_uri = {
        .uri = "/control",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = NULL};
    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL};
    httpd_uri_t style_uri = {
        .uri = "/style.css",
        .method = HTTP_GET,
        .handler = style_handler,
        .user_ctx = NULL};
    httpd_uri_t favicon_16x16_uri = {
        .uri = "/favicon-16x16.png",
        .method = HTTP_GET,
        .handler = favicon_16x16_handler,
        .user_ctx = NULL};
    httpd_uri_t favicon_32x32_uri = {
        .uri = "/favicon-32x32.png",
        .method = HTTP_GET,
        .handler = favicon_32x32_handler,
        .user_ctx = NULL};
    httpd_uri_t favicon_ico_uri = {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = favicon_ico_handler,
        .user_ctx = NULL};
    httpd_uri_t logo_svg_uri = {
        .uri = "/logo.svg",
        .method = HTTP_GET,
        .handler = logo_svg_handler,
        .user_ctx = NULL};
    httpd_uri_t dump_uri = {
        .uri = "/dump",
        .method = HTTP_GET,
        .handler = dump_handler,
        .user_ctx = NULL};

    config.server_port = httpPort;
    config.ctrl_port = httpPort;
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        // Note; config.max_uri_handlers (above) must be >= the number of handlers
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
        httpd_register_uri_handler(camera_httpd, &style_uri);
        httpd_register_uri_handler(camera_httpd, &favicon_16x16_uri);
        httpd_register_uri_handler(camera_httpd, &favicon_32x32_uri);
        httpd_register_uri_handler(camera_httpd, &favicon_ico_uri);
        httpd_register_uri_handler(camera_httpd, &logo_svg_uri);
        httpd_register_uri_handler(camera_httpd, &dump_uri);
    }
}
