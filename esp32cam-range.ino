 /*******************************************************************************************************************
 *            
 *     Cycling close pass distance recorder using 
 *     an ESP32Cam module along with a JSN-SR04T ultrasonic distance sensor 
 *     
 *     
 *     - created using the Arduino IDE with ESP32 module installed   (https://dl.espressif.com/dl/package_esp32_index.json)
 *       No additional libraries required
 * 
 * 
 *     Four wires connect the esp32 to the ultrasonic sensor:
 *          5v to 5v
 *          GND to GND
 *          13 to Trigger
 *          12 to Echo
 *     
 *     
 *     Status led flashes:
 *        1 = no sd card found
 *        2 = invalid format sd card found
 *        3 = failed to start camera
 *        4 = failed to capture image from camera
 *        5 = failed to store image to sd card
 *        6 = failed to create log file
 *        constant = Waiting for close object to clear
 *            
 *            
 *******************************************************************************************************************/


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------

  const String stitle = "ESP32Cam-range";                // title of this sketch

  const String sversion = "23Sep20";                     // Sketch version
  
  const bool camEnable = 1;                              // Enable storing of images (1=enable)
  const int triggerDistance = 150;                       // distance below which will trigger an image capture in cm

  const bool logEnable = 1;                              // Store constant log of distance readings (1=enable)
  const bool flashOnLog = 1;                             // if to flash the main LED when log is written to sd card
  const int logFrequency = 35;                           // how often to add distance to continual log file (milliseconds, 1000 = 1 second)
  const int entriesPerLog = 250;                         // how many entries to store in each log file

  const int numberReadings = 1;                          // number of readings to take from distance sensor to average together for final reading

  const int minTimeBetweenTriggers = 5;                  // minimum time between triggers in seconds

  const int TimeBetweenStatus = 400;                     // time between status light flashes (milliseconds)

  const bool flashRequired = 1;                          // If flash to be used when capturing image (1 = yes)

  const int echoTimeout = (5882 * 5.5);                  // timeout if no echo received from distance sensor (5882 ~ 1m)

  const int indicatorLED = 33;                           // onboard status LED pin (33)

  const int brightLED = 4;                               // onboard flash LED pin (4)

  
// ---------------------------------------------------------------

#include "camera.h"                         // insert camera related code 

#include "soc/soc.h"                        // Used to disable brownout problems
#include "soc/rtc_cntl_reg.h"      
  
// Define ultrasonic range sensor pins (jsn-sr04t)     13/12
  const int trigPin = 13;
  const int echoPin = 12;

// Define general variables:
  int logsDist[entriesPerLog];              // log file temp store distance
  uint32_t logsTime[entriesPerLog];         // log file temp store time recorded
  int templogCounter = 0;                   // counter for temp log store
  int imageCounter;                         // counter for image file names on sdcard
  int logCounter;                           // counter for log file names on sdcard
  uint32_t lastTrigger = millis();          // last time camera was triggered
  uint32_t lastStatus = millis();           // last time status light flashed
  uint32_t logTimer = millis();             // last time log file was updated

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
        showError(3);       // critical error so stop and flash led
      }

  // start sd card
      if (!SD_MMC.begin("/sdcard", true)) {         // if loading sd card fails     
        // note: ("/sdcard", true) = 1 wire - see: https://www.reddit.com/r/esp32/comments/d71es9/a_breakdown_of_my_experience_trying_to_talk_to_an/
        Serial.println("SD Card not found"); 
        showError(1);       // critical error so stop and flash led
      }
      uint8_t cardType = SD_MMC.cardType();
      if (cardType == CARD_NONE) {                 // if no sd card found
          Serial.println("SD Card type detect failed"); 
          showError(2);       // critical error so stop and flash led
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

  // redefine io pins as sd card seems to upset them
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

  // Read distance from sensor
    int tdist = readDistance(numberReadings);    // read distance in cm
    String textDist = "Unk";                     // distance reading as a string
    if (tdist > 0) textDist = String(tdist) + "cm";
    Serial.println("Distance = " + textDist);

  // add distance to continual logs
  if ((unsigned long)(millis() - logTimer) >= logFrequency) { 
    logTimer = millis();        // reset log timer
    logDistance(tdist);         // add entry to logs
  }

  // if distance is less than the trigger distance and recording of images is enabled
  if ( (tdist < triggerDistance && tdist > 0)  && camEnable) {
    // if long enough since last trigger store image
      if ((unsigned long)(millis() - lastTrigger) >= (minTimeBetweenTriggers * 1000)) { 
        lastTrigger = millis();                     // reset timer
        digitalWrite(indicatorLED,LOW);             // indicator led on
        storeImage(textDist);                       // capture and store a live image
      }
    // wait for object to pass
      digitalWrite(indicatorLED,LOW);               // indicator led on
      while (readDistance(numberReadings) < triggerDistance) {
        delay(200);
      }
      digitalWrite(indicatorLED,HIGH);              // indicator led off
  }

  // flash status light to show sketch is running
    if ((unsigned long)(millis() - lastStatus) >= TimeBetweenStatus) { 
      lastStatus = millis();              // reset timer
      digitalWrite(indicatorLED,!digitalRead(indicatorLED));     // led status change
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
    
    duration = pulseIn(echoPin, HIGH, echoTimeout);      // Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:

    if (duration == 0) noReadingCounter++;               // increment failed attempt counter
    else distance += duration*0.034/2;                   // Calculate the distance in cm and add to running total
    
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
    
  if (!camEnable) return;                        // if logs disabled do nothing

  Serial.println("Storing image #" + String(imageCounter) + " to sd card");

  fs::FS &fs = SD_MMC;           // sd card file system

  // capture live image from camera
  cameraImageSettings();                            // apply camera sensor settings
  if (flashRequired) digitalWrite(brightLED,HIGH);  // turn flash on
  camera_fb_t *fb = esp_camera_fb_get();            // capture frame from camera
  digitalWrite(brightLED,LOW);                      // turn flash off
  if (!fb) {
    Serial.println("Camera capture failed");
    flashLED(4);
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
      flashLED(5);
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

  if (!logEnable) return;                        // if logs disabled do nothing
  
    if (distToLog == 0) distToLog = 600;         // if no return signal was received set it to max distance

    logsDist[templogCounter] = distToLog;        // add current distance entry to temp log
    logsTime[templogCounter] = millis();         // add current time to temp log
    templogCounter++;                            // increment temp log store counter
        
    if (templogCounter == entriesPerLog) {     
      // store to temp log entries to sd card
        Serial.println("Writing log to sd card");
        
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
        flashLED(6);
      }
  }

  
}


// ******************************************************************************************************************
// end
