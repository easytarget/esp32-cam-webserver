#include "app_cam.h"

CLAppCam::CLAppCam() {
    setTag("cam");
}


int CLAppCam::start() {
    // Populate camera config structure with hardware and other defaults
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = xclk * 1000000;
    config.pixel_format = PIXFORMAT_JPEG;
    // Low(ish) default framesize and quality
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;

    #if defined(CAMERA_MODEL_ESP_EYE)
        pinMode(13, INPUT_PULLUP);
        pinMode(14, INPUT_PULLUP);
    #endif

    // camera init
    setErr(esp_camera_init(&config));
    
    if (getLastErr()) {
        critERR = "Camera sensor failed to initialise";
        return getLastErr();
    } else {

        // Get a reference to the sensor
        sensor = esp_camera_sensor_get();

        // Dump camera module, warn for unsupported modules.
        switch (sensor->id.PID) {
            case OV9650_PID: 
                Serial.print("OV9650");  
                break;
            case OV7725_PID: 
                Serial.print("OV7725"); 
                break;
            case OV2640_PID: 
                Serial.print("OV2640"); 
                break;
            case OV3660_PID: 
                Serial.print("OV3660"); 
                break;
            default: 
                Serial.print("UNKNOWN");
                break;
        }
        Serial.println(" camera module detected");

    }

    return OS_SUCCESS;
}

int CLAppCam::stop() {
    Serial.println("Stopping Camera");
    return esp_camera_deinit();
}

int CLAppCam::loadPrefs() {
    jparse_ctx_t jctx;
    int ret  = parsePrefs(&jctx);
    if(ret != OS_SUCCESS) {
        return ret;
    }

  // process local settings

    json_obj_get_int(&jctx, (char*)"frame_rate", &frameRate);
    json_obj_get_int(&jctx, (char*)"xclk", &xclk);
    json_obj_get_int(&jctx, (char*)"rotate", &myRotation);

    // get sensor reference
    sensor_t * s = esp_camera_sensor_get();
    // process camera settings
    if(s) {
        s->set_framesize(s, (framesize_t)readJsonIntVal(&jctx, "framesize"));
        s->set_quality(s, readJsonIntVal(&jctx, "quality"));
        s->set_xclk(s, LEDC_TIMER_0, xclk);
        s->set_brightness(s, readJsonIntVal(&jctx, "brightness"));
        s->set_contrast(s, readJsonIntVal(&jctx, "contrast"));
        s->set_saturation(s, readJsonIntVal(&jctx, "saturation"));
        s->set_sharpness(s, readJsonIntVal(&jctx, "sharpness"));
        s->set_denoise(s, readJsonIntVal(&jctx, "denoise"));
        s->set_special_effect(s, readJsonIntVal(&jctx, "special_effect"));
        s->set_wb_mode(s, readJsonIntVal(&jctx, "wb_mode"));
        s->set_whitebal(s, readJsonIntVal(&jctx, "awb"));
        s->set_awb_gain(s, readJsonIntVal(&jctx, "awb_gain"));
        s->set_exposure_ctrl(s, readJsonIntVal(&jctx, "aec"));
        s->set_aec2(s, readJsonIntVal(&jctx, "aec2"));
        s->set_ae_level(s, readJsonIntVal(&jctx, "ae_level"));
        s->set_aec_value(s, readJsonIntVal(&jctx, "aec_value"));
        s->set_gain_ctrl(s, readJsonIntVal(&jctx, "agc"));
        s->set_agc_gain(s, readJsonIntVal(&jctx, "agc_gain"));
        s->set_gainceiling(s, (gainceiling_t)readJsonIntVal(&jctx, "gainceiling"));
        s->set_bpc(s, readJsonIntVal(&jctx, "bpc"));
        s->set_wpc(s, readJsonIntVal(&jctx, "wpc"));
        s->set_raw_gma(s, readJsonIntVal(&jctx, "raw_gma"));
        s->set_lenc(s, readJsonIntVal(&jctx, "lenc"));
        s->set_vflip(s, readJsonIntVal(&jctx, "vflip"));
        s->set_hmirror(s, readJsonIntVal(&jctx, "hmirror"));
        s->set_dcw(s, readJsonIntVal(&jctx, "dcw"));
        s->set_colorbar(s, readJsonIntVal(&jctx, "colorbar"));
        
        bool dbg;
        if(json_obj_get_bool(&jctx, (char*)"debug_mode", &dbg) == OS_SUCCESS)
            setDebugMode(dbg);   
    }
    else {
        Serial.println("Failed to get camera handle. Camera settings skipped");
    }
  
    // close the file
    json_parse_end(&jctx);
    return ret;
}

