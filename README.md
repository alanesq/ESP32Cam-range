<p align="center"><img src="/images/esp32cam-range.jpg" width="90%"/></p>

Device to record an image and distance of overtaking vehicles using an ESP32cam module with JSN-SR04T ultrasonic distance sensor using Arduino IDE.
Although this sketch will probably be a good starting point for any project where you wish to save images to an sd card based on some input/sensor.


The idea is to attach the sensor to the right side of a cycle and when any vehicles passes it will record to an sd card the distance the vehicle 
was from the cycle along with a image of the passing vehicle.  

It takes a distance reading several times a second, if a reading of less than a pre-defined distance is received then an image is captured via
the camera with the file name including the measured distance.
It also keeps a continual log of readings in text files in the /log folder on the sd card.  These files consist of distance read in cm and time 
recorded(in milliseconds).  You can see an example of this in the Images folder.

It will also continually record distance data to text files which can then be imported in to a spreadsheet and graphed etc.
Note: if you have image recording enabled it takes around 3/4 second to capture and store an image during which distance data can not be captured.

See the "settings" section at the top of the code to enable/disable/configure features.

Even if you do not plan to use the camera I think the esp32cam module is a good choice as it is cheap and has the sd card reader built in, you do 
need to have an in circuit programmer though as it does not have a usb socket onboard.
Lots of info on the esp32cam board here: https://randomnerdtutorials.com/esp32-cam-video-streaming-face-recognition-arduino-ide/

If it proves to work as I hope the idea is that this will be a very low cost, easy to build way to collect some data on how close overtaking vehicles actually tend to get to cyclists.

    Unit                    Cost
    ----                    ----
    ESP32Cam module         5ukp
    In circuit programmer   2ukp        I use this type: https://makeradvisor.com/tools/ftdi-programmer-board/
    Distance sensor         5ukp
    USB power bank          1ukp        e.g. from Poundland
    Misc wires              1ukp
    It also requires an sd card to store the data on and a case of some kind.
  
Build details:
  The esp32cam module requires the code installing which if you are not experienced with Arduino IDE can be a bit of a challenge but apart from this
  it is a very simple build.  The esp32cam comes with an sd card reader built in so all it requies is 4 wires connecting it to the ultrasound sensor.
  
      Sensor    ESP32Cam
      ------    --------
      5v        5v
      GND       GND
      Trigger   13
      Echo      12
      
  Insert a formatted sd card, connect 5volts (e.g. a USB power bank) to the 5v/GND on the esp32cam module and it is ready to be installed in a case and used.
  Note: It tends to require a good 5volt supply, if you get strange errors/behaviour this is the first thing to suspect
  
When the module boots it should give a quick flash on the white flash LED to show all is ok, there is a small red led on the rear of the
module which should flash continually to show the sketch is running ok or show errors.

        Flashes     Meaning
        -------     -------
         2          no sd card found
         3          invalid format sd card found
         4          failed to connect to camera
         5          failed to capture image from camera
         6          failed to store image to sd card
         7          log file problem
         constant   waiting for object to clear distance sensor


Misc Notes
----------

The fastest it is able to capture a stream of data is around 85 readings a second (12ms)

There may be an issue with the esp32 being 3.3v logic and the sensor 5v, it seems to be working ok for me so far but you may prefer to
    put a level shifter (or resistor) between the devices on pins 12 and 13.
    Newer versions of the sensor (v2.0) claims to work at either voltage but I have seen several reports of problems when trying
    to use them at 3.3v.
    
The minimum distance the sensor will read is around 20cm, I am finding 2.5m about the max distance although it claims to be 4.5?
    If the sensor is mounted level with the cycle frame then the side of the cyclist will be around 20cm from this which
    works out pretty well as this is the minimum distance it will be able to measure.  
    Note: You will need to subtract 20cm from the readings to give distance between vehicle and cyclist

Using the esp32cam module means that there are no free io pins so if you wish to add lcd display, more inputs etc. then you will
    need to look at a different development board.  Or it may be possible to wire directly to the esp32-s module its self
    Possible io pins to consider:    16, 17, 9, 10, 11, 6, 7, 8    
    
    

