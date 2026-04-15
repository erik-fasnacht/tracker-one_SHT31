#pragma once

/* sht3x-i2c library by Mariano Goluboff
 */

// This will load the definition for common Particle variable types
#include "Particle.h"

#define SHT31_STATE_IDLE            0x00
#define SHT31_STATE_CONTINUOUS      0x01

#define SHT31_ACCURACY_HIGH         0x00
#define SHT31_ACCURACY_MEDIUM       0x01
#define SHT31_ACCURACY_LOW          0x02

#define SHT31_I2C_ERROR             -0x01
#define SHT31_I2C_LOCK_ERROR        -0x02
#define SHT31_CRC_ERROR             -0x03
#define SHT31_WRONG_MODE            -0x04

// This is your main class that users will import into their application
class Sht3xi2c
{
public:
  /**
   * Constructor
   */
  Sht3xi2c(TwoWire& interface, uint8_t i2c_addr = 0x44);

  void begin(uint32_t speed = CLOCK_SPEED_400KHZ);
  /**
   * @brief Have sensor make a reading and return the values
   * 
   * @param temp A pointer to a double that will be populated with the temperature
   * @param humid A pointer to a double that will be populated with the humidity
   * @param accuracy One of SHT31_ACCURACY_HIGH, SHT31_ACCURACY_MEDIUM, or SHT31_ACCURACY_LOW
   * @param i2c_addr I2C address of the device (0x44 or 0x45)
   * @param clock_stretching Whether to use clock stretching or not
   * 
   * @retval 0 if success
   */
  int single_shot(double* temp, double* humid, uint8_t accuracy = SHT31_ACCURACY_MEDIUM);

  /**
   * @brief Start periodic temperature and humidity measurements
   * 
   * @param accuracy One of SHT31_ACCURACY_HIGH, SHT31_ACCURACY_MEDIUM, or SHT31_ACCURACY_LOW
   * @param mps Measurements per second: 10, 4, 2, 1, or 0 (0 is for 0.5)
   * @param i2c_addr I2C address of the device (0x44 or 0x45)
   * 
   * @retval 0 if success
   */
  int start_periodic(uint8_t accuracy = SHT31_ACCURACY_MEDIUM, uint8_t mps = 0);
  
  /**
   * @brief Stop periodic temperature and humidity measurements
   * 
   * @param i2c_addr I2C address of the device (0x44 or 0x45)
   * 
   * @retval 0 if success
   */
  bool stop_periodic();

  /**
   * @brief Get the reading from periodic reads. Do not run more often than the mps
   * 
   * @param temp A pointer to a double that will be populated with the temperature
   * @param humid A pointer to a double that will be populated with the humidity
   * @param i2c_addr I2C address of the device (0x44 or 0x45)
   * 
   * @retval 0 if success, SHT31_WRONG_MODE if sensor is not in periodic mode
   */
  int get_reading(double* temp, double* humid);

private:
  TwoWire* _wire;
  uint8_t _state, _i2c_addr;
  int pr_get_reading(double* temp, double* humid);
};
