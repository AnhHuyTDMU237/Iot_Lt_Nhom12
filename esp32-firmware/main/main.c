#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dht22.h"
#include "wifi_manager.h"


static const char *TAG = "IOT_ESP32";


/* =========================================================
 * CẤU HÌNH THIẾT BỊ TỪ MENUCONFIG
 * ========================================================= */

#define DEVICE_ID          CONFIG_IOT_DEVICE_ID
#define MQTT_BROKER_URL    CONFIG_IOT_MQTT_BROKER_URL
#define DHT_PIN            ((gpio_num_t)CONFIG_IOT_DHT_GPIO)
#define LED_PIN            ((gpio_num_t)CONFIG_IOT_LED_GPIO)


/* =========================================================
 * CẤU HÌNH CẢM BIẾN ANALOG
 *
 * ESP32-S3:
 * GPIO4 = ADC1_CHANNEL_3
 * GPIO5 = ADC1_CHANNEL_4
 * ========================================================= */

#define LIGHT_ADC_CHANNEL  ADC_CHANNEL_3
#define SOIL_ADC_CHANNEL   ADC_CHANNEL_4

#define ADC_SAMPLE_COUNT   32


/*
 * Giá trị hiệu chỉnh cảm biến ánh sáng.
 *
 * Ban đầu để 0 và 4095.
 * Sau khi chạy, xem light_raw trong log để hiệu chỉnh lại:
 *
 * LIGHT_DARK_RAW:
 * Giá trị khi che kín cảm biến.
 *
 * LIGHT_BRIGHT_RAW:
 * Giá trị khi chiếu đèn mạnh vào cảm biến.
 */
#define LIGHT_DARK_RAW     0
#define LIGHT_BRIGHT_RAW   4095


/*
 * Giá trị hiệu chỉnh cảm biến độ ẩm đất.
 *
 * SOIL_DRY_RAW:
 * Giá trị khi cảm biến khô.
 *
 * SOIL_WET_RAW:
 * Giá trị khi cảm biến đặt trong đất ẩm.
 */
#define SOIL_DRY_RAW       3200
#define SOIL_WET_RAW       1400


/* =========================================================
 * BIẾN TOÀN CỤC
 * ========================================================= */

static bool led_state = false;
static volatile bool mqtt_connected = false;

static esp_mqtt_client_handle_t mqtt_client = NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;


/* =========================================================
 * HÀM TẠO THỜI GIAN ISO 8601
 * ========================================================= */

static void iso8601_utc_now(char *buffer, size_t size)
{
    time_t now;
    struct tm utc_time;

    time(&now);
    gmtime_r(&now, &utc_time);

    strftime(
        buffer,
        size,
        "%Y-%m-%dT%H:%M:%SZ",
        &utc_time
    );
}


/* =========================================================
 * ĐỒNG BỘ THỜI GIAN SNTP
 * ========================================================= */

static void sync_clock(void)
{
    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    esp_netif_sntp_init(&config);

    esp_err_t err =
        esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000));

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "System clock synchronized");
    } else {
        ESP_LOGW(
            TAG,
            "SNTP sync timed out; timestamps may be inaccurate"
        );
    }
}


/* =========================================================
 * KHỞI TẠO ADC
 * ========================================================= */

static esp_err_t analog_sensors_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t err =
        adc_oneshot_new_unit(&unit_config, &adc_handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to initialize ADC unit: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    err = adc_oneshot_config_channel(
        adc_handle,
        LIGHT_ADC_CHANNEL,
        &channel_config
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to configure light ADC: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    err = adc_oneshot_config_channel(
        adc_handle,
        SOIL_ADC_CHANNEL,
        &channel_config
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to configure soil ADC: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "Analog sensors initialized: Light=GPIO4, Soil=GPIO5"
    );

    return ESP_OK;
}


/* =========================================================
 * ĐỌC ADC TRUNG BÌNH NHIỀU LẦN
 * ========================================================= */

static int read_adc_average(adc_channel_t channel)
{
    int total = 0;

    for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
        int raw = 0;

        esp_err_t err = adc_oneshot_read(
            adc_handle,
            channel,
            &raw
        );

        if (err != ESP_OK) {
            ESP_LOGW(
                TAG,
                "ADC read failed on channel %d: %s",
                channel,
                esp_err_to_name(err)
            );

            return -1;
        }

        total += raw;
    }

    return total / ADC_SAMPLE_COUNT;
}


