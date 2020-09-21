<p align="center"><img src="/images/esp32cam-range.jpg" width="90%"/></p>

Device to record an image and distance of overtaking vehicles using an ESP32cam module with JSN-SR04T ultrasonic distance sensor using Arduino IDE.

The idea is to attach the sensor to the right side of a cycle and when any vehicles passes it will record to an sd card the distance the vehicle 
was from the cycle along with a image of the passing vehicle.  

It takes a distance reading several times a second, if a reading of less than a pre-defined distance is received then an image is captured via
the camera and the file name includes the distance measured.

This is curently a work in progress so is pretty basic at the moment.
If it proves to work as I hope the idea is that this will be a very low cost way to collect some data on how close overtaking vehicles actually tend to get to cyclists.
Cost to build:
  ESP32Cam module 5ukp
  Distance sensor 6ukp
  SD Card under 10ukp
 



