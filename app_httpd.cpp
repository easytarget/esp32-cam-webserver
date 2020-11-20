// Original Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#include <esp_http_server.h>
#include <esp_timer.h>
#include <esp_camera.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <img_converters.h>
#include <Arduino.h>
#include <WiFi.h>

#include "index_ov2640.h"
#include "index_ov3660.h"
#include "index_other.h"
#include "css.h"
#include "src/favicons.h"
#include "src/logo.h"
#include "storage.h"

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
extern int httpPort;
extern int streamPort;
extern char httpURL[];
extern char streamURL[];
extern char default_index[];
extern int8_t streamCount;
extern int myRotation;
extern int lampVal;
extern bool autoLamp;
extern int8_t detection_enabled;
extern int8_t recognition_enabled;
extern bool filesystem;
extern String critERR;
extern bool debugData;
extern int sketchSize;
extern int sketchSpace;
extern String sketchMD5;

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

#define FACE_COLOR_WHITE  0x00FFFFFF
#define FACE_COLOR_BLACK  0x00000000
#define FACE_COLOR_RED    0x000000FF
#define FACE_COLOR_GREEN  0x0000FF00
#define FACE_COLOR_BLUE   0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN   (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

typedef struct {
        size_t size; //number of values used for filtering
        size_t index; //current value index
        size_t count; //value count
        int sum;
        int * values; //array to be filled with values
} ra_filter_t;

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static mtmn_config_t mtmn_config = {0};
static int8_t is_enrolling = 0;
static face_id_list id_list = {0};
int id_list_alloc = 0;

static ra_filter_t * ra_filter_init(ra_filter_t * filter, size_t sample_size){
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if(!filter->values){
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t * filter, int value) {
    if(!filter->values){
      return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size) {
      filter->count++;
    }
    return filter->sum / filter->count;
}

static void rgb_print(dl_matrix3du_t *image_matrix, uint32_t color, const char * str){
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, 10, color, str);
}

static int rgb_printf(dl_matrix3du_t *image_matrix, uint32_t color, const char *format, ...){
    char loc_buf[64];
    char * temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = (char*)malloc(len+1);
        if(temp == NULL) {
            return 0;
        }
    }
    vsnprintf(temp, len+1, format, arg);
    va_end(arg);
    rgb_print(image_matrix, color, temp);
    if(len > 64){
        free(temp);
    }
    return len;
}

static void draw_face_boxes(dl_matrix3du_t *image_matrix, box_array_t *boxes, int face_id){
    int x, y, w, h, i;
    uint32_t color = FACE_COLOR_YELLOW;
    if(face_id < 0){
        color = FACE_COLOR_RED;
    } else if(face_id > 0){
        color = FACE_COLOR_GREEN;
    }
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    for (i = 0; i < boxes->len; i++){
        // rectangle box
        x = (int)boxes->box[i].box_p[0];
        y = (int)boxes->box[i].box_p[1];
        w = (int)boxes->box[i].box_p[2] - x + 1;
        h = (int)boxes->box[i].box_p[3] - y + 1;
        fb_gfx_drawFastHLine(&fb, x, y, w, color);
        fb_gfx_drawFastHLine(&fb, x, y+h-1, w, color);
        fb_gfx_drawFastVLine(&fb, x, y, h, color);
        fb_gfx_drawFastVLine(&fb, x+w-1, y, h, color);
        #if 0
        // landmark
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)boxes->landmark[i].landmark_p[j];
            y0 = (int)boxes->landmark[i].landmark_p[j+1];
            fb_gfx_fillRect(&fb, x0, y0, 3, 3, color);
        }
        #endif
    }
}

