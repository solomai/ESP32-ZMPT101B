/*
 * ZMPT101B Sensor Interface Component
 *
 * This component provides functionality for interfacing with the ZMPT101B sensor.
 * It includes functions to initialize the sensor, read current Voltage,
 * and handle any errors that may occur during sensor communication.
 *
 * Attention:
 * - The sensors are not factory-calibrated.
 * - You will need to calibrate them yourself using the adjustment potentiometer on the board.
 * - A voltmeter is required for calibration; the more accurate the voltmeter, the better.
 * - An oscilloscope can also be used for more precise calibration and analysis.
 *
 * Dependencies:
 * - ESP-IDF (Espressif IoT Development Framework)
 * - Necessary drivers and libraries for GPIO communication
 *
 * License:
 * This component is released under the MIT License. See the LICENSE file for details.
 * You are free to use, modify, and distribute this code as long as the above license is included in all copies or substantial portions of the code.
 *
 * Disclaimer:
 * This component is provided "as is", without warranty of any kind. The authors are not liable for any damages arising from the use of this component.
 * Use it at your own risk.
 *
 * Author: Andrii Solomai
 * Date: Sep 2024
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"


#define TAG_ZMPT101B "ZMPT101B_SENSOR"

// Enable to output additional debug information
// #define DEBUG_EXTRA_INFO

/*
 * Calibration data for the ZMPT101B sensor.
 */

// Define the ADC resolution (width in bits) for analog-to-digital conversion.
// Using a 12-bit width (ADC_WIDTH_BIT_12) provides a higher resolution (0-4095 range),
// which allows for more precise voltage readings from the ZMPT101B sensor.
// This is important for accurate calibration and measurement of voltage levels.
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12

// Define the attenuation level for the ADC input.
// Using a 12 dB attenuation (ADC_ATTEN_DB_12) allows the ADC to measure a wider voltage range,
// specifically from 0 to ~3.9V, instead of the default 0 to ~1.1V.
// This setting is suitable for reading higher voltage levels from the ZMPT101B sensor
// without saturating the ADC and helps in achieving more accurate voltage measurements.
#define ADC_ATTEN_DB  ADC_ATTEN_DB_12

// ADC unit to be used for voltage measurements (ADC Unit 1)
#define ADC_UNIT ADC_UNIT_1

// Default reference voltage (Vref) for ADC calibration, in millivolts (mV)
#define DEFAULT_VREF 1100  // in mV

// I2S Configuration
// Sampling frequency for collecting voltage data from the ADC using I2S
#define SAMPLING_FREQ 25000  // in Hz

// Maximum length of the DMA buffer for I2S data transfer
#define DMA_BUFFER_LEN 1024  // in bytes

// I2S bit resolution for each sample (16-bit per sample)
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT

// I2S peripheral number to be used for ADC data acquisition
#define ADC_I2S_NUM I2S_NUM_0

// Size of the internal buffer for I2S ADC readings. The DMA buffer is initially sized for 8-bit data,
// but since we are using a 12-bit ADC, we need to repack the 1-byte DMA buffer into a 2-byte buffer.
// Therefore, we divide the DMA buffer length by the size of a uint16_t to accommodate the 12-bit ADC data
// and then multiply by 2 to allocate sufficient space for 1024 samples ( 40ms per 25kHz sampling ).
// Note that this value depends on the ADC_WIDTH_BIT setting.
#define I2S_READ_BUFFER_16B ( DMA_BUFFER_LEN / sizeof(uint16_t) ) * 2

/*
 * Public APIs
 */

/**
 * @brief Initializes the ADC for the specified ADC channel for the ZMPT101B voltage sensor.
 *
 * This function configures the ADC to read data from the specified channel where the ZMPT101B sensor is connected.
 *
 * @param adc_channel ADC channel to configure for the ZMPT101B sensor.
 * @return esp_err_t Error code indicating success (ESP_OK) or failure (appropriate ESP-IDF error code).
 */
esp_err_t zmpt101b_init(adc_channel_t adc_channel);

/**
 * @brief Reads the RMS voltage from the ZMPT101B sensor.
 *
 * This function reads the current RMS voltage from the ZMPT101B sensor connected to the specified ADC channel.
 *
 * @param adc_channel ADC channel where the ZMPT101B sensor is connected.
 * @param rmsVoltage Pointer to a variable where the measured RMS voltage value will be stored.
 * @return esp_err_t Error code indicating success (ESP_OK) or failure (appropriate ESP-IDF error code).
 */
esp_err_t zmpt101b_read_voltage(adc_channel_t adc_channel, uint16_t *rmsVoltage);
