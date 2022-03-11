/*
 *   Pin definitions for some common ESP-CAM modules
 *
 *   Select the module to use in myconfig.h
 *   Defaults to AI-THINKER CAM module
 *
 */
#if defined(CAMERA_MODEL_AI_THINKER)
  //
  // AI Thinker
  // https://github.com/SeeedDocument/forum_doc/raw/master/reg/ESP32_CAM_V1.6.pdf
  //
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  #define LED_PIN           33 // Status led
  #define LED_ON           LOW // - Pin is inverted.
  #define LED_OFF         HIGH //
  #define LAMP_PIN           4 // LED FloodLamp.

#elif defined(CAMERA_MODEL_WROVER_KIT)
  //
  // ESP WROVER
  // https://dl.espressif.com/dl/schematics/ESP-WROVER-KIT_SCH-2.pdf
  //
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    21
  #define SIOD_GPIO_NUM    26
  #define SIOC_GPIO_NUM    27
  #define Y9_GPIO_NUM      35
  #define Y8_GPIO_NUM      34
  #define Y7_GPIO_NUM      39
  #define Y6_GPIO_NUM      36
  #define Y5_GPIO_NUM      19
  #define Y4_GPIO_NUM      18
  #define Y3_GPIO_NUM       5
  #define Y2_GPIO_NUM       4
  #define VSYNC_GPIO_NUM   25
  #define HREF_GPIO_NUM    23
  #define PCLK_GPIO_NUM    22
  #define LED_PIN           2 // A status led on the RGB; could also use pin 0 or 4
  #define LED_ON         HIGH //
  #define LED_OFF         LOW //
  // #define LAMP_PIN          x // No LED FloodLamp.

#elif defined(CAMERA_MODEL_ESP_EYE)
  //
  // ESP-EYE
  // https://twitter.com/esp32net/status/1085488403460882437
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM     4
  #define SIOD_GPIO_NUM    18
  #define SIOC_GPIO_NUM    23
  #define Y9_GPIO_NUM      36
  #define Y8_GPIO_NUM      37
  #define Y7_GPIO_NUM      38
  #define Y6_GPIO_NUM      39
  #define Y5_GPIO_NUM      35
  #define Y4_GPIO_NUM      14
  #define Y3_GPIO_NUM      13
  #define Y2_GPIO_NUM      34
  #define VSYNC_GPIO_NUM    5
  #define HREF_GPIO_NUM    27
  #define PCLK_GPIO_NUM    25
  #define LED_PIN          21 // Status led
  #define LED_ON         HIGH //
  #define LED_OFF         LOW //
  // #define LAMP_PIN          x // No LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
  //
  // ESP32 M5STACK
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_V2_PSRAM)
  //
  // ESP32 M5STACK V2
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     22
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_WIDE)
  //
  // ESP32 M5STACK WIDE
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     22
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  //
  // Common M5 Stack without PSRAM
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // Note NO PSRAM,; so maximum working resolution is XGA 1024×768
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_TTGO_T_JOURNAL)
  //
  // LilyGO TTGO T-Journal ESP32; with OLED! but not used here.. :-(
  #define PWDN_GPIO_NUM      0
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // TTGO T Journal status/illumination LED details unknown/unclear
  // #define LED_PIN           33 // Status led
  // #define LED_ON           LOW // - Pin is inverted.
  // #define LED_OFF         HIGH //
  // #define LAMP_PIN           4 // LED FloodLamp.

#elif defined(CAMERA_MODEL_ARDUCAM_ESP32S_UNO)
  // Pins from user @rdragonrydr
  // https://github.com/ArduCAM/ArduCAM_ESP32S_UNO/
  // Based on AI-THINKER definitions
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  #define LED_PIN            2 // Status led
  #define LED_ON          HIGH // - Pin is not inverted.
  #define LED_OFF          LOW //
  //#define LAMP_PIN           x // No LED FloodLamp.

#else
  // Well.
  // that went badly...
  #error "Camera model not selected, did you forget to uncomment it in myconfig?"
#endif
