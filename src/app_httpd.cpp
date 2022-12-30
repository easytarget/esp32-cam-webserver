#include "app_httpd.h"

CLAppHttpd::CLAppHttpd() {
    // Gather static values used when dumping status; these are slow functions, so just do them once during startup
    sketchSize = ESP.getSketchSize();;
    sketchSpace = ESP.getFreeSketchSpace();
    sketchMD5 = ESP.getSketchMD5();
    setTag("httpd");
}

void onSnapTimer(TimerHandle_t pxTimer){
    AppHttpd.snapToStream();
}

int CLAppHttpd::start() {
    
    loadPrefs();

    server = new AsyncWebServer(AppConn.getPort());
    ws = new AsyncWebSocket("/ws");
    
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if(AppConn.isConfigured())
            request->send(Storage.getFS(), "/www/index.html", "", false, processor);
        else
            request->redirect("/setup");
    });

    server->on("/setup", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(Storage.getFS(), "/www/setup.html", "", false, processor);
    });    

    server->on("/view", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->arg("mode") == "stream" || 
            request->arg("mode") == "still") {
            if(!AppCam.getLastErr()) {
                AppHttpd.setStreamMode((request->arg("mode") == "stream"? CAPTURE_STREAM:CAPTURE_STILL));
                request->send(Storage.getFS(), "/www/view.html", "", false, processor);
            }
            else
                request->send(Storage.getFS(), "/www/error.html", "", false, processor);
        }
        else
            request->send(400);
    });

    // adding fixed mappigs
    for(int i=0; i<mappingCount; i++) {
        server->serveStatic(mappingList[i]->uri, Storage.getFS(), mappingList[i]->path);
    }
    
    server->on("/control", HTTP_GET, onControl);
    server->on("/status", HTTP_GET, onStatus);
    server->on("/system", HTTP_GET, onSystemStatus);
    server->on("/info", HTTP_GET, onInfo);

    
    // adding WebSocket handler
    ws->onEvent(onWsEvent);
    server->addHandler(ws);   

    snap_timer = xTimerCreate("SnapTimer", 1000/AppCam.getFrameRate()/portTICK_PERIOD_MS, pdTRUE, 0, onSnapTimer);

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
        if(AppHttpd.getControlClient() == client->id()) {
            AppHttpd.setControlClient(0);
            AppHttpd.resetPWM(RESET_ALL_PWM);
            AppHttpd.serialSendCommand("Disconnected");
        }
    }
    else if(type == WS_EVT_ERROR){
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    }
    else if(type == WS_EVT_PONG){
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
    }
    else if(type == WS_EVT_DATA){
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        uint8_t* msg = (uint8_t*) data;

        switch(*msg) {
            case (uint8_t)'u':
                AppHttpd.startStream(client->id());
                break;
            case (uint8_t)'s':
                AppHttpd.setStreamMode(CAPTURE_STREAM);
                AppHttpd.startStream(client->id());
                break;
            case (uint8_t)'p':  
                AppHttpd.setStreamMode(CAPTURE_STILL);
                AppHttpd.startStream(client->id());
                break;
            case (uint8_t)'c':
                AppHttpd.setControlClient(client->id());
                AppHttpd.serialSendCommand("Connected");
                break;
            case (uint8_t)'w':  // write PWM value
                if(AppHttpd.getControlClient())
                    if(len > 4) {
                        uint8_t pin = *(msg+1);
                        int nparams = *(msg+2);
                        int vlen = *(msg+3);
                        int value = 0;

                        if(vlen == 2)
                            value = *(msg+4) + *(msg+5)*256;
                        else
                            value = *(msg+4);

                        if(AppHttpd.isDebugMode())
                            Serial.printf("vlen %d nparams %d value %d\r\n", vlen, nparams, value);

                        if(nparams == 1)
                            AppHttpd.writePWM(pin, value); // write to servo
                        else
                            AppHttpd.writePWM(pin, value, 0); // write to raw PWM
                    }
                break;
            case (uint8_t)'t':  // terminate stream
                AppHttpd.stopStream(client->id());
                break;
            default:
                Serial.printf("ws[%s] client[%u] frame[%u] %u %s[%llu - %llu]: ", server->url(), client->id(), info->num,
                    info->message_opcode, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
                for(int i=0; i< len; i++) {
                    Serial.printf("%d,", *msg);
                    msg++;
                }
                Serial.println();
                break;
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

int CLAppHttpd::snapToStream(bool debug) {
    
    if(!stream_client) return ESP_FAIL;

    int res = AppCam.snapToBuffer();

    if(!res) {

        if(AppCam.isJPEGinBuffer()){
            ws->binary(stream_client, AppCam.getBuffer(), AppCam.getBufferSize());
            if(debug) {
                Serial.print("JPG: "); Serial.print(AppCam.getBufferSize()); 
            }
        } else {
            // camera failed to acquire frame
            if(debug) 
                Serial.println("Capture Error: Non-JPEG image returned by camera module");
            res = OS_FAIL;
        }
    }

    AppCam.releaseBuffer();
    return res;
}

int CLAppHttpd::startStream(uint32_t id) {
    
    // if stream already started for a client id, return fail
    if(stream_client)
        return OS_FAIL; 
    
    stream_client = id;

    if(!snap_timer)
        return OS_FAIL;

    if(xTimerIsTimerActive(snap_timer))
        xTimerStop(snap_timer, 100);

    if(streammode == CAPTURE_STREAM) {
        if(autoLamp){
            setLamp();
            delay(75); // coupled with the status led flash this gives ~150ms for lamp to settle.
        }

        Serial.print("Stream start, frame period = "); Serial.println(xTimerGetPeriod(snap_timer));

        xTimerStart(snap_timer, 0);
        streamCount=1;

    }
    else if(streammode == CAPTURE_STILL) {
        Serial.println("Still image requested");
        if(autoLamp){
            setLamp();
            delay(75); // coupled with the status led flash this gives ~150ms for lamp to settle.
        }
        int64_t fr_start = esp_timer_get_time();
    
        if (snapToStream(isDebugMode()) != OS_SUCCESS) {
            if(autoLamp) setLamp(0);
            return OS_FAIL;
        }
        
        if (isDebugMode()) {
            int64_t fr_end = esp_timer_get_time();
            Serial.printf("B %ums\r\n", (uint32_t)((fr_end - fr_start)/1000));
        }

        if(autoLamp) setLamp(0);

        imagesServed++;
    }
    else
        return OS_FAIL;
    return OS_SUCCESS;
}

int CLAppHttpd::stopStream(uint32_t id) {

    if(autoLamp) setLamp(0);

    if(!snap_timer)
        return OS_FAIL;

    if(stream_client != id)
        return OS_FAIL;

    stream_client = 0;    
    
    if(xTimerIsTimerActive(snap_timer))
        xTimerStop(snap_timer, 100);   

    if(streammode == CAPTURE_STREAM) {
        Serial.println("Stream stopped");  
        streamsServed++;
        streamCount=0;
    }
    return OS_SUCCESS;
}

void onControl(AsyncWebServerRequest *request) {
    
    if (AppCam.getLastErr()) {
        request->send(500);
        return;
    }

    if (request->args() == 0) {
        request->send(400);
        return;
    }

    String variable = request->arg("var");
    String value = request->arg("val");
    int res = 0;
    long val = value.toInt();
    sensor_t * s = AppCam.getSensor();

    if(variable == "cmdout") {
        if(AppHttpd.isDebugMode()) {
            Serial.print("cmmdout=");
            Serial.println(value.c_str());
        }
        AppHttpd.serialSendCommand(value.c_str());
        request->send(200);
        return;
    }    
    else if(variable ==  "save_prefs") {
        if(value == "conn") 
            res = AppConn.savePrefs();
        else if(value == "cam") 
            res = AppCam.savePrefs() + AppHttpd.savePrefs(); 
        else {
            request->send(400);
            return;
        }
        if(res == OS_SUCCESS)
            request->send(200);
        else
            request->send(500);
        return;
    }
    else if(variable ==  "remove_prefs") {
        if(value == "conn")
            res = AppConn.removePrefs(); 
        else if(value == "cam")
            res = AppCam.removePrefs();
        else {
            request->send(400);
            return;
        }

        if(res == OS_SUCCESS)
            request->send(200);
        else
            request->send(500);
        return;
    }
    else if(variable == "ssid") {AppConn.setSSID(value.c_str());AppConn.setPassword("");}
    else if(variable == "password") AppConn.setPassword(value.c_str());
    else if(variable == "st_ip") AppConn.setStaticIP(&(AppConn.getStaticIP()->ip), value.c_str());
    else if(variable == "st_subnet") AppConn.setStaticIP(&(AppConn.getStaticIP()->netmask), value.c_str());
    else if(variable == "st_gateway") AppConn.setStaticIP(&(AppConn.getStaticIP()->gateway), value.c_str());
    else if(variable == "dns1") AppConn.setStaticIP(&(AppConn.getStaticIP()->dns1), value.c_str());
    else if(variable == "dns2") AppConn.setStaticIP(&(AppConn.getStaticIP()->dns2), value.c_str());
    else if(variable == "ap_ip") AppConn.setStaticIP(&(AppConn.getAPIP()->ip), value.c_str());
    else if(variable == "ap_subnet") AppConn.setStaticIP(&(AppConn.getAPIP()->netmask), value.c_str());
    else if(variable == "ap_name") AppConn.setApName(value.c_str());
    else if(variable == "ap_pass") AppConn.setApPass(value.c_str());
    else if(variable == "mdns_name") AppConn.setMDNSName(value.c_str());
    else if(variable == "ntp_server") AppConn.setNTPServer(value.c_str());
    else if(variable == "ota_password") AppConn.setOTAPassword(value.c_str());
    else if(variable == "framesize") {
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(variable == "quality") res = s->set_quality(s, val);
    else if(variable == "xclk") { AppCam.setXclk(val); res = s->set_xclk(s, LEDC_TIMER_0, AppCam.getXclk()); }
    else if(variable == "contrast") res = s->set_contrast(s, val);
    else if(variable == "brightness") res = s->set_brightness(s, val);
    else if(variable ==  "saturation") res = s->set_saturation(s, val);
    else if(variable ==  "sharpness") res = s->set_sharpness(s, val);
    else if(variable ==  "denoise") res = s->set_denoise(s, val);
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
    else if(variable ==  "autolamp" && AppHttpd.getLamp() != -1) {
        AppHttpd.setAutoLamp(val);
    }
    else if(variable ==  "lamp" && AppHttpd.getLamp() != -1) {
        AppHttpd.setLamp(constrain(val,0,100));
    }
    else if(variable ==  "flashlamp" && AppHttpd.getLamp() != -1) {
        AppHttpd.setFlashLamp(constrain(val,0,100));
    }
    else if(variable == "accesspoint") AppConn.setAccessPoint(val);
    else if(variable == "ap_channel") AppConn.setAPChannel(val);
    else if(variable == "ap_dhcp") AppConn.setAPDHCP(val);
    else if(variable == "dhcp") AppConn.setDHCPEnabled(val);
    else if(variable == "port") AppConn.setPort(val);
    else if(variable == "ota_enabled") AppConn.setOTAEnabled(val);
    else if(variable == "gmt_offset") AppConn.setGmtOffset_sec(val);
    else if(variable == "dst_offset") AppConn.setDaylightOffset_sec(val);
    else if(variable == "reboot") {
        if (AppHttpd.getLamp() != -1) AppHttpd.setLamp(0); // kill the lamp; otherwise it can remain on during the soft-reboot
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
        request->send(400);
        return;
    }
    request->send(200);
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
    if (!AppCam.getLastErr()) {
        sensor_t * s = AppCam.getSensor();
        response->printf("\"lamp\":%d,", AppHttpd.getLamp());
        response->printf("\"autolamp\":%d,", AppHttpd.isAutoLamp());
        response->printf("\"flashlamp\":%d,", AppHttpd.getFlashLamp());
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
        response->printf("\"cam_pid\":%u,", s->id.PID);
        response->printf("\"cam_ver\":%u,", s->id.VER);
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

    char buf[1280];
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
    buf += sprintf(buf,"\"ssid\":\"%s\",", AppConn.getSSID());

    buf += sprintf(buf,"\"rssi\":%i,", (!AppConn.isAccessPoint()?WiFi.RSSI():0));
    buf += sprintf(buf,"\"bssid\":\"%s\",", (!AppConn.isAccessPoint()?WiFi.BSSIDstr().c_str():""));
    buf += sprintf(buf,"\"dhcp\":%i,", (AppConn.isDHCPEnabled()? 1:0 ));
    buf += sprintf(buf,"\"ip_address\":\"%s\",", (AppConn.isAccessPoint()?WiFi.softAPIP().toString():WiFi.localIP().toString()));
    buf += sprintf(buf,"\"subnet\":\"%s\",", (!AppConn.isAccessPoint()?WiFi.subnetMask().toString():""));
    buf += sprintf(buf,"\"gateway\":\"%s\",", (!AppConn.isAccessPoint()?WiFi.gatewayIP().toString():""));
    buf += sprintf(buf,"\"st_ip\":\"%s\",", (AppConn.getStaticIP()->ip?AppConn.getStaticIP()->ip->toString():""));
    buf += sprintf(buf,"\"st_subnet\":\"%s\",", (AppConn.getStaticIP()->netmask?AppConn.getStaticIP()->netmask->toString():""));
    buf += sprintf(buf,"\"st_gateway\":\"%s\",", (AppConn.getStaticIP()->gateway?AppConn.getStaticIP()->gateway->toString():""));
    buf += sprintf(buf,"\"dns1\":\"%s\",", (AppConn.getStaticIP()->dns1?AppConn.getStaticIP()->dns1->toString():""));
    buf += sprintf(buf,"\"dns2\":\"%s\",", (AppConn.getStaticIP()->dns1?AppConn.getStaticIP()->dns2->toString():""));
    buf += sprintf(buf,"\"ap_ip\":\"%s\",", (AppConn.getAPIP()->ip?AppConn.getAPIP()->ip->toString():""));
    buf += sprintf(buf,"\"ap_subnet\":\"%s\",", (AppConn.getAPIP()->netmask?AppConn.getAPIP()->netmask->toString():""));
    buf += sprintf(buf,"\"ap_channel\":\"%i\",", AppConn.getAPChannel());
    buf += sprintf(buf,"\"ap_dhcp\":\"%i\",", AppConn.getAPDHCP());

    buf += sprintf(buf,"\"mdns_name\":\"%s\",", AppConn.getMDNSname());
    buf += sprintf(buf,"\"port\":%i,", AppConn.getPort());
    byte mac[6];
    WiFi.macAddress(mac);
    buf += sprintf(buf,"\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\",", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    buf += sprintf(buf,"\"local_time\":\"%s\",", (!AppConn.isAccessPoint()?AppConn.getLocalTimeStr():""));
    buf += sprintf(buf,"\"up_time\":\"%s\",", AppConn.getUpTimeStr());
    buf += sprintf(buf,"\"ntp_server\":\"%s\",", AppConn.getNTPServer());
    buf += sprintf(buf,"\"gmt_offset\":%li,", AppConn.getGmtOffset_sec());
    buf += sprintf(buf,"\"dst_offset\":%i,", AppConn.getDaylightOffset_sec());
    buf += sprintf(buf,"\"active_streams\":%i,", AppHttpd.getStreamCount());
    buf += sprintf(buf,"\"prev_streams\":%lu,", AppHttpd.getStreamsServed());
    buf += sprintf(buf,"\"img_captured\":%lu,", AppHttpd.getImagesServed());
    buf += sprintf(buf,"\"ota_enabled\":%i,", (AppConn.isOTAEnabled()? 1:0 ));

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

    buf += sprintf(buf,"\"storage_size\":%i,", Storage.getSize());
    buf += sprintf(buf,"\"storage_used\":%i,", Storage.getUsed());
    buf += sprintf(buf,"\"storage_units\":\"%s\",", (Storage.capacityUnits()==STORAGE_UNITS_MB?"MB":""));
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
    
    json_obj_get_int(&jctx, (char*)"lamp", &lampVal);
    json_obj_get_bool(&jctx, (char*)"autolamp", &autoLamp);
    json_obj_get_int(&jctx, (char*)"flashlamp", &flashLamp);

    int count = 0, pin = 0, freq = 0, resolution = 0, def_val = 0;

    if(json_obj_get_array(&jctx, (char*)"pwm", &count) == OS_SUCCESS) {

        for(int i=0; i < count && i < NUM_PWM; i++) 
            if(json_arr_get_object(&jctx, i) == OS_SUCCESS) {
                if(json_obj_get_int(&jctx, (char*)"pin", &pin) == OS_SUCCESS &&
                    json_obj_get_int(&jctx, (char*)"frequency", &freq) == OS_SUCCESS &&
                    json_obj_get_int(&jctx, (char*)"resolution", &resolution) == OS_SUCCESS) {
                    int index = attachPWM(pin, freq, resolution);
                    if(index >= 0) {
                        if(lampVal >= 0 && i == 0) {
                            lamppin = pin;
                            pwmMax = pow(2, resolution)-1;
                            Serial.printf("Flash lamp activated on pin %d\r\n", lamppin);
                        }

                        if(json_obj_get_int(&jctx, (char*)"default", &def_val) == OS_SUCCESS)  {
                            pwm[index]->setDefaultDuty(def_val);
                            pwm[index]->reset();
                        }
                    }
                    else
                        Serial.printf("Failed to attach PWM to pin %d\r\n", pin);
                }
                json_arr_leave_object(&jctx);
            }

        json_obj_leave_array(&jctx);
    }
    

    if (json_obj_get_array(&jctx, (char*)"mapping", &mappingCount) == OS_SUCCESS) {

        for(int i=0; i < mappingCount && i < MAX_URI_MAPPINGS; i++) {
            if(json_arr_get_object(&jctx, i) == OS_SUCCESS) {
                UriMapping *um = (UriMapping*) malloc(sizeof(UriMapping));
                if(json_obj_get_string(&jctx, (char*)"uri", um->uri, sizeof(um->uri)) == OS_SUCCESS &&
                    json_obj_get_string(&jctx, (char*)"path", um->path, sizeof(um->path)) == OS_SUCCESS ) {
                    mappingList[i] = um;
                } 
                else {
                    free(um);
                }
                json_arr_leave_object(&jctx);
            }    
        }    
        json_obj_leave_array(&jctx);
    }

    json_obj_get_string(&jctx, (char*)"my_name", myName, sizeof(myName));
    bool dbg;
    if(json_obj_get_bool(&jctx, (char*)"debug_mode", &dbg) == OS_SUCCESS)
        setDebugMode(dbg);  

    return ret;
}

int CLAppHttpd::savePrefs() {
    char * prefs_file = getPrefsFileName(true); 
    char buf[1024];
    json_gen_str_t jstr;
    json_gen_str_start(&jstr, buf, sizeof(buf), NULL, NULL);
    json_gen_start_object(&jstr);
    
    json_gen_obj_set_string(&jstr, (char*)"my_name", myName);

    json_gen_obj_set_int(&jstr, (char*)"lamp", lampVal);
    json_gen_obj_set_bool(&jstr, (char*)"autolamp", autoLamp);
    json_gen_obj_set_int(&jstr, (char*)"flashlamp", flashLamp);

    if(pwmCount > 0) {
        json_gen_push_array(&jstr, (char*)"pwm");
        for(int i=0; i < pwmCount; i++) 
            if(pwm[i]) {
                json_gen_start_object(&jstr);
                json_gen_obj_set_int(&jstr, (char*)"pin", pwm[i]->getPin());
                json_gen_obj_set_int(&jstr, (char*)"frequency", pwm[i]->getFreq());
                json_gen_obj_set_int(&jstr, (char*)"resolution", pwm[i]->getResolutionBits());
                if(pwm[i]->getDefaultDuty())
                    json_gen_obj_set_int(&jstr, (char*)"default", pwm[i]->getDefaultDuty());
                json_gen_end_object(&jstr); 
            }
        
        json_gen_pop_array(&jstr);
    }

    if(mappingCount > 0) {
        json_gen_push_array(&jstr, (char*)"mapping");
        for(int i=0; i < mappingCount; i++) {
            json_gen_start_object(&jstr);
            json_gen_obj_set_string(&jstr, (char*)"uri", mappingList[i]->uri);
            json_gen_obj_set_string(&jstr, (char*)"path", mappingList[i]->path);
            json_gen_end_object(&jstr); 
        }
        json_gen_pop_array(&jstr);
    }
    json_gen_obj_set_bool(&jstr, (char*)"debug_mode", isDebugMode());

    json_gen_end_object(&jstr);
    json_gen_str_end(&jstr);

    File file = Storage.open(prefs_file, FILE_WRITE);
    if(file) {
        file.print(buf);
        file.close();
        return OK;
    }
    else {
        Serial.printf("Failed to save web server preferences to file %s\r\n", prefs_file);
        return FAIL;
    }
}

int CLAppHttpd::attachPWM(uint8_t pin, double freq, uint8_t resolution_bits) {

    if(pwmCount >= NUM_PWM) {
        Serial.println("Number of available PWM channels exceeded");
        return OS_FAIL;
    }

    for(int i=0; i<pwmCount; i++) 
        if(pwm[i]->getPin() == pin) {
            Serial.printf("Pin %d already utilized\r\n");
            return OS_FAIL; // pin already used
        }

    ESP32PWM * newpwm = new ESP32PWM();
    if(!newpwm) {
        Serial.println("Failed to create PWM"); 
        delete newpwm;
        return OS_FAIL;
    }
    
    newpwm->attachPin(pin, freq, resolution_bits);

    if(!newpwm->attached()) {
        Serial.print("Failed to attach PWM on pin "); Serial.println(pin);
        delete newpwm;
        return OS_FAIL;
    }

    Serial.printf("Created a new PWM channel %d on pin %d (freq=%.2f, bits=%d)\r\n", 
        newpwm->getChannel(), pin, freq, resolution_bits);

    pwm[pwmCount] = newpwm;

    pwmCount++;

    return pwmCount - 1; 
}

int CLAppHttpd::writePWM(uint8_t pin, int value, int min_v, int max_v) {
    for(int i=0; i<pwmCount; i++) {
        if(pwm[i] && pwm[i]->getPin() == pin) {
            if(pwm[i]->attached()) {
                if(min_v > 0) {
                    // treat values less than MIN_PULSE_WIDTH (500) as angles in degrees 
                    // (valid values in microseconds are handled as microseconds)
                    if (value < MIN_PULSE_WIDTH)
                    {
                        if (value < 0)
                            value = 0;
                        else if (value > 180)
                            value = 180;

                        value = map(value, 0, 180, min_v, max_v);

                    }
                    if (value < min_v)          // ensure pulse width is valid
                        value = min_v;
                    else if (value > max_v)
                        value = max_v;

                    value = pwm[i]->usToTicks(value);  // convert to ticks

                }
                // do the actual write
                if(isDebugMode())
                    Serial.printf("Write %d to PWM channel %d pin %d min %d max %d\r\n", 
                                  value, pwm[i]->getChannel(), pwm[i]->getPin(), min_v, max_v);
                pwm[i]->write(value);
                return OS_SUCCESS;
            }
            else {
                Serial.printf("PWM write failed: pin %d is not attached", pin);
                return OS_FAIL;    
            }
        }
    }
    
    Serial.printf("PWM write failed: pin %d is not found", pin);
    return OS_FAIL;
}

void CLAppHttpd::resetPWM(uint8_t pin) {
    for(int i=0; i<pwmCount; i++) {
        if(pwm[i]->getPin() == pin || pin == RESET_ALL_PWM)
            pwm[i]->reset();
    }
}


// Lamp Control
void CLAppHttpd::setLamp(int newVal) {

    if(newVal == DEFAULT_FLASH) {
        newVal = flashLamp;
    }
    lampVal = newVal;
    
    // Apply a logarithmic function to the scale.
    if(lamppin) {
        int brightness = round(lampVal * pwmMax/100.00);
        writePWM(lamppin, brightness,0);
    }

}

CLAppHttpd AppHttpd;
