// Example usage for sht3x-i2c library by Mariano Goluboff.

#include "sht3x-i2c.h"

// Initialize objects from the lib
Sht3xi2c sht3xi2c;

void setup() {
    // Call functions on initialized library objects that require hardware
    sht3xi2c.begin();
}

void loop() {
    // Use the library's initialized objects and functions
    sht3xi2c.process();
}
