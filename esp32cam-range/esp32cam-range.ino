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
 *        constant = Waiting for captured object to clear
 *            
 *            
 *******************************************************************************************************************/


// ---------------------------------------------------------------
//                          -SETTINGS
// ---------------------------------------------------------------

  const String stitle = "ESP32Cam-range";                // title of this sketch

  const String sversion = "22Sep20";                     // Sketch version
  
  int triggerDistance = 150;                             // distance below which will trigger an image capture in cm

  int numberReadings = 3;                                // number of readings to take from distance sensor to average together for final reading

  int minTimeBetweenTriggers = 2;                        // minimum time between triggers in seconds

  const int TimeBetweenStatus = 2;                       // time between status light flashes

  bool flashRequired = 1;                                // If flash to be used when capturing image (1 = yes)

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
  int imageCounter;                         // counter for file names on sdcard
  uint32_t lastTrigger = millis();          // last time camera was triggered
  uint32_t lastStatus = millis();           // last time status light flashed

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
        flashLED(3); 
      }

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

  // discover number of files stored on sd card and set file counter accordingly
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

  // Turn-off the 'brownout detector'
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

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
    int tdist = readDistance();    // read distance in cm
    String textDist = "Unk";       // distance reading as a string
    if (tdist > 0) textDist = String(tdist) + "cm";
    Serial.println("Distance = " + textDist);

  // if distance is less than the trigger distance
  if (tdist < triggerDistance && tdist > 0) {
    // if long enough since last trigger store image
      if ((unsigned long)(millis() - lastTrigger) >= (minTimeBetweenTriggers * 1000)) { 
        lastTrigger = millis();                     // reset timer
        digitalWrite(indicatorLED,LOW);             // indicator led on
        storeImage(textDist);                       // capture and store a live image
      }
    // wait for object to pass
      digitalWrite(indicatorLED,LOW);               // indicator led on
      while (readDistance() < triggerDistance) {
        delay(200);
      }
      digitalWrite(indicatorLED,HIGH);              // indicator led off
  }

  // flash status light to show sketch is running
    if ((unsigned long)(millis() - lastStatus) >= (TimeBetweenStatus * 1000)) { 
      lastStatus = millis();              // reset timer
      digitalWrite(indicatorLED,LOW);     // led on
    }
    delay(80);
    digitalWrite(indicatorLED,HIGH);     // led off
}


// ******************************************************************************************************************


// Read distance from ultrasound sensor - reads several times and take average
//    returns result in centimeters

int readDistance() {

  int avrgDist = 0;                         // average of several readings
  long duration =0;                         // duration of returning echo
  int distance = 0;                         // calculated distance in cm
  int noZeros = 0;                          // number of zero results (i.e. no return signal received)

  for (int x=0; x < numberReadings; x++) {
  
    // Clear the trigPin by setting it LOW:
      digitalWrite(trigPin, LOW);
      delayMicroseconds(5);
    
   // Trigger the sensor by setting the trigPin high for 10 microseconds:
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH, 25000);      // Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
    
    distance = duration*0.034/2;                   // Calculate the distance in cm

    avrgDist += distance;                          // add to running total

    if (distance == 0) noZeros++;                  // increment zero result counter (so it can be ignored)
  }

  // calculate average reading
  distance = 0;
  if ((numberReadings - noZeros) > 0) {                      // avoid divide by zero
    distance = avrgDist / ( numberReadings - noZeros);       // calculate average
  }

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


// ******************************************************************************************************************


// save image to sd card
//    iTitle = add to end of title of file


void storeImage(String iTitle) {

  Serial.println("Storing image #" + String(imageCounter) + " to sd card");

  fs::FS &fs = SD_MMC; 

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
    int tdist = readDistance();    // distance in cm
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
// end
