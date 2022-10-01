#include "app_httpd.h"

CLAppHttpd::CLAppHttpd() {
    // Gather static values used when dumping status; these are slow functions, so just do them once during startup
    sketchSize = ESP.getSketchSize();;
    sketchSpace = ESP.getFreeSketchSpace();
    sketchMD5 = ESP.getSketchMD5();
    setTag("httpd");
}

void onSnapTimer(TimerHandle_t pxTimer){
    if(AppHttpd.getClientId() != 0) AppHttpd.snapToStream();
}

int CLAppHttpd::start() {
    
    loadPrefs();

    server = new AsyncWebServer(AppConn.getPort());
    ws = new AsyncWebSocket("/ws");
    
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if(AppConn.isAccessPoint())
            request->redirect("/setup");
        else
            request->redirect("/portal");
    });

    server->on("/setup", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(*fsStorage, "/www/setup.html", "", false, processor);
    });

    server->on("/portal", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(*fsStorage, "/www/portal.html", "", false, processor);
    });

    server->on("/view", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasArg("mode")) {
            if(request->arg("mode") == "simple") {
                request->send(*fsStorage, "/www/index_simple.html", "", false, processor);
            }
            else if(request->arg("mode") == "full") {
                if (AppCam.getSensorPID() == OV3660_PID) 
                    request->send(*fsStorage, "/www/index_ov3660.html", "", false, processor);
                request->send(*fsStorage, "/www/index_ov2640.html", "", false, processor);
            }
            else if(request->arg("mode") == "stream" || 
                    request->arg("mode") == "still") {
                if(AppCam.getErr().isEmpty()) {
                    AppHttpd.setStreamMode((request->arg("mode") == "stream"? CAPTURE_STREAM:CAPTURE_STILL));
                    request->send(*fsStorage, "/www/streamviewer.html", "", false, processor);
                }
                else
                    request->send(*fsStorage, "/www/error.html", "", false, processor);
            }
            else
                request->send(400);
        }
        else
            request->send(*fsStorage, "/www/index_simple.html", "", false, processor);
    });

    server->on("/dump", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(*fsStorage, "/www/dump.html");
    });
    
    server->on("/control", HTTP_GET, onControl);
    server->on("/status", HTTP_GET, onStatus);
    server->on("/system", HTTP_GET, onSystemStatus);
    server->on("/info", HTTP_GET, onInfo);

    
    // adding WebSocket handler
    ws->onEvent(onWsEvent);
    server->addHandler(ws);   

    snap_timer = xTimerCreate("SnapTimer", 1000/AppCam.getFrameRate()/portTICK_PERIOD_MS, pdTRUE, 0, onSnapTimer);
    
    server->serveStatic("/", *fsStorage, "/www");

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server->begin();

    if(isDebugMode()) {
        Serial.printf("\r\nUse '%s' to connect\r\n", AppConn.getHTTPUrl());
        Serial.printf("Stream viewer available at '%sview?mode=stream'\r\n", AppConn.getHTTPUrl());
        Serial.printf("Raw stream URL is '%s'\r\n", AppConn.getStreamUrl());
    }

    Serial.println("HTTP server started");
    return OK;
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    

    if(type == WS_EVT_CONNECT){
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    }
    else if(type == WS_EVT_DISCONNECT){
        Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
        AppHttpd.stopStream(client->id());        
    }
    else if(type == WS_EVT_ERROR){
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    }
    else if(type == WS_EVT_PONG){
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
    }
    else if(type == WS_EVT_DATA){
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        Serial.printf("ws[%s][%u] frame[%u] %u %s[%llu - %llu]: ", server->url(), client->id(), info->num,
         info->message_opcode, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

        char* msg = (char*) data;

        if(*msg == 's') {
            AppHttpd.startStream(client->id());
        }
        else if(*msg, 't') {
            AppHttpd.stopStream(client->id());
        }
        
    }

}    


