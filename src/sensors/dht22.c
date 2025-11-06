/**
 * DHT22 Temperature and Humidity Sensor Driver - VERSÃO FUNCIONAL
 * 
 * Versão corrigida sem controle de interrupções que causava travamentos.
 * Baseada no debug que identificou que save_and_disable_interrupts() 
 * estava causando o problema.
 */

#include "dht22.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>

// DHT22 Protocol Timing (in microseconds)
#define DHT22_START_SIGNAL_LOW  1000    // Host pulls low for 1ms
#define DHT22_START_SIGNAL_HIGH 30      // Host releases for 30µs
#define DHT22_RESPONSE_LOW      80      // DHT22 responds low for 80µs
#define DHT22_RESPONSE_HIGH     80      // DHT22 responds high for 80µs
#define DHT22_BIT_LOW           50      // Each bit starts low for 50µs
#define DHT22_BIT0_HIGH         26      // Bit '0' is high for 26-28µs
#define DHT22_BIT1_HIGH         70      // Bit '1' is high for 70µs

#define DHT22_TIMEOUT_US        5000    // Timeout mais conservativo (5ms)
#define DHT22_DATA_BITS         40      // Total data bits (5 bytes)

static uint dht22_pin = 0;

/**
 * Wait for pin to reach specified state with timeout - VERSÃO FUNCIONAL
 */
static bool wait_for_pin(bool state, uint32_t timeout_us) {
    // Timeout baseado em loops em vez de tempo absoluto (mais simples)
    uint32_t max_loops = timeout_us / 5; // ~5us por loop
    if (max_loops > 20000) max_loops = 20000; // Limite máximo
    
    for (uint32_t i = 0; i < max_loops; i++) {
        if (gpio_get(dht22_pin) == state) {
            return true; // Sucesso
        }
        sleep_us(5);
    }
    
    return false; // Timeout
}

/**
 * Measure pulse duration - VERSÃO SIMPLIFICADA
 */
static uint32_t measure_pulse_us(bool state, uint32_t timeout_us) {
    // Aguardar início do pulso
    if (!wait_for_pin(state, timeout_us)) {
        return 0; // Timeout
    }
    
    // Medir duração
    uint32_t count = 0;
    uint32_t max_count = timeout_us / 5;
    
    while (gpio_get(dht22_pin) == state && count < max_count) {
        sleep_us(5);
        count++;
    }
    
    return count * 5; // Aproximação em microsegundos
}

void dht22_init(uint pin) {
    dht22_pin = pin;
    
    // Inicializar GPIO
    gpio_init(dht22_pin);
    gpio_set_dir(dht22_pin, GPIO_IN);
    gpio_pull_up(dht22_pin); // Enable internal pull-up
    
    // Aguardar estabilização
    sleep_ms(10);
}

dht22_result_t dht22_read(float *temperature, float *humidity) {
    uint8_t data[5] = {0}; // 40 bits = 5 bytes
    
    // Step 1: Send start signal
    gpio_set_dir(dht22_pin, GPIO_OUT);
    gpio_put(dht22_pin, 0);                    // Pull low
    sleep_us(DHT22_START_SIGNAL_LOW);          // Wait 1ms
    gpio_put(dht22_pin, 1);                    // Release (pull-up takes over)
    sleep_us(DHT22_START_SIGNAL_HIGH);         // Wait 30µs
    gpio_set_dir(dht22_pin, GPIO_IN);          // Switch to input
    
    // Step 2: Wait for DHT22 response
    if (!wait_for_pin(0, DHT22_TIMEOUT_US)) {
        return DHT22_ERROR_NO_RESPONSE;
    }
    
    if (!wait_for_pin(1, DHT22_TIMEOUT_US)) {
        return DHT22_ERROR_NO_RESPONSE;
    }
    
    if (!wait_for_pin(0, DHT22_TIMEOUT_US)) {
        return DHT22_ERROR_NO_RESPONSE;
    }
    
    // Step 3: Read 40 data bits
    for (int i = 0; i < DHT22_DATA_BITS; i++) {
        // Wait for bit start (low period)
        if (!wait_for_pin(1, DHT22_TIMEOUT_US)) {
            return DHT22_ERROR_TIMEOUT;
        }
        
        // Measure high period duration
        uint32_t pulse_duration = measure_pulse_us(1, DHT22_TIMEOUT_US);
        if (pulse_duration == 0) {
            return DHT22_ERROR_TIMEOUT;
        }
        
        // Determine if bit is 0 or 1
        int byte_index = i / 8;
        int bit_index = 7 - (i % 8);
        
        if (pulse_duration > 40) { // Threshold between 0 and 1
            data[byte_index] |= (1 << bit_index); // Set bit to 1
        }
    }
    
    // Step 4: Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return DHT22_ERROR_CHECKSUM;
    }
    
    // Step 5: Convert data to temperature and humidity
    // Humidity: data[0] and data[1] (16-bit, 0.1% resolution)
    uint16_t humidity_raw = (data[0] << 8) | data[1];
    *humidity = humidity_raw / 10.0f;
    
    // Temperature: data[2] and data[3] (16-bit, 0.1°C resolution)
    uint16_t temperature_raw = (data[2] << 8) | data[3];
    
    // Check for negative temperature (MSB of data[2])
    if (data[2] & 0x80) {
        temperature_raw &= 0x7FFF; // Remove sign bit
        *temperature = -(temperature_raw / 10.0f);
    } else {
        *temperature = temperature_raw / 10.0f;
    }
    
    return DHT22_OK;
}

const char* dht22_error_string(dht22_result_t result) {
    switch (result) {
        case DHT22_OK:
            return "Success";
        case DHT22_ERROR_TIMEOUT:
            return "Communication timeout";
        case DHT22_ERROR_CHECKSUM:
            return "Checksum error";
        case DHT22_ERROR_NO_RESPONSE:
            return "No response from sensor";
        default:
            return "Unknown error";
    }
}