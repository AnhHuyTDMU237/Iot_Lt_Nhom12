#include "dht22.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_rom_sys.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static gpio_num_t dht_pin = GPIO_NUM_NC;
static portMUX_TYPE dht_mux = portMUX_INITIALIZER_UNLOCKED;

/*
 * Chờ tín hiệu thoát khỏi một mức logic.
 * Trả về thời gian tín hiệu giữ mức đó, đơn vị microsecond.
 * Trả về -1 nếu quá thời gian chờ.
 */
static int wait_while_level(int level, int timeout_us)
{
    int64_t start_time = esp_timer_get_time();

    while (gpio_get_level(dht_pin) == level) {
        if ((esp_timer_get_time() - start_time) >= timeout_us) {
            return -1;
        }
    }

    return (int)(esp_timer_get_time() - start_time);
}

esp_err_t dht22_init(gpio_num_t pin)
{
    if (!GPIO_IS_VALID_GPIO(pin)) {
        return ESP_ERR_INVALID_ARG;
    }

    dht_pin = pin;

    gpio_config_t config = {
        .pin_bit_mask = 1ULL << dht_pin,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t result = gpio_config(&config);
    if (result != ESP_OK) {
        return result;
    }

    /* Trạng thái nghỉ của DHT22 là HIGH. */
    gpio_set_level(dht_pin, 1);

    return ESP_OK;
}

esp_err_t dht22_read(float *temperature, float *humidity)
{
    if (temperature == NULL ||
        humidity == NULL ||
        dht_pin == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[5] = {0};

    /*
     * Gửi tín hiệu bắt đầu:
     * ESP32 kéo DATA xuống LOW trong 2 ms.
     *
     * Phải dùng esp_rom_delay_us thay cho vTaskDelay vì DHT22
     * cần thời gian chính xác theo microsecond.
     */
    gpio_set_direction(dht_pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(dht_pin, 0);
    esp_rom_delay_us(2000);

    /*
     * Bắt đầu vùng đọc tín hiệu nhạy thời gian.
     * Tạm thời ngăn task hoặc interrupt khác làm sai timing.
     */
    portENTER_CRITICAL(&dht_mux);

    /* Nhả đường DATA lên HIGH. */
    gpio_set_level(dht_pin, 1);
    esp_rom_delay_us(30);

    /* Chuyển GPIO sang chế độ đọc dữ liệu. */
    gpio_set_direction(dht_pin, GPIO_MODE_INPUT);

    bool timing_ok = true;

    /*
     * DHT22 phản hồi:
     * LOW khoảng 80 us
     * HIGH khoảng 80 us
     * Sau đó bắt đầu gửi 40 bit.
     */
    if (wait_while_level(1, 200) < 0 ||
        wait_while_level(0, 200) < 0 ||
        wait_while_level(1, 200) < 0) {
        timing_ok = false;
    }

    for (int bit = 0; bit < 40 && timing_ok; bit++) {
        /*
         * Mỗi bit bắt đầu bằng LOW khoảng 50 us.
         */
        if (wait_while_level(0, 120) < 0) {
            timing_ok = false;
            break;
        }

        /*
         * HIGH khoảng:
         * 26-28 us = bit 0
         * 70 us    = bit 1
         */
        int high_time = wait_while_level(1, 150);

        if (high_time < 0) {
            timing_ok = false;
            break;
        }

        data[bit / 8] <<= 1;

        if (high_time > 40) {
            data[bit / 8] |= 1;
        }
    }

    portEXIT_CRITICAL(&dht_mux);

    /* Trả đường DATA về trạng thái nghỉ. */
    gpio_set_direction(dht_pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(dht_pin, 1);

    if (!timing_ok) {
        return ESP_ERR_TIMEOUT;
    }

    uint8_t checksum =
        (uint8_t)(data[0] + data[1] + data[2] + data[3]);

    if (checksum != data[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t humidity_raw =
        ((uint16_t)data[0] << 8) | data[1];

    uint16_t temperature_raw =
        ((uint16_t)(data[2] & 0x7F) << 8) | data[3];

    *humidity = humidity_raw / 10.0f;
    *temperature = temperature_raw / 10.0f;

    if ((data[2] & 0x80) != 0) {
        *temperature = -*temperature;
    }

    return ESP_OK;
}