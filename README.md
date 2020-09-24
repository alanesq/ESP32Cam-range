<p align="center"><img src="/images/esp32cam-range.jpg" width="90%"/></p>

Device to record an image and distance of overtaking vehicles using an ESP32cam module with JSN-SR04T ultrasonic distance sensor using Arduino IDE.

The idea is to attach the sensor to the right side of a cycle and when any vehicles passes it will record to an sd card the distance the vehicle 
was from the cycle along with a image of the passing vehicle.  

It takes a distance reading several times a second, if a reading of less than a pre-defined distance is received then an image is captured via
the camera with the file name including the measured distance.
It also keeps a continual log of readins in text files in the /log folder on the sd card.  These files consist of distance read in cm and time 
recorded(in milliseconds)

It will also continually record distance data to text files which can then be imported in to a spreadsheet and graphed etc.
Note: if you have image recording enabled it takes around 3/4 second to capture and store an image during which distance data can not be captured.

See the "settings" section at the top of the code to enable/disable/configure features.

If it proves to work as I hope the idea is that this will be a very low cost, easy to build way to collect some data on how close overtaking vehicles actually tend to get to cyclists.

    Unit                    Cost
    ----                    ----
    ESP32Cam module         5ukp
    In circuit programmer   2ukp        As the esp32 module does not have it built in
    Distance sensor         5ukp
    USB power bank          1ukp        e.g. from Poundland
    Misc wires              1ukp
    It also requires an sd card to store the data on and a case of some kind.
  
Build details:
  The esp32cam module requires the code installing which if you are not experienced with Arduino IDE can be a challenge but apart from this
  it is a very simple build.  The esp32cam comes with an sd card reader built in so all it requies is 4 wires connecting it to the ultrasound sensor.
  
      Sensor    ESP32Cam
      ------    --------
      5v        5v
      GND       GND
      Trigger   13
      Echo      12
      
  Insert a sd card, connect a USB power bank to the 5v/GND on the esp32cam module and it is ready to be installed in a case and used.
  
  Note: There may be an issue with the esp32 being 3.3v logic and the sensor 5v, it seems to be working ok but you may prefer to
        put a level shifter (or resistor) between the devices on pins 12 and 13.  Mine seems to be working ok directly connected so far.
        Newer versions of the sensor (v2.0) claims to work at either voltage but I have seen several reports of problems when trying
        to use them at 3.3v.
 

When the module boots it should give a quick flash on the white flash LED to show all is ok, there is a small red led on the rear of the
module which should flash every couple of seconds to show the sketch is running ok or show error.

The small led will also flash to show error states:

         1 = no sd card found
         2 = invalid format sd card found
         3 = failed to connect to camera
         4 = failed to capture image from camera
         5 = failed to store image to sd card
         6 = log file problem
         constant = waiting for object to clear distance sensor