static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes){
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if(!aligned_face){
        Serial.println("Could not allocate face recognition buffer");
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
        if (is_enrolling == 1){
            int8_t this_face = id_list.tail;
            int8_t left_sample_face = enroll_face(&id_list, aligned_face);

            if(left_sample_face == (ENROLL_CONFIRM_TIMES - 1)){
                Serial.printf("Enrolling Face ID: %d\n", this_face);
            }
            Serial.printf("Enrolling Face ID: %d sample %d\n", this_face, ENROLL_CONFIRM_TIMES - left_sample_face);
            rgb_printf(image_matrix, FACE_COLOR_CYAN, "ID[%u] Sample[%u]", this_face, ENROLL_CONFIRM_TIMES - left_sample_face);
            if (left_sample_face == 0){
                is_enrolling = 0;
                Serial.printf("Enrolled Face ID: %d\n", this_face);
            }
        } else {
            matched_id = recognize_face(&id_list, aligned_face);
            if (matched_id >= 0) {
                Serial.printf("Match Face ID: %u\n", matched_id);
                rgb_printf(image_matrix, FACE_COLOR_GREEN, "Hello Subject %u", matched_id);
            } else {
                Serial.println("No Match Found");
                rgb_print(image_matrix, FACE_COLOR_RED, "Intruder Alert!");
                matched_id = -1;
            }
        }
    } else {
        Serial.println("Face Not Aligned");
        //rgb_print(image_matrix, FACE_COLOR_YELLOW, "Human Detected");
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    Serial.println("Capture Requested");
    if (autoLamp && (lampVal != -1)) setLamp(lampVal);
    flashLED(75); // little flash of status LED

    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t out_len, out_width, out_height;
    uint8_t * out_buf;
    bool s;
    bool detected = false;
    int face_id = 0;
    if(!detection_enabled || fb->width > 400){
        size_t fb_len = 0;
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        if (debugData) {
            Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start)/1000));
        }
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return res;
    }

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix) {
        esp_camera_fb_return(fb);
        Serial.println("dl_matrix3du_alloc failed");
        httpd_resp_send_500(req);
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return ESP_FAIL;
    }

    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if(!s){
        dl_matrix3du_free(image_matrix);
        Serial.println("to rgb888 failed");
        httpd_resp_send_500(req);
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return ESP_FAIL;
    }

    box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

    if (net_boxes){
        detected = true;
        if(recognition_enabled){
            face_id = run_face_recognition(image_matrix, net_boxes);
        }
        draw_face_boxes(image_matrix, net_boxes, face_id);
        free(net_boxes->score);
        free(net_boxes->box);
        free(net_boxes->landmark);
        free(net_boxes);
    }

    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    dl_matrix3du_free(image_matrix);
    if(!s){
        Serial.println("JPEG compression failed");
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return ESP_FAIL;
    }

    int64_t fr_end = esp_timer_get_time();
    if (debugData) {
        Serial.printf("FACE: %uB %ums %s%d\n", (uint32_t)(jchunk.len), (uint32_t)((fr_end - fr_start)/1000), detected?"DETECTED ":"", face_id);
    }
    if (autoLamp && (lampVal != -1)) setLamp(0);
    return res;
}

