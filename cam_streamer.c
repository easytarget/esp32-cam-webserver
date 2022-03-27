#include <esp_timer.h>
#include <esp_http_server.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <sys/socket.h>

#include "cam_streamer.h"

#ifndef CAM_STREAMER_MAX_CLIENTS
#define CAM_STREAMER_MAX_CLIENTS 10
#endif

#define PART_BOUNDARY "123456789000000000000987654321"

#define _STREAM_HEADERS "HTTP/1.1 200 OK\r\n"\
				"Access-Control-Allow-Origin: *\r\n"\
				"Connection: Keep-Alive\r\n"\
				"Keep-Alive: timeout=15\r\n"\
				"Content-Type: multipart/x-mixed-replace;boundary=" PART_BOUNDARY "\r\n"

#define _TEXT_HEADERS "HTTP/1.1 200 OK\r\n"\
				"Access-Control-Allow-Origin: *\r\n"\
				"Connection: Close\r\n"\
				"Content-Type: text/plain\r\n\r\n"

extern bool debugData;

static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static inline void print_debug(const char *fmt, ...) {
    if(debugData) {
        va_list l;
        va_start(l, fmt);
        vprintf(fmt, l);
        va_end(l);
    }
}

static uint8_t is_send_error(int r) {
    switch(r) {
    case HTTPD_SOCK_ERR_INVALID:
        print_debug("[cam_streamer] invalid argument occured!\n");
        return 1;
    case HTTPD_SOCK_ERR_TIMEOUT:
        print_debug("[cam_streamer] timeout/interrupt occured!\n");
        return 1;
    case HTTPD_SOCK_ERR_FAIL:
        print_debug("[cam_streamer] unrecoverable error while send()!\n");
        return 1;
    case ESP_ERR_INVALID_ARG:
        print_debug("[text-streamer] session closed!\n");
        return 1;
    default:
        print_debug("[cam_streamer] sent %d bytes!\n", r);
        return 0;
    }
}

void cam_streamer_init(cam_streamer_t *s, httpd_handle_t server, uint16_t frame_delay) {
    memset(s, 0, sizeof(cam_streamer_t));
    s->frame_delay=1000*frame_delay;
    s->clients=xQueueCreate(CAM_STREAMER_MAX_CLIENTS*2, sizeof(int));
    s->server=server;
}

// frame_delay must be in ms (not us)
void cam_streamer_set_frame_delays(cam_streamer_t *s, uint16_t frame_delay) {
    s->frame_delay=1000*frame_delay;
}

