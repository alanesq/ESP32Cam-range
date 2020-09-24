 /*******************************************************************************************************************
 *            
 *     Cycling close pass distance recorder using ESP32Cam module along with a JSN-SR04T ultrasonic distance sensor 
 *                                 Github: https://github.com/alanesq/ESP32Cam-range
 *     
 * 
 *     
 *     - created using the Arduino IDE with ESP32 module installed   (https://dl.espressif.com/dl/package_esp32_index.json)
 *       No additional libraries required
 * 
 * 
 *     Four wires connect the esp32 to the ultrasonic sensor:
 *          5v to 5v
 *          GND to GND
 *          13 to Trigger (Rx)
 *          12 to Echo (Tx)
 *     Note: Ideally there should be a voltage converter on pins 12 and 13 or at least a resistor although I have been using 
 *           it without myself...
 *     
 *     
 *     Status led error flashes:
 *        2 = no sd card found
 *        3 = invalid format sd card found
 *        4 = failed to start camera
 *        5 = failed to capture image from camera
 *        6 = failed to store image to sd card
 *        7 = failed to create log file
 *        constant = Waiting for close object to clear
 * 
 * 
 *     Info on the esp32cam board:  https://randomnerdtutorials.com/esp32-cam-video-streaming-face-recognition-arduino-ide/
 *            
 *            
 * 
 *******************************************************************************************************************/


#include "esp_camera.h"       // https://github.com/espressif/esp32-camera


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------

  const String stitle = "ESP32Cam-range";                // title of this sketch

  const String sversion = "24Sep20";                     // Sketch version

  const bool debugInfo = 0;                              // show additional debug info. on serial port (1=enabled)
  
  const bool camEnable = 0;                              // Enable storing of images (1=enable)
  const int triggerDistance = 150;                       // distance below which will trigger an image capture (in cm)
  const bool flashRequired = 1;                          // If flash to be used when capturing image (1 = yes)
  const int minTimeBetweenTriggers = 5;                  // minimum time between camera triggers in seconds
  const framesize_t FRAME_SIZE_IMAGE = FRAMESIZE_XGA;    // Image resolution:   160x120 (QQVGA), 128x160 (QQVGA2), 176x144 (QCIF), 240x176 (HQVGA), 
                                                         //                     320x240 (QVGA), 400x296 (CIF), 640x480 (VGA, default), 800x600 (SVGA), 
                                                         //                     1024x768 (XGA), 1280x1024 (SXGA), 1600x1200 (UXGA)
                                                                                            
  const bool logEnable = 1;                              // Store constant log of distance readings (1=enable)
  const bool flashOnLog = 1;                             // if to flash the main LED when log is written to sd card
  const int logFrequency = 10;                           // how often to add distance to continual log file (1000 = 1 second, fastest possible is 12)
  const int entriesPerLog = 500;                         // how many entries to store in each log file

  const int numberReadings = 1;                          // number of readings to take from distance sensor to average together for final result
  const int echoTimeout = (5882 * 4.5);                  // timeout if no echo received from distance sensor (5882 ~ 1m, max = 4.5m)

  const int TimeBetweenStatus = 600;                     // speed of flashing system running ok status light (milliseconds)

  const int indicatorLED = 33;                           // onboard status LED pin (33)

  const int brightLED = 4;                               // onboard flash LED pin (4)

  
// ---------------------------------------------------------------


#include "camera.h"                         // insert camera related code 

#include "soc/soc.h"                        // Used to disable brownout problems
#include "soc/rtc_cntl_reg.h"      
  
// Define ultrasonic range sensor pins (jsn-sr04t)     13/12
//    Note: Pin 13 is the only fully available io pin when using an sd card (must be in 1 wire mode), 12 can be used but must be low at boot
  const int trigPin = 13;
  const int echoPin = 12;

// Define variables:
  bool camTriggered = 0;                    // flag if camera has been triggered
  int logsDist[entriesPerLog];              // log file temp store of distance readings
  uint32_t logsTime[entriesPerLog];         // log file temp store of time readings (millis)
  int templogCounter = 0;                   // counter for temp log store (when limit reached data is written to sd card)
  int imageCounter;                         // counter for image file names on sdcard
  int logCounter;                           // counter for log file names on sdcard
  uint32_t lastTrigger = millis();          // last time camera was triggered
  uint32_t lastStatus = millis();           // last time status light changed status (to flash all ok led)
  uint32_t logTimer = millis();             // last time log entry was stored

