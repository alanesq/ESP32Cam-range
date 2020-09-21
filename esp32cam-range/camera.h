/**************************************************************************************************
 *  
 *  ESP32 camera - Sep20
 * 
 *  
 *  For info on the camera module see: https://github.com/espressif/esp32-camera
 * 
 * 
 **************************************************************************************************/


#define DEBUG_MOTION 0        // extended serial debug enable for motion.h

#include "esp_camera.h"       // https://github.com/espressif/esp32-camera


// camera type settings (CAMERA_MODEL_AI_THINKER)
  #define CAMERA_MODEL_AI_THINKER
  #define PWDN_GPIO_NUM     32      // power down pin
  #define RESET_GPIO_NUM    -1      // -1 = not used
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26      // i2c sda
  #define SIOC_GPIO_NUM     27      // i2c scl
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25      // vsync_pin
  #define HREF_GPIO_NUM     23      // href_pin
  #define PCLK_GPIO_NUM     22      // pixel_clock_pin

  
//   ---------------------------------------------------------------------------------------------------------------------


// forward delarations
    bool setupCameraHardware();
    bool cameraImageSettings();


// camera configuration settings
    camera_config_t config;


//   ---------------------------------------------------------------------------------------------------------------------

    
/**
 * Setup camera hardware
 */

bool setupCameraHardware() {
  
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
    config.xclk_freq_hz = 20000000;               // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    config.pixel_format = PIXFORMAT_JPEG;         // PIXFORMAT_ + YUV422, GRAYSCALE, RGB565, JPEG, RGB888?
    config.frame_size = FRAMESIZE_XGA;            // FRAMESIZE_ + QVGA, CIF, VGA, SVGA, XGA, SXGA, UXGA 
    config.jpeg_quality = 10;                     // 0-63 lower number means higher quality
    config.fb_count = 1;                          // if more than one, i2s runs in continuous mode. Use only with JPEG
    
    esp_err_t camerr = esp_camera_init(&config);  // initialise the camera
    if (camerr != ESP_OK) Serial.printf("ERROR: Camera init failed with error 0x%x", camerr);

    cameraImageSettings();                 // apply camera sensor settings
    
    return (camerr == ESP_OK);             // return boolean result of camera initilisation
}


//   ---------------------------------------------------------------------------------------------------------------------

/**
 * apply camera sensor/image settings
 */

bool cameraImageSettings() { 
   
    sensor_t *s = esp_camera_sensor_get();  

    if (s == NULL) {
      Serial.println("Error: problem getting camera sensor settings");
      return 0;
    } 

    #if IMAGE_SETTINGS           // Implement adjustment of image settings 

      // If you enable gain_ctrl or exposure_ctrl it will prevent a lot of the other settings having any effect
      // more info on settings here: https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/
      s->set_gain_ctrl(s, 0);                       // auto gain off (1 or 0)
      s->set_exposure_ctrl(s, 0);                   // auto exposure off (1 or 0)
      s->set_agc_gain(s, cameraImageGain);          // set gain manually (0 - 30)
      s->set_aec_value(s, cameraImageExposure);     // set exposure manually  (0-1200)
      s->set_vflip(s, cameraImageInvert);           // Invert image (0 or 1)     
      s->set_quality(s, 10);                        // (0 - 63)
      s->set_gainceiling(s, GAINCEILING_32X);       // Image gain (GAINCEILING_x2, x4, x8, x16, x32, x64 or x128) 
      s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
      s->set_lenc(s, 1);                            // lens correction? (1 or 0)
      s->set_saturation(s, 0);                      // (-2 to 2)
      s->set_contrast(s, cameraImageContrast);      // (-2 to 2)
      s->set_sharpness(s, 0);                       // (-2 to 2)  
      s->set_hmirror(s, 0);                         // (0 or 1) flip horizontally
      s->set_colorbar(s, 0);                        // (0 or 1) - show a testcard
      s->set_special_effect(s, 0);                  // (0 to 6?) apply special effect
//       s->set_whitebal(s, 0);                        // white balance enable (0 or 1)
//       s->set_awb_gain(s, 1);                        // Auto White Balance enable (0 or 1) 
//       s->set_wb_mode(s, 0);                         // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
//       s->set_dcw(s, 0);                             // downsize enable? (1 or 0)?
//       s->set_raw_gma(s, 1);                         // (1 or 0)
//       s->set_aec2(s, 0);                            // automatic exposure sensor?  (0 or 1)
//       s->set_ae_level(s, 0);                        // auto exposure levels (-2 to 2)
      s->set_bpc(s, 0);                             // black pixel correction
      s->set_wpc(s, 0);                             // white pixel correction
    #endif

    // capture a frame to ensure settings apply (not sure if this is really needed)
      camera_fb_t *frame_buffer = esp_camera_fb_get();    // capture frame from camera
      esp_camera_fb_return(frame_buffer);                 // return frame so memory can be released
    
    return 1;
}


// ------------------------------------------------- end ----------------------------------------------------------------
