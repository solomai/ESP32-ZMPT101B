/*
 * ZMPT101B Sensor Example
 *
 * This example code is released into the Public Domain (or is licensed under CC0, at your option).
 *
 * Unless required by applicable law or agreed upon in writing,
 * this software is provided "AS IS", without any warranties or conditions
 * of any kind, either express or implied.
 */

// include freertos lib
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
// include components
#include "zmpt101b.h"

#define TAG "EXAMPLE_FOR_ZMPT101B_SENSOR"

// Define the GPIO pin number for the LED used for blinking
#define BLINK_GPIO         GPIO_NUM_2

// Select ADC channel (assuming using ADC1 channel 0 GPIO36 "VP")
#define ZMPT101B_SENSOR_ADC_CHANNEL ADC1_CHANNEL_0

// Define the GPIO level for turning the LED on
#define LED_ON  1

// Define the GPIO level for turning the LED off
#define LED_OFF 0

// Duration in milliseconds for LED blink
#define LED_BLINK_DURATION 1000

// Interval in milliseconds for LED blink
#define LED_BLINK_INTERVAL 500

// Interval in milliseconds for reinitializing the sensor after a failure
#define SENSOR_INIT_INTERVAL 10000

// Interval in milliseconds to read data from sensor
#define SENSOR_READ_INTERVAL 5000

void app_main(void)
{
    // Init blink LED
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << BLINK_GPIO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // Initialize the ZMPT101B sensor.
    esp_err_t sensor_err = ESP_ERR_NOT_FOUND;
    do{
        sensor_err = zmpt101b_init(ZMPT101B_SENSOR_ADC_CHANNEL);
        if (sensor_err!=ESP_OK){
            ESP_LOGW(TAG, "timeout %dmsec before retry initializatin ZMPT101B sensor", SENSOR_INIT_INTERVAL );
            vTaskDelay(pdMS_TO_TICKS(SENSOR_INIT_INTERVAL));
        }
    }while(sensor_err!=ESP_OK);

    // Infinite loop to continuously fetch data from ZMPT101B sensor
    while (1) {
        gpio_set_level(BLINK_GPIO, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DURATION));

        uint16_t voltage = 0.0;
         ESP_ERROR_CHECK(zmpt101b_read_voltage(ZMPT101B_SENSOR_ADC_CHANNEL, &voltage ));
        printf("ZMPT101B return voltage = %dV\n", voltage);

        gpio_set_level(BLINK_GPIO, LED_OFF);
        // Wait for the next iteration
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL));
    }
}