static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;
    int face_id = 0;
    bool detected = false;
    int64_t fr_start = 0;
    int64_t fr_face = 0;
    int64_t fr_recognize = 0;
    int64_t fr_encode = 0;
    int64_t fr_ready = 0;

    Serial.println("Stream requested");
    if (autoLamp && (lampVal != -1)) setLamp(lampVal);
    streamCount = 1;  // at present we only have one stream handler..
    flashLED(75);     // double flash of status LED
    delay(75);
    flashLED(75);

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while(true){
        detected = false;
        face_id = 0;
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_face = fr_start; 
            fr_encode = fr_start;
            fr_recognize = fr_start;
            if(!detection_enabled || fb->width > 400){
                if(fb->format != PIXFORMAT_JPEG){
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            } else {

                image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

                if (!image_matrix) {
                    Serial.println("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                } else {
                    if(!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)){
                        Serial.println("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    } else {
                        fr_ready = esp_timer_get_time();
                        box_array_t *net_boxes = NULL;
                        if(detection_enabled){
                            net_boxes = face_detect(image_matrix, &mtmn_config);
                        }
                        fr_face = esp_timer_get_time();
                        fr_recognize = fr_face;
                        if (net_boxes || fb->format != PIXFORMAT_JPEG){
                            if(net_boxes){
                                detected = true;
                                if(recognition_enabled){
                                    face_id = run_face_recognition(image_matrix, net_boxes);
                                }
                                fr_recognize = esp_timer_get_time();
                                draw_face_boxes(image_matrix, net_boxes, face_id);
                                free(net_boxes->score);
                                free(net_boxes->box);
                                free(net_boxes->landmark);
                                free(net_boxes);
                            }
                            if(!fmt2jpg(image_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)){
                                Serial.println("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            esp_camera_fb_return(fb);
                            fb = NULL;
                        } else {
                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                        fr_encode = esp_timer_get_time();
                    }
                    dl_matrix3du_free(image_matrix);
                }
            }
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t ready_time = (fr_ready - fr_start)/1000;
        int64_t face_time = (fr_face - fr_ready)/1000;
        int64_t recognize_time = (fr_recognize - fr_face)/1000;
        int64_t encode_time = (fr_encode - fr_recognize)/1000;
        int64_t process_time = (fr_encode - fr_start)/1000;
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        if (debugData) {
            Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u+%u+%u=%u %s%d\n",
                (uint32_t)(_jpg_buf_len),
                (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                avg_frame_time, 1000.0 / avg_frame_time,
                (uint32_t)ready_time, (uint32_t)face_time, (uint32_t)recognize_time, (uint32_t)encode_time, (uint32_t)process_time,
                (detected)?"DETECTED ":"", face_id
                 );
        }
    }

    if (autoLamp && (lampVal != -1)) setLamp(0);
    streamCount = 0;  // at present we only have one stream handler..
    Serial.println("Stream ended");
    last_frame = 0;
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req){
    char*  buf;
    size_t buf_len;
    char variable[32] = {0,};
    char value[32] = {0,};
    flashLED(75);

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;
    if(!strcmp(variable, "framesize")) {
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(!strcmp(variable, "quality")) res = s->set_quality(s, val);
    else if(!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
    else if(!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
    else if(!strcmp(variable, "saturation")) res = s->set_saturation(s, val);
    else if(!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);
    else if(!strcmp(variable, "awb")) res = s->set_whitebal(s, val);
    else if(!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
    else if(!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
    else if(!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);
    else if(!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);
    else if(!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);
    else if(!strcmp(variable, "aec2")) res = s->set_aec2(s, val);
    else if(!strcmp(variable, "dcw")) res = s->set_dcw(s, val);
    else if(!strcmp(variable, "bpc")) res = s->set_bpc(s, val);
    else if(!strcmp(variable, "wpc")) res = s->set_wpc(s, val);
    else if(!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);
    else if(!strcmp(variable, "lenc")) res = s->set_lenc(s, val);
    else if(!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);
    else if(!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);
    else if(!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);
    else if(!strcmp(variable, "rotate")) myRotation = val;
    else if(!strcmp(variable, "face_detect")) {
        detection_enabled = val;
        if(!detection_enabled) {
            recognition_enabled = 0;
        }
    }
    else if(!strcmp(variable, "face_enroll")) is_enrolling = val;
    else if(!strcmp(variable, "face_recognize")) {
        recognition_enabled = val;
        if(recognition_enabled){
            detection_enabled = val;
        }
    }
    else if(!strcmp(variable, "autolamp") && (lampVal != -1)) {
        autoLamp = val;
        if (autoLamp) {
           if (streamCount > 0) setLamp(lampVal);
           else setLamp(0);
        } else {
            setLamp(lampVal);
        }
    }
    else if(!strcmp(variable, "lamp") && (lampVal != -1)) {
        lampVal = constrain(val,0,100);
        if (autoLamp) {
           if (streamCount > 0) setLamp(lampVal);
           else setLamp(0);
        } else {
            setLamp(lampVal);
        }
    }
    else if(!strcmp(variable, "save_face")) {
        if (filesystem) saveFaceDB(SPIFFS);
    }
    else if(!strcmp(variable, "clear_face")) {
        if (filesystem) removeFaceDB(SPIFFS);
    }
    else if(!strcmp(variable, "save_prefs")) {
        if (filesystem) savePrefs(SPIFFS);
    }
    else if(!strcmp(variable, "clear_prefs")) {
        if (filesystem) removePrefs(SPIFFS);
    }
    else if(!strcmp(variable, "reboot")) {
        esp_task_wdt_init(3,true);  // schedule a a watchdog panic event for 3 seconds in the future
        esp_task_wdt_add(NULL);
        periph_module_disable(PERIPH_I2C0_MODULE); // try to shut I2C down properly
        periph_module_disable(PERIPH_I2C1_MODULE);
        periph_module_reset(PERIPH_I2C0_MODULE);
        periph_module_reset(PERIPH_I2C1_MODULE);
        Serial.print("REBOOT requested");
        while(true) {
          flashLED(50);
          delay(150);
          Serial.print('.');
        }
    }
    else {
        res = -1;
    }
    if(res){
        return httpd_resp_send_500(req);
    }
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req){
    static char json_response[1024];
    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';
    p+=sprintf(p, "\"lamp\":%d,", lampVal);
    p+=sprintf(p, "\"autolamp\":%d,", autoLamp);
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
    p+=sprintf(p, "\"face_enroll\":%u,", is_enrolling);
    p+=sprintf(p, "\"face_recognize\":%u,", recognition_enabled);
    p+=sprintf(p, "\"cam_name\":\"%s\",", myName);
    p+=sprintf(p, "\"code_ver\":\"%s\",", myVer);
    p+=sprintf(p, "\"rotate\":\"%d\",", myRotation);
    p+=sprintf(p, "\"stream_url\":\"%s\"", streamURL);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t info_handler(httpd_req_t *req){
    static char json_response[256];
    char * p = json_response;
    *p++ = '{';
    p+=sprintf(p, "\"cam_name\":\"%s\",", myName);
    p+=sprintf(p, "\"rotate\":\"%d\",", myRotation);
    p+=sprintf(p, "\"stream_url\":\"%s\"", streamURL);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t favicon_16x16_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)favicon_16x16_png, favicon_16x16_png_len);
}

static esp_err_t favicon_32x32_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)favicon_32x32_png, favicon_32x32_png_len);
}

static esp_err_t favicon_ico_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)favicon_ico, favicon_ico_len);
}

static esp_err_t logo_svg_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)logo_svg, logo_svg_len);
}