// sd card - see https://randomnerdtutorials.com/esp32-cam-take-photo-save-microsd-card/
  #include "SD_MMC.h"
  #include <SPI.h>                       
  #include <FS.h>                           // gives file access 
  #define SD_CS 5                           // sd chip select pin


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

  // Turn-off the 'brownout detector'
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Define io pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(indicatorLED, OUTPUT);
    digitalWrite(indicatorLED,HIGH);
    pinMode(brightLED, OUTPUT);
    digitalWrite(brightLED,LOW);

  // set up camera
      Serial.print(("Initialising camera: "));
      if (setupCameraHardware()) Serial.println("OK");
      else {
        Serial.println("Error!");
        showError(4);       // critical error so stop and flash led
      }

  // start sd card
      if (!SD_MMC.begin("/sdcard", true)) {         // if loading sd card fails     
        // note: ("/sdcard", true) = 1 wire - see: https://www.reddit.com/r/esp32/comments/d71es9/a_breakdown_of_my_experience_trying_to_talk_to_an/
        Serial.println("SD Card not found"); 
        showError(2);       // critical error so stop and flash led
      }
      uint8_t cardType = SD_MMC.cardType();
      if (cardType == CARD_NONE) {                 // if no sd card found
          Serial.println("SD Card type detect failed"); 
          showError(3);       // critical error so stop and flash led
      } 
      uint16_t SDfreeSpace = (uint64_t)(SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
      Serial.println("SD Card found, free space = " + String(SDfreeSpace) + "MB");  

  fs::FS &fs = SD_MMC;                    // sd card file system
  
  if (logEnable) {
    int tq=fs.mkdir("/log");        // create the "/log" folder on sd card
    if (!tq) Serial.println("Error creating LOG folder on sd card");
  }
    
  // discover number of image files stored on root of sd card and set image file counter accordingly
    File root = fs.open("/");
    while (true)
    {
      File entry =  root.openNextFile();
      if (! entry) break;
      imageCounter ++;    // increment image counter
      entry.close();
    }
    root.close();
    Serial.println("Image file count = " + String(imageCounter));

 // discover number of log files stored in /log of sd card and set log file counter accordingly
    if (logEnable) {
      root = fs.open("/log");
      while (true)
      {
        File entry =  root.openNextFile();
        if (! entry) break;
        logCounter ++;    // increment image counter
        entry.close();
      }
      root.close();
      Serial.println("Log file count = " + String(logCounter));
    }

  // redefine io pins as sd card seems to upset some of them
    pinMode(brightLED, OUTPUT);
    pinMode(indicatorLED, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

  if (!psramFound()) Serial.println("Warning: No PSRam found");

  // flash to show started ok
    digitalWrite(brightLED,HIGH);  // turn flash on
    delay(200);
    digitalWrite(brightLED,LOW);  // turn flash off

  // storeImage("startup");        // capture and store a live image
  
}


// ******************************************************************************************************************


// ----------------------------------------------------------------
//   -LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// ----------------------------------------------------------------


void loop() {

  // Read distance from ultrasonic sensor
    int tDist = readDistance(numberReadings);                              // read distance in cm
    String textDist = "Unk";                                               // distance reading as a string
    if (tDist > 0) textDist = String(tDist) + "cm";
    if (debugInfo) Serial.println("Distance = " + textDist);

  // if distance greater than trigger distance clear the triggered flag to re-enable triggering
    if (tDist > triggerDistance) {
      if (camTriggered == 1) digitalWrite(indicatorLED,HIGH);              // indicator led off when flag is cleared
      camTriggered = 0;
      if (debugInfo) Serial.println("Object clear");
    }

  // if distance is less than the trigger distance and recording of images is enabled
    if ( camEnable == 1 && tDist < triggerDistance && tDist > 0) {
      // if long enough since last trigger and not already triggered store image
        if ( camTriggered == 0 && ((unsigned long)(millis() - lastTrigger) >= (minTimeBetweenTriggers * 1000)) ) {
          storeImage(textDist); 
          digitalWrite(indicatorLED,LOW);                                  // indicator led on
          camTriggered = 1;                                                // flag that camera has been triggered (will not re trigger until this has been cleared)
        }
    }

  // add distance to continual logs if logs are enabled and suficient time since last entry
    if (logEnable == 1 && ((unsigned long)(millis() - logTimer) >= logFrequency)) logDistance(tDist);
    
  // flash status light to show sketch is running
    if ((unsigned long)(millis() - lastStatus) >= TimeBetweenStatus) { 
      lastStatus = millis();                                               // reset timer
      digitalWrite(indicatorLED,!digitalRead(indicatorLED));               // indicator led status change
    }
    
}


// ******************************************************************************************************************


// Read distance from ultrasound sensor - reads several times and take average
//    returns result in centimeters
//    noReps = number of distance readings to take average from

int readDistance(int noReps) {

  long duration = 0;                        // duration of returning echo
  int distance = 0;                         // calculated distance in cm
  int noReadingCounter = 0;                 // number of failed attempts to read distance

  for (int x=0; x < noReps; x++) {
  
    // Clear the trigPin by setting it LOW:
      digitalWrite(trigPin, LOW);
      delayMicroseconds(5);
    
   // Trigger the sensor by setting the trigPin high for 10 microseconds:
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH, echoTimeout);      // Read the echoPin. pulseIn() returns the duration (length of the pulse in millionths of a second) in microseconds:

    if (duration == 0) noReadingCounter++;               // increment failed attempt counter
    else distance += duration*0.034/2;                   // Calculate the distance in cm and add to running total  (speed of sound = 340m/s)
    
  }   // for loop

  if (distance > 0) distance = distance / ( noReps - noReadingCounter);      // calculate average, avoiding divide by zero if failed to get a reading

  return distance;
}

  
// ******************************************************************************************************************


