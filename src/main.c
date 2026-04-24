#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WIFI_SSID "your_ssid"
#define WIFI_PASS "your_password"
#define API_URL   "https://api.openai.com/v1/chat/completions"
#define API_KEY   "Bearer your_api_key"

static const char *TAG = "CHATBOT";

/* =========================
   WiFi Event Handler
   ========================= */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        esp_wifi_connect();

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        ESP_LOGI(TAG, "Connected to WiFi");
}

/* =========================
   WiFi Initialization
   ========================= */
void wifi_init()
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

/* =========================
   HTTP Response Handler
   ========================= */
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        printf("%.*s\n", evt->data_len, (char*)evt->data);
    }
    return ESP_OK;
}

/* =========================
   Send Chat Request
   ========================= */
void send_chat_request()
{
    const char *post_data =
        "{"
        "\"model\":\"gpt-3.5-turbo\","
        "\"messages\":[{\"role\":\"user\",\"content\":\"Hello from ESP32\"}]"
        "}";

    esp_http_client_config_t config = {
        .url = API_URL,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", API_KEY);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Request successful");
    } else {
        ESP_LOGE(TAG, "Request failed");
    }

    esp_http_client_cleanup(client);
}

/* =========================
   Main
   ========================= */
void app_main(void)
{
    wifi_init();

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    while (1) {
        send_chat_request();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