static esp_err_t dump_handler(httpd_req_t *req){
    flashLED(75);
    Serial.println("\nDump Requested");
    Serial.print("Preferences file: ");
    dumpPrefs(SPIFFS);
    static char dumpOut[2000] = "";
    char * d = dumpOut;
    // Header
    d+= sprintf(d,"<html><head><meta charset=\"utf-8\">\n");
    d+= sprintf(d,"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n");
    d+= sprintf(d,"<title>%s - Status</title>\n", myName);
    d+= sprintf(d,"<link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/favicon-32x32.png\">\n");
    d+= sprintf(d,"<link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/favicon-16x16.png\">\n");
    d+= sprintf(d,"<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n");
    d+= sprintf(d,"</head>\n");
    d+= sprintf(d,"<body>\n");
    d+= sprintf(d,"<img src=\"/logo.svg\" style=\"position: relative; float: right;\">\n"); 
    if (critERR.length() > 0) {
        d+= sprintf(d,"%s<hr>\n", critERR.c_str());
        Serial.printf("\n\nA critical error has occurred when initialising Hardware, see startup megssages\n\n\n");
    }
    d+= sprintf(d,"<h1>ESP32 Cam Webserver</h1>\n");
    // Module
    d+= sprintf(d,"Name: %s<br>\n", myName);
    Serial.printf("Name: %s\n", myName);
    d+= sprintf(d,"Firmware: %s (base: %s)<br>\n", myVer, baseVersion);
    Serial.printf("Firmware: %s (base: %s)\n", myVer, baseVersion);
    float sketchPct = 100 * sketchSize / sketchSpace;
    d+= sprintf(d,"Sketch Size: %i (total: %i, %.1f%% used)<br>\n", sketchSize, sketchSpace, sketchPct);
    Serial.printf("Sketch Size: %i (total: %i, %.1f%% used)\n", sketchSize, sketchSpace, sketchPct);
    d+= sprintf(d,"MD5: %s<br>\n", sketchMD5.c_str());
    Serial.printf("MD5: %s\n", sketchMD5.c_str());
    d+= sprintf(d,"ESP sdk: %s<br>\n", ESP.getSdkVersion());
    Serial.printf("ESP sdk: %s\n", ESP.getSdkVersion());
    // Network
    d+= sprintf(d,"<h2>WiFi</h2>\n");
    if (accesspoint) {
        if (captivePortal) {
            d+= sprintf(d,"Mode: AccessPoint with captive portal<br>\n");
            Serial.printf("Mode: AccessPoint with captive portal\n");
        } else {
            d+= sprintf(d,"Mode: AccessPoint<br>\n");
            Serial.printf("Mode: AccessPoint\n");
        }
        d+= sprintf(d,"SSID: %s<br>\n", apName);
        Serial.printf("SSID: %s\n", apName);
    } else {
        d+= sprintf(d,"Mode: Client<br>\n");
        Serial.printf("Mode: Client\n");
        String ssidName = WiFi.SSID();
        d+= sprintf(d,"SSID: %s<br>\n", ssidName.c_str());
        Serial.printf("Ssid: %s\n", ssidName.c_str());
        d+= sprintf(d,"Rssi: %i<br>\n", WiFi.RSSI());
        Serial.printf("Rssi: %i\n", WiFi.RSSI());
        String bssid = WiFi.BSSIDstr();
        d+= sprintf(d,"BSSID: %s<br>\n", bssid.c_str());
        Serial.printf("BSSID: %s\n", bssid.c_str());
    }
    d+= sprintf(d,"IP address: %d.%d.%d.%d<br>\n", ip[0], ip[1], ip[2], ip[3]);
    Serial.printf("IP address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    if (!accesspoint) {
        d+= sprintf(d,"Netmask: %d.%d.%d.%d<br>\n", net[0], net[1], net[2], net[3]);
        Serial.printf("Netmask: %d.%d.%d.%d\n", net[0], net[1], net[2], net[3]);
        d+= sprintf(d,"Gateway: %d.%d.%d.%d<br>\n", gw[0], gw[1], gw[2], gw[3]);
        Serial.printf("Gateway: %d.%d.%d.%d\n", gw[0], gw[1], gw[2], gw[3]);
    }
    d+= sprintf(d,"Http port: %i, Stream port: %i<br>\n", httpPort, streamPort);
    Serial.printf("Http port: %i, Stream port: %i\n", httpPort, streamPort);
    byte mac[6];
    WiFi.macAddress(mac);
    d+= sprintf(d,"MAC: %02X:%02X:%02X:%02X:%02X:%02X<br>\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // System
    d+= sprintf(d,"<h2>System</h2>\n");
    int64_t sec = esp_timer_get_time() / 1000000;
    int64_t upDays = int64_t(floor(sec/86400));
    int upHours = int64_t(floor(sec/3600)) % 24;
    int upMin = int64_t(floor(sec/60)) % 60;
    int upSec = sec % 60;
    d+= sprintf(d,"Up: %" PRId64 ":%02i:%02i:%02i (d:h:m:s)<br>\n", upDays, upHours, upMin, upSec);
    Serial.printf("Up: %" PRId64 ":%02i:%02i:%02i (d:h:m:s)\n", upDays, upHours, upMin, upSec);
    d+= sprintf(d,"Freq: %i MHz<br>\n", ESP.getCpuFreqMHz());
    Serial.printf("Freq: %i MHz\n", ESP.getCpuFreqMHz());
    d+= sprintf(d,"Heap: %i, free: %i, min free: %i, max block: %i<br>\n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    Serial.printf("Heap: %i, free: %i, min free: %i, max block: %i\n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    d+= sprintf(d,"Psram: %i, free: %i, min free: %i, max block: %i<br>\n", ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
    Serial.printf("Psram: %i, free: %i, min free: %i, max block: %i\n", ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
    if (filesystem) {
        d+= sprintf(d,"Spiffs: %i, used: %i<br>\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
        Serial.printf("Spiffs: %i, used: %i\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());
    }
    d+= sprintf(d,"Enrolled faces: %i (max %i)<br>\n", id_list.count, id_list.size);
    Serial.printf("Enrolled faces: %i (max %i)\n", id_list.count, id_list.size);

    // Footer
    d+= sprintf(d,"<br><div class=\"input-group\">\n");
    d+= sprintf(d,"<button title=\"Instant Refresh; the page reloads every minute anyway\" onclick=\"location.replace(document.URL)\">Refresh</button>\n");
    d+= sprintf(d,"<button title=\"Close this page\" onclick=\"javascript:window.close()\">Close</button>\n");
    d+= sprintf(d,"</div>\n</body>\n");
    // A javascript timer to refresh the page every minute.
    d+= sprintf(d,"<script>\nsetTimeout(function(){\nlocation.replace(document.URL);\n}, 60000);\n");
    d+= sprintf(d,"</script>\n</html>\n");
    *d++ = 0;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, dumpOut, strlen(dumpOut));
}

static esp_err_t style_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)style_css, style_css_len);
}

static esp_err_t streamviewer_handler(httpd_req_t *req){
    flashLED(75);
    Serial.println("Stream Viewer requested");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)streamviewer_html, streamviewer_html_len);
}