String processor(const String& var) {
  if(var == "CAMNAME")
    return String(AppHttpd.getName());
  else if(var == "ERRORTEXT")
    return AppCam.getErr();
  else if(var == "APPURL")
    return String(AppConn.getHTTPUrl());
  else if(var == "STREAMURL")
    return String(AppConn.getStreamUrl());
  else
    return String();
}

esp_err_t CLAppHttpd::snapToStream(bool debug) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    fb = esp_camera_fb_get();

    if(fb) {
        size_t fb_len = 0;
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            ws->binary(AppHttpd.getClientId(), fb->buf, fb->len);
            if(debug) {
                Serial.print("JPG: "); Serial.print((uint32_t)(fb_len)); 
            }
        } else {
            // camera failed to aquire frame
            if(debug) 
                Serial.println("Capture Error: Non-JPEG image returned by camera module");
            res = ESP_FAIL;
        }
    }

    esp_camera_fb_return(fb);
    fb = NULL;
    return res;
}

esp_err_t CLAppHttpd::startStream(uint32_t id) {
    client_id = id;

    if(!snap_timer)
        return ESP_FAIL;

    if(xTimerIsTimerActive(snap_timer))
        xTimerStop(snap_timer, 100);

    if(streammode == CAPTURE_STREAM) {

        Serial.print("Stream start, frame period = "); Serial.println(xTimerGetPeriod(snap_timer));

        xTimerStart(snap_timer, 0);
        streamCount=1;
    }
    else if(streammode == CAPTURE_STILL) {
        Serial.println("Still image requested");
        if(AppCam.isAutoLamp()){
            AppCam.setLamp();
            delay(75); // coupled with the status led flash this gives ~150ms for lamp to settle.
        }
        int64_t fr_start = esp_timer_get_time();
    
        if (snapToStream(isDebugMode()) != ESP_OK) {
            if(AppCam.isAutoLamp()) AppCam.setLamp(0);
            return ESP_FAIL;
        }
        
        if (isDebugMode()) {
            int64_t fr_end = esp_timer_get_time();
            Serial.printf("B %ums\r\n", (uint32_t)((fr_end - fr_start)/1000));
        }

        if(AppCam.isAutoLamp()) AppCam.setLamp(0);

        imagesServed++;
    }
    else
        return ESP_FAIL;
    return ESP_OK;
}

esp_err_t CLAppHttpd::stopStream(uint32_t id) {
    client_id = 0;
    if(!snap_timer)
        return ESP_FAIL;
    
    if(xTimerIsTimerActive(snap_timer))
        xTimerStop(snap_timer, 100);   

    if(streammode == CAPTURE_STREAM) {
        Serial.println("Stream stopped");  
        streamsServed++;
        streamCount=0;
    }
    return ESP_OK;
}