int CLAppCam::savePrefs(){
    char * prefs_file = getPrefsFileName(true); 

    if (Storage.exists(prefs_file)) {
        Serial.print("Updating "); 
    } else {
        Serial.print("Creating ");
    }
    Serial.println(prefs_file);
    
    char buf[CAM_DUMP_BUFFER_SIZE];
    json_gen_str_t jstr;
    json_gen_str_start(&jstr, buf, sizeof(buf), NULL, NULL);
    json_gen_start_object(&jstr);

    dumpStatusToJson(&jstr);

    json_gen_end_object(&jstr);
    json_gen_str_end(&jstr);

    File file = Storage.open(prefs_file, FILE_WRITE);
    if(file) {
        file.print(buf);
        file.close();
        return OK;
    }
    else {
        Serial.print("Failed to save camera preferences to file "); Serial.println(prefs_file);
        return FAIL;
    }

}

int IRAM_ATTR CLAppCam::snapToBuffer() {
    fb = esp_camera_fb_get();

    return (fb?ESP_OK:ESP_FAIL);
}

void IRAM_ATTR CLAppCam::releaseBuffer() {
    if(fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
    }
}

void CLAppCam::dumpStatusToJson(json_gen_str_t * jstr, bool full_status) {
 
    
    json_gen_obj_set_int(jstr, (char*)"rotate", myRotation);
    
    if(getLastErr()) return;

    sensor_t * s = esp_camera_sensor_get();
    json_gen_obj_set_int(jstr, (char*)"cam_pid", s->id.PID);
    json_gen_obj_set_int(jstr, (char*)"cam_ver", s->id.VER);        
    json_gen_obj_set_int(jstr, (char*)"framesize", s->status.framesize);
    json_gen_obj_set_int(jstr, (char*)"frame_rate", frameRate);   
    
    if(!full_status) return;

    json_gen_obj_set_int(jstr, (char*)"quality", s->status.quality);
    json_gen_obj_set_int(jstr, (char*)"brightness", s->status.brightness);
    json_gen_obj_set_int(jstr, (char*)"contrast", s->status.contrast);
    json_gen_obj_set_int(jstr, (char*)"saturation", s->status.saturation);
    json_gen_obj_set_int(jstr, (char*)"sharpness", s->status.sharpness);
    json_gen_obj_set_int(jstr, (char*)"denoise", s->status.denoise);
    json_gen_obj_set_int(jstr, (char*)"special_effect", s->status.special_effect);
    json_gen_obj_set_int(jstr, (char*)"wb_mode", s->status.wb_mode);
    json_gen_obj_set_int(jstr, (char*)"awb", s->status.awb);
    json_gen_obj_set_int(jstr, (char*)"awb_gain", s->status.awb_gain);
    json_gen_obj_set_int(jstr, (char*)"aec", s->status.aec);
    json_gen_obj_set_int(jstr, (char*)"aec2", s->status.aec2);
    json_gen_obj_set_int(jstr, (char*)"ae_level", s->status.ae_level);
    json_gen_obj_set_int(jstr, (char*)"aec_value", s->status.aec_value);
    json_gen_obj_set_int(jstr, (char*)"agc", s->status.agc);
    json_gen_obj_set_int(jstr, (char*)"agc_gain", s->status.agc_gain);
    json_gen_obj_set_int(jstr, (char*)"gainceiling", s->status.gainceiling);
    json_gen_obj_set_int(jstr, (char*)"bpc", s->status.bpc);
    json_gen_obj_set_int(jstr, (char*)"wpc", s->status.wpc);
    json_gen_obj_set_int(jstr, (char*)"raw_gma", s->status.raw_gma);
    json_gen_obj_set_int(jstr, (char*)"lenc", s->status.lenc);
    json_gen_obj_set_int(jstr, (char*)"vflip", s->status.vflip);
    json_gen_obj_set_int(jstr, (char*)"hmirror", s->status.hmirror);
    json_gen_obj_set_int(jstr, (char*)"dcw", s->status.dcw);
    json_gen_obj_set_int(jstr, (char*)"colorbar", s->status.colorbar);
    json_gen_obj_set_bool(jstr, (char*)"debug_mode", isDebugMode());

    json_gen_obj_set_int(jstr, (char*)"xclk", xclk); 

}



CLAppCam AppCam;