/* =========================================================
 * CHUYỂN GIÁ TRỊ ADC SANG PHẦN TRĂM
 *
 * Hàm hỗ trợ cả trường hợp:
 * - Giá trị tăng khi độ ẩm/ánh sáng tăng.
 * - Giá trị giảm khi độ ẩm/ánh sáng tăng.
 * ========================================================= */

static float raw_to_percent(
    int raw,
    int zero_raw,
    int hundred_raw
)
{
    if (zero_raw == hundred_raw) {
        return 0.0f;
    }

    float percent =
        ((float)(raw - zero_raw) * 100.0f) /
        ((float)(hundred_raw - zero_raw));

    if (percent < 0.0f) {
        percent = 0.0f;
    }

    if (percent > 100.0f) {
        percent = 100.0f;
    }

    return percent;
}


/* =========================================================
 * GỬI TRẠNG THÁI ONLINE/OFFLINE
 * ========================================================= */

static void publish_status(
    esp_mqtt_client_handle_t client,
    const char *status
)
{
    char topic[96];
    char timestamp[32];
    char payload[192];

    iso8601_utc_now(timestamp, sizeof(timestamp));

    snprintf(
        topic,
        sizeof(topic),
        "device/%s/status",
        DEVICE_ID
    );

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"deviceId\":\"%s\","
        "\"status\":\"%s\","
        "\"timestamp\":\"%s\""
        "}",
        DEVICE_ID,
        status,
        timestamp
    );

    esp_mqtt_client_publish(
        client,
        topic,
        payload,
        0,
        1,
        1
    );

    ESP_LOGI(TAG, "Published %s status", status);
}


/* =========================================================
 * GỬI ACK SAU KHI NHẬN LỆNH
 * ========================================================= */

static void publish_ack(
    esp_mqtt_client_handle_t client,
    const char *command_id,
    const char *action
)
{
    char topic[112];
    char timestamp[32];
    char payload[320];

    iso8601_utc_now(timestamp, sizeof(timestamp));

    snprintf(
        topic,
        sizeof(topic),
        "device/%s/command/ack",
        DEVICE_ID
    );

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"commandId\":\"%s\","
        "\"deviceId\":\"%s\","
        "\"action\":\"%s\","
        "\"status\":\"ACKNOWLEDGED\","
        "\"led\":%s,"
        "\"timestamp\":\"%s\""
        "}",
        command_id,
        DEVICE_ID,
        action,
        led_state ? "true" : "false",
        timestamp
    );

    esp_mqtt_client_publish(
        client,
        topic,
        payload,
        0,
        1,
        0
    );

    ESP_LOGI(
        TAG,
        "Sent ACK for command %s",
        command_id
    );
}


/* =========================================================
 * XỬ LÝ LỆNH LED
 * ========================================================= */