static esp_err_t error_handler(httpd_req_t *req){
    flashLED(75);
    Serial.println("Sending Error page");
    std::string s(error_html);
    size_t index;
    while ((index = s.find("<APPURL>")) != std::string::npos)
        s.replace(index, strlen("<APPURL>"), httpURL);
    while ((index = s.find("<CAMNAME>")) != std::string::npos)
        s.replace(index, strlen("<CAMNAME>"), myName);
    while ((index = s.find("<ERRORTEXT>")) != std::string::npos)
        s.replace(index, strlen("<ERRORTEXT>"), critERR.c_str());
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, (const char *)s.c_str(), s.length());
}

static esp_err_t index_handler(httpd_req_t *req){
    char*  buf;
    size_t buf_len;
    char view[32] = {0,};

    flashLED(75);
    // See if we have a specific target (full/simple/portal) and serve as appropriate
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "view", view, sizeof(view)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        // no target specified; default.
        strcpy(view,default_index);
        // If captive portal is active send that instead
        if (captivePortal) {
            strcpy(view,"portal");
        }
    }

    if  (strncmp(view,"simple", sizeof(view)) == 0) {
        Serial.println("Simple index page requested");
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Content-Encoding", "identity");
        return httpd_resp_send(req, (const char *)index_simple_html, index_simple_html_len);
    } else if(strncmp(view,"full", sizeof(view)) == 0) {
        Serial.println("Full index page requested");
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Content-Encoding", "identity");
        sensor_t * s = esp_camera_sensor_get();
        if (s->id.PID == OV3660_PID) {
            return httpd_resp_send(req, (const char *)index_ov3660_html, index_ov3660_html_len);
        }
        return httpd_resp_send(req, (const char *)index_ov2640_html, index_ov2640_html_len);
    } else if(strncmp(view,"portal", sizeof(view)) == 0) {
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
    } else  {
        Serial.print("Unknown page requested: ");
        Serial.println(view);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
}

