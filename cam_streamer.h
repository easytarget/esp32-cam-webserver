#ifndef _INC_CAM_STREAMER
#define _INC_CAM_STREAMER

#include <esp_timer.h>
#include <esp_http_server.h>
#include <esp_camera.h>
#include <string.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define CAM_STREAMER_MAX_CLIENTS 10
typedef struct {
    QueueHandle_t clients;
    TaskHandle_t task;
    uint64_t last_updated;
    int64_t frame_delay;
    uint8_t buf_lock;
    camera_fb_t *buf;
    char part_buf[64];
    size_t part_len;
    httpd_handle_t server;
    size_t num_clients;
} cam_streamer_t;

void cam_streamer_init(cam_streamer_t *s, httpd_handle_t server, uint16_t fps);
void cam_streamer_task(void *p);
void cam_streamer_start(cam_streamer_t *s);
void cam_streamer_stop(cam_streamer_t *s);
bool cam_streamer_enqueue_client(cam_streamer_t *s, int fd);
size_t cam_streamer_get_num_clients(cam_streamer_t *s);
void cam_streamer_dequeue_all_clients(cam_streamer_t *s);

#endif