static void cam_streamer_update_frame(cam_streamer_t *s) {
    uint8_t l=0;
    while(!__atomic_compare_exchange_n(&s->buf_lock, &l, 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
        l=0;
        vTaskDelay(1/portTICK_PERIOD_MS);
    }

    if(s->buf)
        esp_camera_fb_return(s->buf);

    s->buf=esp_camera_fb_get();

    s->last_updated=esp_timer_get_time();
    s->part_len=snprintf(s->part_buf, 64, _STREAM_PART, s->buf->len);
    __atomic_store_n(&s->buf_lock, 0, __ATOMIC_RELAXED);
    print_debug("[cam_streamer] fetched new frame\n");
}

static void cam_streamer_decrement_num_clients(cam_streamer_t *s) {
    size_t num_clients=s->num_clients;
    while(num_clients>0 && !__atomic_compare_exchange_n(&s->num_clients, &num_clients, num_clients-1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    print_debug("[cam_streamer] num_clients decremented\n");
}

void cam_streamer_task(void *p) {
    cam_streamer_t *s=(cam_streamer_t *) p;

    uint64_t last_time=0, current_time;
    int fd;
    unsigned int n_entries;
    for(;;) {
        while(!(n_entries=uxQueueMessagesWaiting(s->clients)))
            vTaskSuspend(NULL);

        current_time=esp_timer_get_time();
        if((current_time-last_time)<s->frame_delay)
            vTaskDelay((s->frame_delay-(current_time-last_time))/(1000*portTICK_PERIOD_MS));
        last_time=current_time;

        cam_streamer_update_frame(s);

        for(unsigned int i=0; i<n_entries; ++i) {
            if(xQueueReceive(s->clients, &fd, 10/portTICK_PERIOD_MS)==pdFALSE) {
                print_debug("[cam_streamer] failed to dequeue fd!\n");
                continue;
            }

            print_debug("[cam_streamer] dequeued fd %d\n", fd);
            print_debug("[cam_streamer] sending part: \"%.*s\"\n", (int) s->part_len, s->part_buf);

            if(is_send_error(httpd_socket_send(s->server, fd, s->part_buf, s->part_len, 0))) {
                cam_streamer_decrement_num_clients(s);
                continue;
            }

            if(is_send_error(httpd_socket_send(s->server, fd, s->buf->buf, s->buf->len, 0))) {
                cam_streamer_decrement_num_clients(s);
                continue;
            }

            if(is_send_error(httpd_socket_send(s->server, fd, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY), 0))) {
                cam_streamer_decrement_num_clients(s);
                continue;
            }

            xQueueSend(s->clients, (void *) &fd, 10/portTICK_PERIOD_MS);
            print_debug("[cam_streamer] fd %d requeued\n", fd);
        }
    }
}

void cam_streamer_start(cam_streamer_t *s) {
    BaseType_t r=xTaskCreate(cam_streamer_task, "cam_streamer", 10*1024, (void *) s, tskIDLE_PRIORITY+3, &s->task);

    if(r!=pdPASS)
        print_debug("[cam_streamer] failed to create task!\n");
}

void cam_streamer_stop(cam_streamer_t *s) {
    vTaskDelete(s->task);
}

size_t cam_streamer_get_num_clients(cam_streamer_t *s) {
    return s->num_clients;
}

void cam_streamer_dequeue_all_clients(cam_streamer_t *s) {
    xQueueReset(s->clients);
    __atomic_exchange_n(&s->num_clients, 0, __ATOMIC_RELAXED);
}

bool cam_streamer_enqueue_client(cam_streamer_t *s, int fd) {
    if(s->num_clients>=CAM_STREAMER_MAX_CLIENTS) {
        if(httpd_socket_send(s->server, fd, _TEXT_HEADERS, strlen(_TEXT_HEADERS), 0)) {
            print_debug("failed sending text headers!\n");
            return false;
        }

#define EMSG "too many clients"
        if(httpd_socket_send(s->server, fd, EMSG, strlen(EMSG), 0)) {
            print_debug("failed sending message\n");
            return false;
        }
#undef EMSG
        close(fd);
        return false;
    }

    print_debug("sending stream headers:\n%s\nLength: %d\n", _STREAM_HEADERS, strlen(_STREAM_HEADERS));
    if(is_send_error(httpd_socket_send(s->server, fd, _STREAM_HEADERS, strlen(_STREAM_HEADERS), 0))) {
        print_debug("failed sending headers!\n");
        return false;
    }

    if(is_send_error(httpd_socket_send(s->server, fd, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY), 0))) {
        print_debug("failed sending boundary!\n");
        return false;
    }

    const BaseType_t r=xQueueSend(s->clients, (void *) &fd, 10*portTICK_PERIOD_MS);
    if(r!=pdTRUE) {
        print_debug("[cam_streamer] failed to enqueue fd %d\n", fd);
#define EMSG "failed to enqueue"
        httpd_socket_send(s->server, fd, EMSG, strlen(EMSG), 0);
#undef EMSG
    } else {
        print_debug("[cam_streamer] socket %d enqueued\n", fd);
        __atomic_fetch_add(&s->num_clients, 1, __ATOMIC_RELAXED);
        vTaskResume(s->task);
    }

    return r==pdTRUE?true:false;
}

