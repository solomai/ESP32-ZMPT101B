#include <math.h>
#include <string.h>
#include "zmpt101b.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_adc_cal.h"
#include "driver/i2s.h"

// Internal functions
// Applies a median filter to the entire array in-place, including edge cases.
// This function modifies the input array `data` directly and calculates the
// minimum and maximum values within the filtered data. It handles edge cases
// by adjusting the window size near the boundaries.
bool median_filter_in_place(uint16_t *data, size_t length, size_t window_size, uint16_t *min_value, uint16_t *max_value)
{
    // Validate window size
    if (window_size > length) {
        ESP_LOGE(TAG_ZMPT101B, "%s Invalid window size\n", __FUNCTION__ );
        return false;
    }

    // Ensure window size is odd for a proper median calculation
    if (window_size % 2 == 0)
        window_size++;

    uint16_t *window = (uint16_t*)malloc(window_size * sizeof(uint16_t));
    if (window == NULL) {
        ESP_LOGE(TAG_ZMPT101B, "%s Failed to allocate memory for window\n", __FUNCTION__ );
        return false;
    }

    *min_value = 0xFFFFu;
    *max_value = !*min_value;
    const size_t half_window = window_size / 2;
    for (size_t i = 0; i < length; ++i) {
        // Determine the actual window size for edge cases
        const size_t start = (i < half_window) ? 0 : i - half_window;
        const size_t end = (i + half_window >= length) ? length - 1 : i + half_window;
        const size_t current_window_size = end - start + 1;

        // Copy the current window
        memcpy(window, data + start, current_window_size * sizeof(uint16_t));

        // Sort the window using insertion sort
        for (size_t j = 1; j < current_window_size; ++j) {
            const uint16_t key = window[j];
            size_t k = j;
            while (k > 0 && window[k - 1] > key) {
                window[k] = window[k - 1];
                k--;
            }
            window[k] = key;
        }

        // Set the median value directly back to the original data array
        data[i] = window[current_window_size / 2];
        // Detect peaks
        if (data[i] > *max_value)
            *max_value = data[i];
        if (data[i] < *min_value)
            *min_value = data[i];
    }
    free(window);
    return true;
}

static esp_adc_cal_characteristics_t *adc_chars;

static void check_efuse()
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG_ZMPT101B, "eFuse Two Point: Supported");
    } else {
        ESP_LOGI(TAG_ZMPT101B, "eFuse Two Point: NOT supported");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG_ZMPT101B, "eFuse Vref: Supported");
    } else {
        ESP_LOGI(TAG_ZMPT101B, "eFuse Vref: NOT supported");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG_ZMPT101B, "Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG_ZMPT101B, "Characterized using eFuse Vref");
    } else {
        ESP_LOGI(TAG_ZMPT101B, "Characterized using Default Vref");
    }
}

// public API implementation
esp_err_t zmpt101b_init(adc_channel_t adc_channel)
{
    esp_err_t esp_err = ESP_OK;
    ESP_LOGI(TAG_ZMPT101B, "%s: Initializing ADC for channel %d", __FUNCTION__, adc_channel);
    check_efuse();

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN_DB, ADC_WIDTH_BIT, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    // I2S config
    i2s_config_t i2s_config =
    {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLING_FREQ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = DMA_BUFFER_LEN,
        .tx_desc_auto_clear = 1,
        .use_apll = 0,
    };

    // Configure ADC width
    esp_err |= adc1_config_width(ADC_WIDTH_BIT);

    // Configure attenuation for the ADC channel
    esp_err |= adc1_config_channel_atten(adc_channel, ADC_ATTEN_DB);

    esp_err |= i2s_driver_install(ADC_I2S_NUM, &i2s_config, 0, NULL);
    esp_err |= i2s_set_clk(ADC_I2S_NUM, SAMPLING_FREQ, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    esp_err |= i2s_set_adc_mode(ADC_UNIT, adc_channel);
    esp_err |= i2s_adc_enable(ADC_I2S_NUM);

    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG_ZMPT101B, "Failed to initialize ADC (%s)", esp_err_to_name(esp_err));
    }

    return esp_err;
}

esp_err_t zmpt101b_read_voltage(adc_channel_t adc_channel, uint16_t *rmsVoltage)
{
    *rmsVoltage = 0.0;
    ESP_LOGI(TAG_ZMPT101B, "%s: for channel %d", __FUNCTION__, adc_channel);

#ifdef DEBUG_EXTRA_INFO
    int64_t perf_start_time = esp_timer_get_time();
#endif

    uint16_t* i2s_read_buffer = (uint16_t*) calloc(I2S_READ_BUFFER_16B , sizeof(uint16_t));
    if (i2s_read_buffer == NULL) {
        ESP_LOGE(TAG_ZMPT101B, "Failed to allocate memory for I2S buffer");
        return ESP_ERR_NO_MEM;
    }

    size_t total_bytes_read = 0;
    do{
        size_t bytes_read = 0;
        esp_err_t ret = i2s_read(ADC_I2S_NUM, (uint8_t*)i2s_read_buffer + total_bytes_read, I2S_READ_BUFFER_16B * 2 - total_bytes_read, &bytes_read, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_ZMPT101B, "Failed to read data from I2S: %s", esp_err_to_name(ret));
            free(i2s_read_buffer);
            return ESP_ERR_INVALID_SIZE;
        }
        total_bytes_read += bytes_read;
    }while((total_bytes_read / 2) < I2S_READ_BUFFER_16B);

    uint16_t min_value = 0;
    uint16_t max_value = 0;
    // The median filter is necessary for filtering out voltage ripples.
    median_filter_in_place(i2s_read_buffer, I2S_READ_BUFFER_16B, 10, &min_value, &max_value);

    // Calculate the RMS voltage based on the full amplitude of the signal.
    // The amplitude difference (voltage_max - voltage_min) is divided by 2 to get the peak-to-peak amplitude.
    // The result is then divided by √2 (approximately 1.4142135) to convert peak-to-peak amplitude to RMS value.
    const uint16_t voltage_min = esp_adc_cal_raw_to_voltage(min_value, adc_chars);
    const uint16_t voltage_max = esp_adc_cal_raw_to_voltage(max_value, adc_chars);
    *rmsVoltage = round((( voltage_max - voltage_min ) / 2.0 ) / 1.4142135);

#ifdef DEBUG_EXTRA_INFO
    int64_t perf_end_time = esp_timer_get_time();
    int64_t perf_elapsed_time = perf_end_time - perf_start_time;

    // Print sensor voltage
    printf("SAMPLING_FREQ: %d\nSAMPLED: %d\n", SAMPLING_FREQ, I2S_READ_BUFFER_16B);
    for (size_t i = 0; i < I2S_READ_BUFFER_16B; i++) {
        const double voltage = esp_adc_cal_raw_to_voltage(i2s_read_buffer[i], adc_chars) / 1000.0;
        printf("%.2f ", voltage);
        if ((i + 1) % 32 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    // Print summary
    printf("%s Performance time: %lld microseconds ( %lld milliseconds )\n", __FUNCTION__, perf_elapsed_time, perf_elapsed_time / 1000);
    printf("sensor voltage delta == %.2fV\n", (voltage_max - voltage_min) / 1000.0 );
    printf("sensor voltage_max == %.2fV\n", voltage_max / 1000.0 );
    printf("sensor voltage_min == %.2fV\n", voltage_min / 1000.0 );
    printf("sensor measuring voltage == %dV\n", *rmsVoltage );
#endif
    free(i2s_read_buffer);

    return ESP_OK;
}