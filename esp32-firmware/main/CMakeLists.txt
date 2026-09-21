idf_component_register(
    SRCS
        "main.c"
        "dht22.c"
        "wifi_manager.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        mqtt
        json
        driver
        esp_timer
        esp_adc
        lwip
)