static void handle_command(
    esp_mqtt_client_handle_t client,
    const char *data,
    int data_len
)
{
    cJSON *root =
        cJSON_ParseWithLength(data, data_len);

    if (root == NULL) {
        ESP_LOGW(TAG, "Ignored invalid command JSON");
        return;
    }

    const cJSON *command_id =
        cJSON_GetObjectItemCaseSensitive(
            root,
            "commandId"
        );

    const cJSON *action =
        cJSON_GetObjectItemCaseSensitive(
            root,
            "action"
        );

    if (!cJSON_IsString(command_id) ||
        command_id->valuestring == NULL ||
        !cJSON_IsString(action) ||
        action->valuestring == NULL) {

        ESP_LOGW(
            TAG,
            "Command is missing commandId or action"
        );

        cJSON_Delete(root);
        return;
    }

    bool supported = true;

    if (strcmp(action->valuestring, "LED_ON") == 0) {
        led_state = true;
        gpio_set_level(LED_PIN, 1);

    } else if (
        strcmp(action->valuestring, "LED_OFF") == 0
    ) {
        led_state = false;
        gpio_set_level(LED_PIN, 0);

    } else {
        supported = false;

        ESP_LOGW(
            TAG,
            "Unsupported action: %s",
            action->valuestring
        );
    }

    if (supported) {
        ESP_LOGI(
            TAG,
            "Received %s",
            action->valuestring
        );

        publish_ack(
            client,
            command_id->valuestring,
            action->valuestring
        );
    }

    cJSON_Delete(root);
}


/* =========================================================
 * MQTT EVENT HANDLER
 * ========================================================= */

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            char command_topic[104];

            mqtt_connected = true;

            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            publish_status(client, "ONLINE");

            snprintf(
                command_topic,
                sizeof(command_topic),
                "device/%s/command",
                DEVICE_ID
            );

            esp_mqtt_client_subscribe(
                client,
                command_topic,
                1
            );

            ESP_LOGI(
                TAG,
                "Subscribed %s",
                command_topic
            );

            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;

            ESP_LOGW(
                TAG,
                "MQTT_EVENT_DISCONNECTED"
            );

            break;

        case MQTT_EVENT_DATA:
            handle_command(
                client,
                event->data,
                event->data_len
            );

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            break;
    }
}


/* =========================================================
 * KHỞI ĐỘNG MQTT
 * ========================================================= */

static void mqtt_app_start(void)
{
    static char lwt_topic[96];
    static char lwt_payload[192];

    /*
     * Last Will được khai báo khi ESP32 kết nối MQTT.
     * Khi ESP32 mất điện hoặc mất Wi-Fi, EMQX sẽ tự gửi
     * payload OFFLINE sau khi hết thời gian keepalive.
     */
    snprintf(
        lwt_topic,
        sizeof(lwt_topic),
        "device/%s/status",
        DEVICE_ID
    );

    snprintf(
        lwt_payload,
        sizeof(lwt_payload),
        "{"
        "\"deviceId\":\"%s\","
        "\"status\":\"OFFLINE\""
        "}",
        DEVICE_ID
    );

    const esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER_URL,

        .session = {
            /*
             * ESP32 gửi gói MQTT keepalive mỗi 10 giây.
             * Broker thường phát hiện mất kết nối sau
             * khoảng 1 đến 1.5 lần thời gian này.
             */
            .keepalive = 10,

            .last_will = {
                .topic = lwt_topic,
                .msg = lwt_payload,
                .qos = 1,
                .retain = 1,
            },
        },

        /*
         * Nếu chỉ mất Wi-Fi tạm thời, thử kết nối lại
         * sau mỗi 5 giây.
         */
        .network.reconnect_timeout_ms = 5000,
    };

    mqtt_client =
        esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    ESP_ERROR_CHECK(
        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_mqtt_client_start(mqtt_client)
    );
}


/* =========================================================
 * TASK ĐỌC VÀ GỬI DỮ LIỆU CẢM BIẾN
 * ========================================================= */

