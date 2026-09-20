#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "driver/gpio.h"
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

#define DEVICE_ID CONFIG_IOT_DEVICE_ID
#define MQTT_BROKER_URL CONFIG_IOT_MQTT_BROKER_URL
#define DHT_PIN ((gpio_num_t)CONFIG_IOT_DHT_GPIO)
#define LED_PIN ((gpio_num_t)CONFIG_IOT_LED_GPIO)

static bool led_state;
static bool mqtt_connected;
static esp_mqtt_client_handle_t mqtt_client;

static void iso8601_utc_now(char *buffer, size_t size)
{
    time_t now;
    struct tm utc_time;

    time(&now);
    gmtime_r(&now, &utc_time);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", &utc_time);
}

static void sync_clock(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    esp_err_t err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "System clock synchronized");
    } else {
        ESP_LOGW(TAG, "SNTP sync timed out; timestamps may be inaccurate");
    }
}

static void publish_status(esp_mqtt_client_handle_t client, const char *status)
{
    char topic[96];
    char timestamp[32];
    char payload[192];

    iso8601_utc_now(timestamp, sizeof(timestamp));
    snprintf(topic, sizeof(topic), "device/%s/status", DEVICE_ID);
    snprintf(payload, sizeof(payload),
             "{\"deviceId\":\"%s\",\"status\":\"%s\",\"timestamp\":\"%s\"}",
             DEVICE_ID, status, timestamp);

    esp_mqtt_client_publish(client, topic, payload, 0, 1, 1);
    ESP_LOGI(TAG, "Published %s status", status);
}

static void publish_ack(esp_mqtt_client_handle_t client,
                        const char *command_id,
                        const char *action)
{
    char topic[112];
    char timestamp[32];
    char payload[320];

    iso8601_utc_now(timestamp, sizeof(timestamp));
    snprintf(topic, sizeof(topic), "device/%s/command/ack", DEVICE_ID);
    snprintf(payload, sizeof(payload),
             "{\"commandId\":\"%s\",\"deviceId\":\"%s\","
             "\"action\":\"%s\",\"status\":\"ACKNOWLEDGED\","
             "\"led\":%s,\"timestamp\":\"%s\"}",
             command_id, DEVICE_ID, action,
             led_state ? "true" : "false", timestamp);

    esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Sent ACK for command %s", command_id);
}

static void handle_command(esp_mqtt_client_handle_t client,
                           const char *data,
                           int data_len)
{
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    if (root == NULL) {
        ESP_LOGW(TAG, "Ignored invalid command JSON");
        return;
    }

    const cJSON *command_id = cJSON_GetObjectItemCaseSensitive(root, "commandId");
    const cJSON *action = cJSON_GetObjectItemCaseSensitive(root, "action");

    if (!cJSON_IsString(command_id) || command_id->valuestring == NULL ||
        !cJSON_IsString(action) || action->valuestring == NULL) {
        ESP_LOGW(TAG, "Command is missing commandId or action");
        cJSON_Delete(root);
        return;
    }

    bool supported = true;
    if (strcmp(action->valuestring, "LED_ON") == 0) {
        led_state = true;
        gpio_set_level(LED_PIN, 1);
    } else if (strcmp(action->valuestring, "LED_OFF") == 0) {
        led_state = false;
        gpio_set_level(LED_PIN, 0);
    } else {
        supported = false;
        ESP_LOGW(TAG, "Unsupported action: %s", action->valuestring);
    }

    if (supported) {
        ESP_LOGI(TAG, "Received %s", action->valuestring);
        publish_ack(client, command_id->valuestring, action->valuestring);
    }

    cJSON_Delete(root);
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
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
        snprintf(command_topic, sizeof(command_topic),
                 "device/%s/command", DEVICE_ID);
        esp_mqtt_client_subscribe(client, command_topic, 1);
        ESP_LOGI(TAG, "Subscribed %s", command_topic);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_DATA:
        handle_command(client, event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        break;
    }
}

static void mqtt_app_start(void)
{
    static char lwt_topic[96];
    static char lwt_payload[192];
    char timestamp[32];

    iso8601_utc_now(timestamp, sizeof(timestamp));
    snprintf(lwt_topic, sizeof(lwt_topic), "device/%s/status", DEVICE_ID);
    snprintf(lwt_payload, sizeof(lwt_payload),
             "{\"deviceId\":\"%s\",\"status\":\"OFFLINE\",\"timestamp\":\"%s\"}",
             DEVICE_ID, timestamp);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .session.last_will = {
            .topic = lwt_topic,
            .msg = lwt_payload,
            .qos = 1,
            .retain = 1,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

static void telemetry_task(void *pv_parameters)
{
    (void)pv_parameters;

    char topic[104];
    snprintf(topic, sizeof(topic), "device/%s/telemetry", DEVICE_ID);

    while (true) {
        float temperature = 0.0f;
        float humidity = 0.0f;

        if (dht22_read(&temperature, &humidity) == ESP_OK) {
            if (mqtt_connected) {
                char timestamp[32];
                char payload[320];
                iso8601_utc_now(timestamp, sizeof(timestamp));
                snprintf(payload, sizeof(payload),
                         "{\"deviceId\":\"%s\",\"temperature\":%.1f,"
                         "\"humidity\":%.1f,\"illuminance\":null,"
                         "\"soilMoisture\":null,\"led\":%s,"
                         "\"timestamp\":\"%s\"}",
                         DEVICE_ID, temperature, humidity,
                         led_state ? "true" : "false", timestamp);

                esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 0, 0);
                ESP_LOGI(TAG, "Published Telemetry: T=%.1f H=%.1f",
                         temperature, humidity);
            } else {
                ESP_LOGW(TAG, "MQTT is offline; telemetry skipped");
            }
        } else {
            ESP_LOGW(TAG, "DHT22 read failed; check DATA pin and pull-up resistor");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting device %s", DEVICE_ID);
    ESP_LOGI(TAG, "ESP-IDF version: %s", esp_get_idf_version());

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(wifi_manager_start());
    sync_clock();

    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_config));
    gpio_set_level(LED_PIN, 0);

    ESP_ERROR_CHECK(dht22_init(DHT_PIN));
    mqtt_app_start();

    xTaskCreate(telemetry_task, "telemetry_task", 4096, NULL, 5, NULL);
}