void onControl(AsyncWebServerRequest *request) {
    
    if (AppCam.getErr().length() > 0) {
        request->send(500);
        return;
    }

    if (request->args() == 0) {
        request->send(400);
        return;
    }

    String variable = request->arg("var");
    String value = request->arg("val");

    if(variable == "cmdout") {
        if(AppHttpd.isDebugMode()) {
            Serial.print("cmmdout=");
            Serial.println(value.c_str());
        }
        AppHttpd.serialSendCommand(value.c_str());
        request->send(200, "", "OK");
        return;
    }

    int val = value.toInt();
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;
    if(variable == "framesize") {
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(variable == "quality") res = s->set_quality(s, val);
    else if(variable == "xclk") { AppCam.setXclk(val); res = s->set_xclk(s, LEDC_TIMER_0, AppCam.getXclk()); }
    else if(variable == "contrast") res = s->set_contrast(s, val);
    else if(variable == "brightness") res = s->set_brightness(s, val);
    else if(variable ==  "saturation") res = s->set_saturation(s, val);
    else if(variable ==  "gainceiling") res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(variable ==  "colorbar") res = s->set_colorbar(s, val);
    else if(variable ==  "awb") res = s->set_whitebal(s, val);
    else if(variable ==  "agc") res = s->set_gain_ctrl(s, val);
    else if(variable ==  "aec") res = s->set_exposure_ctrl(s, val);
    else if(variable ==  "hmirror") res = s->set_hmirror(s, val);
    else if(variable ==  "vflip") res = s->set_vflip(s, val);
    else if(variable ==  "awb_gain") res = s->set_awb_gain(s, val);
    else if(variable ==  "agc_gain") res = s->set_agc_gain(s, val);
    else if(variable ==  "aec_value") res = s->set_aec_value(s, val);
    else if(variable ==  "aec2") res = s->set_aec2(s, val);
    else if(variable ==  "dcw") res = s->set_dcw(s, val);
    else if(variable ==  "bpc") res = s->set_bpc(s, val);
    else if(variable ==  "wpc") res = s->set_wpc(s, val);
    else if(variable ==  "raw_gma") res = s->set_raw_gma(s, val);
    else if(variable ==  "lenc") res = s->set_lenc(s, val);
    else if(variable ==  "special_effect") res = s->set_special_effect(s, val);
    else if(variable ==  "wb_mode") res = s->set_wb_mode(s, val);
    else if(variable ==  "ae_level") res = s->set_ae_level(s, val);
    else if(variable ==  "rotate") AppCam.setRotation(val);
    else if(variable ==  "frame_rate") {
        AppCam.setFrameRate(val);
        AppHttpd.updateSnapTimer(val);
    }
    else if(variable ==  "autolamp" && AppCam.getLamp() != -1) {
        AppCam.setAutoLamp(val);
    }
    else if(variable ==  "lamp" && AppCam.getLamp() != -1) {
        AppCam.setLamp(constrain(val,0,100));
    }
    else if(variable ==  "save_prefs") {
        AppCam.savePrefs();
    }
    else if(variable == "clear_prefs") {
        AppCam.removePrefs();
    }
    else if(variable == "reboot") {
        if (AppCam.getLamp() != -1) AppCam.setLamp(0); // kill the lamp; otherwise it can remain on during the soft-reboot
        esp_task_wdt_init(3,true);  // schedule a a watchdog panic event for 3 seconds in the future
        esp_task_wdt_add(NULL);
        periph_module_disable(PERIPH_I2C0_MODULE); // try to shut I2C down properly
        periph_module_disable(PERIPH_I2C1_MODULE);
        periph_module_reset(PERIPH_I2C0_MODULE);
        periph_module_reset(PERIPH_I2C1_MODULE);
        Serial.print("REBOOT requested");
        while(true) {
          delay(200);
          Serial.print('.');
        }
    }
    else {
        res = -1;
    }
    if(res){
        request->send(400, "", "Unknown command");
        return;
    }
    request->send(200, "", "OK");
}

void CLAppHttpd::updateSnapTimer(int tps) {
    if(snap_timer)
        xTimerChangePeriod(snap_timer, 1000/tps/portTICK_PERIOD_MS, 100);
}

void onInfo(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{");
    response->printf("\"cam_name\":\"%s\",", AppHttpd.getName());
    response->printf("\"rotate\":\"%d\",", AppCam.getRotation());
    response->printf("\"stream_url\":\"%s\"", AppConn.getStreamUrl());
    response->print("}");
    request->send(response);    
}

void onStatus(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // Do not get attempt to get sensor when in error; causes a panic..
    response->print("{");
    if (AppCam.getErr().length() == 0) {
        sensor_t * s = esp_camera_sensor_get();
        response->printf("\"lamp\":%d,", AppCam.getLamp());
        response->printf("\"autolamp\":%d,", AppCam.isAutoLamp());
        response->printf("\"frame_rate\":%d,", AppCam.getFrameRate());
        response->printf("\"framesize\":%u,", s->status.framesize);
        response->printf("\"quality\":%u,", s->status.quality);
        response->printf("\"xclk\":%u,", AppCam.getXclk());
        response->printf("\"brightness\":%d,", s->status.brightness);
        response->printf("\"contrast\":%d,", s->status.contrast);
        response->printf("\"saturation\":%d,", s->status.saturation);
        response->printf("\"sharpness\":%d,", s->status.sharpness);
        response->printf("\"special_effect\":%u,", s->status.special_effect);
        response->printf("\"wb_mode\":%u,", s->status.wb_mode);
        response->printf("\"awb\":%u,", s->status.awb);
        response->printf("\"awb_gain\":%u,", s->status.awb_gain);
        response->printf("\"aec\":%u,", s->status.aec);
        response->printf("\"aec2\":%u,", s->status.aec2);
        response->printf("\"ae_level\":%d,", s->status.ae_level);
        response->printf("\"aec_value\":%u,", s->status.aec_value);
        response->printf("\"agc\":%u,", s->status.agc);
        response->printf("\"agc_gain\":%u,", s->status.agc_gain);
        response->printf("\"gainceiling\":%u,", s->status.gainceiling);
        response->printf("\"bpc\":%u,", s->status.bpc);
        response->printf("\"wpc\":%u,", s->status.wpc);
        response->printf("\"raw_gma\":%u,", s->status.raw_gma);
        response->printf("\"lenc\":%u,", s->status.lenc);
        response->printf("\"vflip\":%u,", s->status.vflip);
        response->printf("\"hmirror\":%u,", s->status.hmirror);
        response->printf("\"dcw\":%u,", s->status.dcw);
        response->printf("\"colorbar\":%u,", s->status.colorbar);
        response->printf("\"cam_name\":\"%s\",", AppHttpd.getName());
        response->printf("\"code_ver\":\"%s\",", AppHttpd.getVersion().c_str());
        response->printf("\"rotate\":\"%d\",", AppCam.getRotation());
        response->printf("\"stream_url\":\"%s\"", AppConn.getStreamUrl());
    }
    response->print("}");
    request->send(response);
}

void onSystemStatus(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    char buf[1024];
    char * buf_ptr = buf;
    dumpSystemStatusToJson(buf_ptr, sizeof(buf));
    
    response->print(buf);

    if(AppHttpd.isDebugMode()) {
        Serial.println();
        Serial.println("Dump requested through web");
        Serial.println(buf);
    }

    request->send(response);
}

void dumpSystemStatusToJson(char * buf, size_t size) {

    buf += sprintf(buf, "{");
    buf += sprintf(buf,"\"cam_name\":\"%s\",", AppHttpd.getName());
    buf += sprintf(buf,"\"code_ver\":\"%s\",", AppHttpd.getVersion().c_str());
    buf += sprintf(buf,"\"base_version\":\"%s\",", BASE_VERSION);
    buf += sprintf(buf,"\"sketch_size\":%u,", AppHttpd.getSketchSize());
    buf += sprintf(buf,"\"sketch_space\":%u,", AppHttpd.getSketchSpace());
    buf += sprintf(buf,"\"sketch_md5\":\"%s\",", AppHttpd.getSketchMD5().c_str());
    buf += sprintf(buf,"\"esp_sdk\":\"%s\",", ESP.getSdkVersion());
    buf += sprintf(buf,"\"accesspoint\":%s,", (AppConn.isAccessPoint()?"true":"false"));
    buf += sprintf(buf,"\"captiveportal\":%s,", (AppConn.isCaptivePortal()?"true":"false"));
    buf += sprintf(buf,"\"ap_name\":\"%s\",", AppConn.getApName());
    String ssidName = WiFi.SSID();
    buf += sprintf(buf,"\"ssid\":\"%s\",", ssidName.c_str());
    buf += sprintf(buf,"\"rssi\":%i,", WiFi.RSSI());
    String bssid = WiFi.BSSIDstr();
    buf += sprintf(buf,"\"bssid\":\"%s\",", bssid.c_str());
    buf += sprintf(buf,"\"ip_address\":\"%s\",", (AppConn.isAccessPoint()?WiFi.softAPIP().toString():WiFi.localIP().toString()));
    buf += sprintf(buf,"\"subnet\":\"%s\",", (!AppConn.isAccessPoint()?WiFi.subnetMask().toString():""));
    buf += sprintf(buf,"\"gateway\":\"%s\",", (!AppConn.isAccessPoint()?WiFi.gatewayIP().toString():""));
    buf += sprintf(buf,"\"port\":%i,", AppConn.getPort());
    byte mac[6];
    WiFi.macAddress(mac);
    buf += sprintf(buf,"\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\",", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    buf += sprintf(buf,"\"local_time\":\"%s\",", AppConn.getLocalTimeStr());
    buf += sprintf(buf,"\"up_time\":\"%s\",", AppConn.getUpTimeStr());
    buf += sprintf(buf,"\"ntp_server\":\"%s\",", AppConn.getNTPServer());
    buf += sprintf(buf,"\"gmt_offset\":%li,", AppConn.getGmtOffset_sec());
    buf += sprintf(buf,"\"dst_offset\":%i,", AppConn.getDaylightOffset_sec());
    buf += sprintf(buf,"\"active_streams\":%i,", AppHttpd.getStreamCount());
    buf += sprintf(buf,"\"prev_streams\":%lu,", AppHttpd.getStreamsServed());
    buf += sprintf(buf,"\"img_captured\":%lu,", AppHttpd.getImagesServed());

    buf += sprintf(buf,"\"cpu_freq\":%i,", ESP.getCpuFreqMHz());
    buf += sprintf(buf,"\"xclk\":%i,", AppCam.getXclk());
    buf += sprintf(buf,"\"num_cores\":%i,", ESP.getChipCores());

    int McuTf = temprature_sens_read(); // fahrenheit
    buf += sprintf(buf,"\"esp_temp\":%i,", McuTf);
    buf += sprintf(buf,"\"heap_avail\":%i,", ESP.getHeapSize());
    buf += sprintf(buf,"\"heap_free\":%i,", ESP.getFreeHeap());
    buf += sprintf(buf,"\"heap_min_free\":%i,", ESP.getMinFreeHeap());
    buf += sprintf(buf,"\"heap_max_bloc\":%i,", ESP.getMaxAllocHeap());

    buf += sprintf(buf,"\"psram_found\":%s,", (psramFound()?"true":"false"));
    buf += sprintf(buf,"\"psram_size\":%i,", (psramFound()?ESP.getPsramSize():0));
    buf += sprintf(buf,"\"psram_free\":%i,", (psramFound()?ESP.getFreePsram():0));
    buf += sprintf(buf,"\"psram_min_free\":%i,", (psramFound()?ESP.getMinFreePsram():0));
    buf += sprintf(buf,"\"psram_max_bloc\":%i,", (psramFound()?ESP.getMaxAllocPsram():0));

    buf += sprintf(buf,"\"storage_size\":%i,", storageSize());
    buf += sprintf(buf,"\"storage_used\":%i,", storageUsed());
    buf += sprintf(buf,"\"storage_units\":\"%s\",", (capacityUnits()==STORAGE_UNITS_MB?"MB":""));
    buf += sprintf(buf,"\"serial_buf\":\"%s\"", AppHttpd.getSerialBuffer());
    buf += sprintf(buf, "}");
}

void CLAppHttpd::serialSendCommand(const char *cmd) {
    Serial.print("^");
    Serial.println(cmd);
}

int CLAppHttpd::loadPrefs() {
    jparse_ctx_t jctx;
    int ret  = parsePrefs(&jctx);
    if(ret != OS_SUCCESS) {
        return ret;
    }
    
    json_obj_get_string(&jctx, "my_name", myName, sizeof(myName));
    bool dbg;
    if(json_obj_get_bool(&jctx, "debug_mode", &dbg) == OS_SUCCESS)
        setDebugMode(dbg);  

    return ret;
}

CLAppHttpd AppHttpd;
