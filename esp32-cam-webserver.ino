
#include "src/app_config.h"     // global definitions
#include "src/storage.h"        // Filesystem
#include "src/app_conn.h"       // Conectivity 
#include "src/app_cam.h"        // Camera 
#include "src/app_httpd.h"      // Web server
#include "src/camera_pins.h"    // Pin Mappings

/* 
 * This sketch is a extension/expansion/rework of the ESP32 Camera webserer example.
 * 
 */


void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // Warn if no PSRAM is detected (typically user error with board selection in the IDE)
    if(!psramFound()){
        Serial.println("\r\nFatal Error; Halting");
        while (true) {
            Serial.println("No PSRAM found; camera cannot be initialised: Please check the board config for your module.");
            delay(5000);
        }
    }

    #if defined(LED_PIN)  // If we have a notification LED, set it to output
        pinMode(LED_PIN, OUTPUT);
    #endif

    // Start the filesystem before we initialise the camera
    filesystemStart();
    delay(200); // a short delay to let spi bus settle after init

    // Start (init) the camera 
    if (AppCam.start() != OS_SUCCESS) {
        delay(100);  // need a delay here or the next serial o/p gets missed
        Serial.println();
        Serial.print("CRITICAL FAILURE:"); Serial.println(AppCam.getErr());
        Serial.println("A full (hard, power off/on) reboot will probably be needed to recover from this.");
        Serial.println("Meanwhile; this unit will reboot in 1 minute since these errors sometime clear automatically");
        resetI2CBus();
        scheduleReboot(60);
    }
    else
        Serial.println("Camera init succeeded");

    // Now load and apply preferences
    delay(200); // a short delay to let spi bus settle after camera init
    AppCam.loadPrefs();

    /*
    * Camera setup complete; initialise the rest of the hardware.
    */

    // Start Wifi and loop until we are connected or have started an AccessPoint
    while (AppConn.wifiStatus() != WL_CONNECTED)  {
        if(AppConn.start() != WL_CONNECTED) {
            Serial.println("Failed to initiate WiFi, retryng in 5 sec ... ");
            delay(5000);
        }
        else {
            // Flash the LED to show we are connected
            notifyConect();
        }
    }

    // Set up OTA
    #ifndef NO_OTA
        AppConn.enableOTA();
    #else
        AppConn.enableOTA(false);
    #endif

    // http service attached to port
    AppConn.configMDNS();

    // Set time via NTP server when enabled
    if(!AppConn.isAccessPoint()) {
        AppConn.configNTP();
        Serial.print("Time: ");
        AppConn.printLocalTime(true);
    }

    // Start the web server
    AppHttpd.start();

}

void loop() {
    /*
     *  Just loop forever, reconnecting Wifi As necesscary in client mode
     * The stream and URI handler processes initiated by the startCameraServer() call at the
     * end of setup() will handle the camera and UI processing from now on.
    */
    if (AppConn.isAccessPoint()) {
        // Accespoint is permanently up, so just loop, servicing the captive portal as needed
        // Rather than loop forever, follow the watchdog, in case we later add auto re-scan.
        unsigned long pingwifi = millis();
        while (millis() - pingwifi < WIFI_WATCHDOG ) {
            // delay(100);
            AppConn.handleOTA();
            handleSerial();
            AppConn.handleDNSRequest();
        }
    } else {
        // client mode can fail; so reconnect as appropriate

        if (AppConn.wifiStatus() == WL_CONNECTED) {
            // We are connected
            // loop here for §
            unsigned long pingwifi = millis();
            while (millis() - pingwifi < WIFI_WATCHDOG ) {
                AppConn.handleOTA();
                handleSerial();
            }
        } else {
            // disconnected; notify 
            notifyDisconnect();

            // ensures disconnect is complete, wifi scan cleared
            AppConn.stop();  

            //attempt to reconnect
            if(AppConn.start() == WL_CONNECTED) {
                notifyConect();
            }
            
        }
    }
}


/// @brief tries to initialize the filesystem until success, otherwise loops indefinitely
void filesystemStart(){
  Serial.println("Starting filesystem");
  while ( !Storage.init() ) {
    // if we sit in this loop something is wrong;
    Serial.println("Filesystem mount failed");
    for (int i=0; i<10; i++) {
      flashLED(100); // Show filesystem failure
      delay(100);
    }
    delay(1000);
    Serial.println("Retrying..");
  }
  
  // Storage.listDir("/", 0);
}

// Serial input 
void handleSerial() {
    if(Serial.available()) {
        char cmd = Serial.read();

        // Rceiving commands and data from serial. Any input, which doesnt start from '#' is ignored.
        if (cmd == '#' ) {
            String rsp = Serial.readStringUntil('\n');
            rsp.trim();
            sprintf(AppHttpd.getSerialBuffer(), rsp.c_str());
        }
    }
}

void notifyConect() {
    for (int i = 0; i < 5; i++) {
        flashLED(150);
        delay(50);
    }
    AppHttpd.serialSendCommand("Connected");
}

void notifyDisconnect() {
    AppHttpd.serialSendCommand("Disconnected");
}

// Flash LED if LED pin defined
void flashLED(int flashtime) {
#ifdef LED_PIN
    digitalWrite(LED_PIN, LED_ON);
    delay(flashtime);
    digitalWrite(LED_PIN, LED_OFF);
#endif
}

void scheduleReboot(int delay) {
    esp_task_wdt_init(delay,true);
    esp_task_wdt_add(NULL);
}

// Reset the I2C bus.. may help when rebooting.
void resetI2CBus() {
    periph_module_disable(PERIPH_I2C0_MODULE); // try to shut I2C down properly in case that is the problem
    periph_module_disable(PERIPH_I2C1_MODULE);
    periph_module_reset(PERIPH_I2C0_MODULE);
    periph_module_reset(PERIPH_I2C1_MODULE);
}