// Flash the status LED

void flashLED(int reps) {
  for(int x=0; x < reps; x++) {
    digitalWrite(indicatorLED,LOW);
    delay(1000);
    digitalWrite(indicatorLED,HIGH);
    delay(500);
  }
}



// critical error - stop and flash error status on led

void showError(int errorNo) {
  while(1) {
    flashLED(errorNo);
    delay(4000);
  }
}


// ******************************************************************************************************************


// save image to sd card
//    iTitle = add to end of title of file


void storeImage(String iTitle) {
    
  // if (!camEnable) return;                        // if logs disabled do nothing

  lastTrigger = millis();                           // reset timer

  if (debugInfo) Serial.println("Storing image #" + String(imageCounter) + " to sd card");

  fs::FS &fs = SD_MMC;           // sd card file system

  // capture live image from camera
    // cameraImageSettings();                            // apply camera sensor settings (in camera.h)
  if (flashRequired) digitalWrite(brightLED,HIGH);  // turn flash on
  camera_fb_t *fb = esp_camera_fb_get();            // capture frame from camera
  digitalWrite(brightLED,LOW);                      // turn flash off
  if (!fb) {
    Serial.println("Camera capture failed");
    flashLED(5);
  }

  // take a distance reading and add to file name
    int tdist = readDistance(numberReadings);       // distance in cm
    iTitle += "(" + String(tdist) + "cm)";
  
  // save image
    imageCounter ++;                                                          // increment image counter
    String SDfilename = "/" + String(imageCounter) + "-" + iTitle + ".jpg";   // create image file name
    File file = fs.open(SDfilename, FILE_WRITE);                              // create file on sd card
    if (!file) {
      Serial.println("Error: Failed to create file on sd-card: " + SDfilename);
      flashLED(6);
    }
    else {
        if (file.write(fb->buf, fb->len)) Serial.println("Saved image to sd card");   // save image to file
        else {
          Serial.println("Error: failed to save image to sd card");
          flashLED(5);
        }
        file.close();                // close image file on sd card
    }
    esp_camera_fb_return(fb);        // return frame so memory can be released

}


// ******************************************************************************************************************


// add distance to log file
//    distToLog = the distance to add to the logs

void logDistance(int distToLog) {

    // if (!logEnable) return;                       // if logs disabled do nothing
 
    logTimer = millis();                         // reset timer
  
    if (distToLog == 0) distToLog = 500;         // if no return signal was received set it to max distance

    logsDist[templogCounter] = distToLog;        // add current distance entry to temp log
    logsTime[templogCounter] = millis();         // add current time to temp log
    templogCounter++;                            // increment temp log store counter
        
    if (templogCounter == entriesPerLog) {     
      // store to temp log entries to sd card
        if (debugInfo) Serial.println("Writing log to sd card");
        
    // flash to show new log file created on sd card
    if (flashOnLog) {
        digitalWrite(brightLED,HIGH);  // turn flash on
        delay(20);
        digitalWrite(brightLED,LOW);  // turn flash off
    }
        
      fs::FS &fs = SD_MMC;                 // sd card file system
      templogCounter = 0;                  // reset temp log counter     
      logCounter++;                        // increment log file name number
      String logFileName = "/log/log" + String(logCounter) + ".txt";
      File logFile = fs.open(logFileName, FILE_WRITE);
      if (logFile) {
        // store entries in temp log in to new text file on sd card
        for (int x=0; x < entriesPerLog; x++) {
          logFile.print(String(logsDist[x]));
          logFile.print(",");
          logFile.println(String(logsTime[x]));
        }
        logFile.close();
      } 
      else {
        // error creating new log file
        Serial.println("Error creating log file: " + logFileName);
        flashLED(7);
      }
  }

  
}


// ******************************************************************************************************************
// end