static void telemetry_task(void *pv_parameters)
{
    (void)pv_parameters;

    char topic[104];

    snprintf(
        topic,
        sizeof(topic),
        "device/%s/telemetry",
        DEVICE_ID
    );

    while (true) {
        float temperature = 0.0f;
        float humidity = 0.0f;

        esp_err_t dht_result =
            dht22_read(&temperature, &humidity);

        int light_raw =
            read_adc_average(LIGHT_ADC_CHANNEL);

        int soil_raw =
            read_adc_average(SOIL_ADC_CHANNEL);

        if (dht_result == ESP_OK &&
            light_raw >= 0 &&
            soil_raw >= 0) {

            float light_percent = raw_to_percent(
                light_raw,
                LIGHT_DARK_RAW,
                LIGHT_BRIGHT_RAW
            );

            float soil_percent = raw_to_percent(
                soil_raw,
                SOIL_DRY_RAW,
                SOIL_WET_RAW
            );

            ESP_LOGI(
                TAG,
                "Sensors: T=%.1f C, H=%.1f %%, "
                "Light=%.1f %% (raw=%d), "
                "Soil=%.1f %% (raw=%d)",
                temperature,
                humidity,
                light_percent,
                light_raw,
                soil_percent,
                soil_raw
            );

            if (mqtt_connected) {
                char timestamp[32];
                char payload[384];

                iso8601_utc_now(
                    timestamp,
                    sizeof(timestamp)
                );

                snprintf(
                    payload,
                    sizeof(payload),
                    "{"
                    "\"deviceId\":\"%s\","
                    "\"temperature\":%.1f,"
                    "\"humidity\":%.1f,"
                    "\"illuminance\":%.1f,"
                    "\"soilMoisture\":%.1f,"
                    "\"led\":%s,"
                    "\"timestamp\":\"%s\""
                    "}",
                    DEVICE_ID,
                    temperature,
                    humidity,
                    light_percent,
                    soil_percent,
                    led_state ? "true" : "false",
                    timestamp
                );

                esp_mqtt_client_publish(
                    mqtt_client,
                    topic,
                    payload,
                    0,
                    0,
                    0
                );

                ESP_LOGI(
                    TAG,
                    "Published telemetry: "
                    "T=%.1f H=%.1f "
                    "Light=%.1f Soil=%.1f",
                    temperature,
                    humidity,
                    light_percent,
                    soil_percent
                );

            } else {
                ESP_LOGW(
                    TAG,
                    "MQTT is offline; telemetry skipped"
                );
            }

        } else {
            if (dht_result != ESP_OK) {
                ESP_LOGW(
                    TAG,
                    "DHT22 read failed: %s",
                    esp_err_to_name(dht_result)
                );
            }

            if (light_raw < 0) {
                ESP_LOGW(
                    TAG,
                    "Light sensor ADC read failed"
                );
            }

            if (soil_raw < 0) {
                ESP_LOGW(
                    TAG,
                    "Soil sensor ADC read failed"
                );
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


/* =========================================================
 * APP MAIN
 * ========================================================= */

void app_main(void)
{
    ESP_LOGI(
        TAG,
        "Starting device %s",
        DEVICE_ID
    );

    ESP_LOGI(
        TAG,
        "ESP-IDF version: %s",
        esp_get_idf_version()
    );

    /*
     * Khởi tạo NVS.
     */
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());

        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    /*
     * Khởi tạo mạng.
     */
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    /*
     * Kết nối Wi-Fi.
     */
    ESP_ERROR_CHECK(wifi_manager_start());

    /*
     * Đồng bộ thời gian.
     */
    sync_clock();

    /*
     * Khởi tạo LED.
     */
    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(
        gpio_config(&led_config)
    );

    led_state = false;
    gpio_set_level(LED_PIN, 0);

    /*
     * Khởi tạo DHT22.
     */
    ESP_ERROR_CHECK(
        dht22_init(DHT_PIN)
    );

    /*
     * Khởi tạo cảm biến ánh sáng và độ ẩm đất.
     */
    ESP_ERROR_CHECK(
        analog_sensors_init()
    );

    /*
     * Khởi động MQTT.
     */
    mqtt_app_start();

    /*
     * Tạo task gửi dữ liệu mỗi 5 giây.
     */
    BaseType_t task_result = xTaskCreate(
        telemetry_task,
        "telemetry_task",
        4096,
        NULL,
        5,
        NULL
    );

    if (task_result != pdPASS) {
        ESP_LOGE(
            TAG,
            "Failed to create telemetry task"
        );
    }
}