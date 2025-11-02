/**
 * DHT22 Temperature and Humidity Sensor Driver
 * 
 * This driver implements the DHT22 (AM2302) communication protocol
 * for reading temperature and humidity values.
 * 
 * DHT22 Specifications:
 * - Temperature: -40 to 80°C (±0.5°C accuracy)
 * - Humidity: 0 to 100% RH (±2-5% accuracy)
 * - Resolution: 0.1°C, 0.1% RH
 * - Communication: Single-wire digital signal
 * 
 * Wiring:
 * - VCC: 3.3V or 5V
 * - GND: Ground
 * - DATA: GPIO pin (with 10kΩ pull-up resistor)
 * 
 * Usage:
 *   dht22_init(pin_number);
 *   float temp, hum;
 *   if (dht22_read(&temp, &hum) == DHT22_OK) {
 *       printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temp, hum);
 *   }
 */

#ifndef DHT22_H
#define DHT22_H

#include "pico/stdlib.h"
#include <stdbool.h>

// Return codes
typedef enum {
    DHT22_OK = 0,           // Reading successful
    DHT22_ERROR_TIMEOUT,    // Timeout during communication
    DHT22_ERROR_CHECKSUM,   // Checksum mismatch
    DHT22_ERROR_NO_RESPONSE // No response from sensor
} dht22_result_t;

/**
 * Initialize DHT22 sensor
 * @param pin GPIO pin connected to DHT22 data line
 */
void dht22_init(uint pin);

/**
 * Read temperature and humidity from DHT22
 * @param temperature Pointer to store temperature (°C)
 * @param humidity Pointer to store humidity (%RH)
 * @return DHT22_OK on success, error code on failure
 */
dht22_result_t dht22_read(float *temperature, float *humidity);

/**
 * Get string description of error code
 * @param result Error code from dht22_read()
 * @return Human-readable error description
 */
const char* dht22_error_string(dht22_result_t result);

#endif // DHT22_H