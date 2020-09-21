 /*******************************************************************************************************************
 *            
 *     Cycling close pass distance recorder using 
 *     an Esp32camera module along with a JSN-SR04T ultrasonic distance sensor 
 *     
 *     - created using the Arduino IDE
 *     
 *     
 *     led flashes at startup:
 *        1 = no sd card
 *        2 = invalid format sd card
 *        3 = all ok
 *            
 *            
 *******************************************************************************************************************/


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------

  const String stitle = "ESP32Cam-range";                // title of this sketch

  const String sversion = "21Sep20";                     // Sketch version

  int minTimeBetweenTriggers = 3;                        // minimum time between triggers in seconds

  int TimeBetweenStatus = 2;                             // minimum time between status light flash
  
  

// ---------------------------------------------------------------

#include "camera.h"                 // camera related code
  
// Define ultrasonic range sensor pins (jsn-sr04t)     13/12
  #define trigPin 13
  #define echoPin 12   

// Define general variables:
  long duration;
  int distance;
  int imageCounter = 0;             // counter for file names on sdcard
  int indicatorLED = 33;            // onboard LED (33)
  int brightLED = 4;                // onboard bright LED (flash on pin 4)
  uint32_t lastTrigger = millis();  // last time camera was triggered
  uint32_t lastStatus = millis();   // last time status light flashed
  

#include "soc/soc.h"                         // Used to disable brownout problems
#include "soc/rtc_cntl_reg.h"      

// sd card - see https://randomnerdtutorials.com/esp32-cam-take-photo-save-microsd-card/
  #include "SD_MMC.h"
  #include <SPI.h>                       
  #include <FS.h>                            // gives file access 
  #define SD_CS 5                            // sd chip select pin
  bool SD_Present;                           // flag if an sd card was found (0 = no)


// ******************************************************************************************************************


// ---------------------------------------------------------------
//    -SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
// ---------------------------------------------------------------

void setup() {
  
  // Serial communication 
    Serial.begin(115200);
  
    Serial.println(("\n\n\n---------------------------------------"));
    Serial.println("Starting - " + stitle + " - " + sversion);
    Serial.println(("---------------------------------------"));

   // set up camera
    Serial.print(("Initialising camera: "));
    Serial.println(setupCameraHardware() ? "OK" : "ERR INIT");

   // start sd card
      if (!SD_MMC.begin("/sdcard", true)) {         // if loading sd card fails     
        // note: ("/sdcard", true) = 1 wire - see: https://www.reddit.com/r/esp32/comments/d71es9/a_breakdown_of_my_experience_trying_to_talk_to_an/
        Serial.println("SD Card not found"); 
        flashLED(1);  
        while(1);   // stop
      }
      uint8_t cardType = SD_MMC.cardType();
      if (cardType == CARD_NONE) {                 // if no sd card found
          Serial.println("SD Card type detect failed"); 
          flashLED(2);
          while(1);  // stop
      } 
      uint16_t SDfreeSpace = (uint64_t)(SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
      Serial.println("SD Card found, free space = " + String(SDfreeSpace) + "MB");   

  // discover number of files on sd card and set file counter accordingly
    fs::FS &fs = SD_MMC; 
    File root = fs.open("/");
    while (true)
    {
      File entry =  root.openNextFile();
      if (! entry) break;
      imageCounter ++;    // increment image counter
      entry.close();
    }
    root.close();
    Serial.println("File count = " + String(imageCounter));

  // Define io pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(indicatorLED, OUTPUT);
    digitalWrite(indicatorLED,HIGH);
    pinMode(brightLED, OUTPUT);
    digitalWrite(brightLED,LOW);

  // Turn-off the 'brownout detector'
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  if (!psramFound()) Serial.println("Warning: No PSRam found");

  flashLED(3);    // all ok

  storeImage("startup");        // capture and store a live image
  
}


// ******************************************************************************************************************


void loop() {

  // Read distance from sensor
    int tdist = readDistance();    // distance in cm
    String textDist = "Unk";       // distance reading as a string
    if (tdist > 0) textDist = String(tdist) + "cm";
    Serial.println("Distance = " + textDist);

  // if distance is less than 30cm 
  if (tdist < 30 && tdist > 0) {
    // if long enough since last trigger
      if ((unsigned long)(millis() - lastTrigger) >= (minTimeBetweenTriggers * 1000)) { 
        lastTrigger = millis();       // reset timer
        storeImage(textDist);         // capture and store a live image
      }
  }

  // flash ststus light to show all ok
    if ((unsigned long)(millis() - lastStatus) >= (TimeBetweenStatus * 1000)) { 
      lastStatus = millis();              // reset timer
      digitalWrite(indicatorLED,LOW);     // led on
    }
    delay(100);
    digitalWrite(indicatorLED,HIGH);     // led off
}


// ******************************************************************************************************************


// Read distance from ultrasound sensor
//    returns result in centimeters

int readDistance() {
  
  // Clear the trigPin by setting it LOW:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  
 // Trigger the sensor by setting the trigPin high for 10 microseconds:
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
  duration = pulseIn(echoPin, HIGH, 25000);
  
  // Calculate the distance:
  distance = duration*0.034/2;

  return distance;
}

  
// ******************************************************************************************************************


// Flash the LED

void flashLED(int reps) {
    for(int x; x <= reps; x++) {
      digitalWrite(indicatorLED,LOW);
      delay(500);
      digitalWrite(indicatorLED,HIGH);
      delay(500);
    }
}


// ******************************************************************************************************************


// save image to sd card
//    returns 1 if image saved ok,  iTitle = add to end of title of file


byte storeImage(String iTitle) {

  Serial.println("Storing image #" + String(imageCounter) + " to sd card");

  fs::FS &fs = SD_MMC; 

  // capture live image from camera
  cameraImageSettings();                            // apply camera sensor settings
  digitalWrite(brightLED,HIGH);                     // turn flash on
  camera_fb_t *fb = esp_camera_fb_get();            // capture frame from camera
  digitalWrite(brightLED,LOW);                      // turn flash off
  if (!fb) {
    Serial.println("Camera capture failed");
    flashLED(1);
    return(0);
  }

  // save image
    imageCounter ++;                                // increment image counter
    String SDfilename = "/" + String(imageCounter) + "-" + iTitle + ".jpg";
    File file = fs.open(SDfilename, FILE_WRITE);
    if (!file) {
      Serial.println("Error: Failed to create file on sd-card: " + SDfilename);
      flashLED(1);
      return(0);
    }
    else {
        if (file.write(fb->buf, fb->len)) Serial.println("Saved image to sd card");
        else {
          Serial.println("Error: failed to save image to sd card");
          flashLED(1);
          return(0);
        }
        file.close();
        // flashLED(2);
        return(1);
    }


}



// ******************************************************************************************************************
// end
