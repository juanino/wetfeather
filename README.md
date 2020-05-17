# wetfeather

Despite it's name, wifitest.ino is the main file. To run:
* Buy an esp8266 board, mine is an adafruit feather huzzah esp8266
* Get a water sensor.  Mine is a plate with 2 wires running close to each other
* Wire 3.3v to one of the plate wires
* Wire pin 14 (or any gpio digital pin) to the other wire of the plate
* Use a pulldown resistor to pull pin 14 to ground (10k or higher)
* Configure and IFTTT webhook
* Copy config.h_sample to config.h and adjust wifi settings and ifttt key
* Upload to the board

# notes
* pin0 red led on the feather will light up if sensor is wire (wires connected)
* pin2 blue led on the feather will go solid while looking for wifi
* pin2 blue will also flash every few hundred cycles so you know it is looping still
* I've been told a higher resistor like 1meg will help the accuracy but 10k worked fine for me