void startCameraServer(int hPort, int sPort){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12; // we use more than the default 8 (on port 80)

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t style_uri = {
        .uri       = "/style.css",
        .method    = HTTP_GET,
        .handler   = style_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t favicon_16x16_uri = {
        .uri       = "/favicon-16x16.png",
        .method    = HTTP_GET,
        .handler   = favicon_16x16_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t favicon_32x32_uri = {
        .uri       = "/favicon-32x32.png",
        .method    = HTTP_GET,
        .handler   = favicon_32x32_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t favicon_ico_uri = {
        .uri       = "/favicon.ico",
        .method    = HTTP_GET,
        .handler   = favicon_ico_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t logo_svg_uri = {
        .uri       = "/logo.svg",
        .method    = HTTP_GET,
        .handler   = logo_svg_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t dump_uri = {
        .uri       = "/dump",
        .method    = HTTP_GET,
        .handler   = dump_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t stream_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t streamviewer_uri = {
        .uri       = "/view",
        .method    = HTTP_GET,
        .handler   = streamviewer_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t info_uri = {
        .uri       = "/info",
        .method    = HTTP_GET,
        .handler   = info_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t error_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = error_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t viewerror_uri = {
        .uri       = "/view",
        .method    = HTTP_GET,
        .handler   = error_handler,
        .user_ctx  = NULL
    };

    // Filter list; used during face detection
    ra_filter_init(&ra_filter, 20);

    // Mtmn config values (face detection and recognition parameters)
    mtmn_config.type = FAST;
    mtmn_config.min_face = 80;
    mtmn_config.pyramid = 0.707;
    mtmn_config.pyramid_times = 4;
    mtmn_config.p_threshold.score = 0.6;
    mtmn_config.p_threshold.nms = 0.7;
    mtmn_config.p_threshold.candidate_number = 20;
    mtmn_config.r_threshold.score = 0.7;
    mtmn_config.r_threshold.nms = 0.7;
    mtmn_config.r_threshold.candidate_number = 10;
    mtmn_config.o_threshold.score = 0.7;
    mtmn_config.o_threshold.nms = 0.7;
    mtmn_config.o_threshold.candidate_number = 1;

    // Face ID list (settings + pointer to the data allocation)
    face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
    // The size of the allocated data block; calculated in dl_lib_calloc()

    
    // Request Handlers; config.max_uri_handlers (above) must be >= the number of handlers
    config.server_port = hPort;
    config.ctrl_port = hPort;
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        if (critERR.length() > 0) {
            httpd_register_uri_handler(camera_httpd, &error_uri);
        } else {
            httpd_register_uri_handler(camera_httpd, &index_uri);
            httpd_register_uri_handler(camera_httpd, &cmd_uri);
            httpd_register_uri_handler(camera_httpd, &status_uri);
            httpd_register_uri_handler(camera_httpd, &capture_uri);
        }
        httpd_register_uri_handler(camera_httpd, &style_uri);
        httpd_register_uri_handler(camera_httpd, &favicon_16x16_uri);
        httpd_register_uri_handler(camera_httpd, &favicon_32x32_uri);
        httpd_register_uri_handler(camera_httpd, &favicon_ico_uri);
        httpd_register_uri_handler(camera_httpd, &logo_svg_uri);
        httpd_register_uri_handler(camera_httpd, &dump_uri);
    }

    config.server_port = sPort;
    config.ctrl_port = sPort;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        if (critERR.length() > 0) {
            httpd_register_uri_handler(camera_httpd, &error_uri);
            httpd_register_uri_handler(camera_httpd, &viewerror_uri);
        } else {
            httpd_register_uri_handler(stream_httpd, &stream_uri);
            httpd_register_uri_handler(stream_httpd, &info_uri);
            httpd_register_uri_handler(stream_httpd, &streamviewer_uri);
        }
        httpd_register_uri_handler(stream_httpd, &favicon_16x16_uri);
        httpd_register_uri_handler(stream_httpd, &favicon_32x32_uri);
        httpd_register_uri_handler(stream_httpd, &favicon_ico_uri);
    }
}
