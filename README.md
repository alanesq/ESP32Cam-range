<p align="center"><img src="/images/esp32cam-range.jpg" width="90%"/></p>

Device to record an image and distance of overtaking vehicles using an ESP32cam module with JSN-SR04T ultrasonic distance sensor using Arduino IDE.

The idea is to attach the sensor to the right side of a cycle and when any vehicles passes it will record to an sd card the distance the vehicle 
was from the cycle along with a image of the passing vehicle.  

It takes a distance reading several times a second, if a reading of less than a pre-defined distance is received then an image is captured via
the camera with the file name including the measured distance.

This is curently a work in progress so is pretty basic at the moment but is now in a working state and ready to be mounted on my cycle for testing.
If it proves to work as I hope the idea is that this will be a very low cost, easy to build way to collect some data on how close overtaking vehicles actually tend to get to cyclists.

    Unit              Cost
    ----              ----
    ESP32Cam module   5ukp
    Distance sensor   6ukp
    SD Card under     10ukp
    USB power bank    1ukp (Poundland)
    
  
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
        put a level shifter between the devices on pins 12 and 13.  Mine seems to be working ok directly connected so far.
 

When the module boots it should give a quick flash on the white flash LED to show all is ok, there is a small red led on the rear of the
module which should flash every couple of seconds to show the sketch is running ok.

The small led will also flash to show error states:

         1 = no sd card found
         2 = invalid format sd card found
         3 = failed to connect to camera
         4 = failed to capture image from camera
         5 = failed to store image to sd card
         constant = waiting for object to clear


