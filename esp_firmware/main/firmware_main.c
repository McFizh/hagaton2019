#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "mdns.h"
#include "mqtt_client.h"

#include "dht22.h"

#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"

static EventGroupHandle_t wifi_event_group;

const int IP4_CONNECTED_BIT = BIT0;
const int IP6_CONNECTED_BIT = BIT1;
const int MQTT_ADDR_FOUND = BIT2;
const int MQTT_LINK_ESTABLISHED = BIT3;

esp_mqtt_client_handle_t mqtt_client;
char mqtt_host[45];

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch( event->event_id ) {
        // ID: 2
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            printf("Got IP4\n");
            xEventGroupSetBits(wifi_event_group, IP4_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            printf("Got IP6\n");
            xEventGroupSetBits(wifi_event_group, IP6_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            printf("Disconnected\n");
            // Auto reconnect
            // esp_wifi_connect();

            // Reset bits & mqtt data
            xEventGroupClearBits(
                wifi_event_group, 
                IP4_CONNECTED_BIT | IP6_CONNECTED_BIT | 
                MQTT_ADDR_FOUND | MQTT_LINK_ESTABLISHED );

            memset(mqtt_host,0,45);
            break;
        default:
            break;
    }

    mdns_handle_system_event(ctx, event);
    return ESP_OK;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            printf("MQTT connected\n");
            xEventGroupSetBits(wifi_event_group, MQTT_LINK_ESTABLISHED);
            break;
        case MQTT_EVENT_DISCONNECTED:
            printf("MQTT disconnected\n");
            xEventGroupClearBits(wifi_event_group, MQTT_LINK_ESTABLISHED);
            break;
        case MQTT_EVENT_ERROR:
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            break;
        case MQTT_EVENT_SUBSCRIBED:
            break;
        case MQTT_EVENT_PUBLISHED:
            printf("MQTT msg published\n");
            break;
        case MQTT_EVENT_DATA:
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            break;
    }

    return ESP_OK;
}

void init_wifi()
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void mdns_find_mqtt(mdns_result_t * results) {
    mdns_result_t *resIterator = results;
    mdns_ip_addr_t *a = NULL;
    uint8_t i;
    uint16_t mqtt_port = 0;

    while(resIterator) {
        // Identify server from TXT field
        if(!resIterator->txt_count) {
            resIterator = resIterator->next;
            continue;
        }

        // Verify server & determine port
        for(i=0; i<resIterator->txt_count; i++) {
            if( strncmp("info",resIterator->txt[i].key,4) == 0 &&
                strncmp("MQTT server",resIterator->txt[i].value,11) == 0 ) {
                    mqtt_port = resIterator->port;
            }
        }

        // Most likely no results
        if(mqtt_port == 0) {
            resIterator = resIterator->next;
            continue;
        }

        a = resIterator->addr;
        while(a){
            // Ignore IPV6
            if(a->addr.type == IPADDR_TYPE_V6){
                a = a->next;
                continue;
            }

            snprintf(
                mqtt_host,45,
                "mqtt://test:test@%d.%d.%d.%d:%u",
                IP2STR(&(a->addr.u_addr.ip4)),mqtt_port);
            break;
        }

        // Determine IP
        xEventGroupSetBits(wifi_event_group, MQTT_ADDR_FOUND);
        break;
    }
}

static void app_primary_task(void *pvParameters)
{
    // Wait for app to get ip address
    xEventGroupWaitBits(wifi_event_group,
        IP4_CONNECTED_BIT | IP6_CONNECTED_BIT,
        false, true, portMAX_DELAY);

    //
    printf("Application received IP\n");

    // Wait until service is found:
    mdns_result_t *results = NULL;
    while(1) {
        mdns_query_ptr("_mqtt", "_tcp", 3000, 20, &results);

        if(!results) {
            printf("No results found!\n");
        } else {
            mdns_find_mqtt(results);
            mdns_query_results_free(results);
            break;
        }

        // Delay for 5s, then try again
        vTaskDelay( 5000 / portTICK_RATE_MS );
    }

    //
    if( xEventGroupGetBits(wifi_event_group) & MQTT_ADDR_FOUND ) {
        printf("Trying to connect to MQTT: host: [%s]\n",mqtt_host);

        esp_mqtt_client_config_t mqtt_cfg = {
            .uri = mqtt_host,
            .event_handle = mqtt_event_handler
        };

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_start(mqtt_client);
    }

    //
    vTaskDelete( NULL );
/*
    // Doze for 180 seconds
    printf("Starting sleep cycle...\n");
    esp_sleep_enable_timer_wakeup(180*1000*1000);
    esp_deep_sleep_start();
*/
}

static void app_sensor_task(void *pvParameters)
{
    printf("Reading sensor data\n");

    readDht();

    printf("Temp: %f , Hum: %f\n", getDhtTemp(), getDhtHum() );

    vTaskDelete( NULL );
}

static void app_mqtt_task(void *pvParameters) 
{
    // Wait until we find mqtt server
    xEventGroupWaitBits(wifi_event_group,
        MQTT_LINK_ESTABLISHED,
        false, true, portMAX_DELAY);

    while(1) {
        esp_mqtt_client_publish(mqtt_client, "/topic/test", "{\"temp\":21.2}", 0, 1, 0);
        vTaskDelay( 15000 / portTICK_RATE_MS );
    }
}

void app_main()
{
    int64_t time;

    // Alustetaan flash muisti ja odotetaan 1S, että DHT22 on valmis
    time = esp_timer_get_time();

	gpio_set_direction( 23, GPIO_MODE_OUTPUT );
	gpio_set_direction( 22, GPIO_MODE_OUTPUT );
    gpio_set_level( 23, 1 );
    gpio_set_level( 22, 1 );

    ESP_ERROR_CHECK(nvs_flash_init());
    init_wifi();
    ESP_ERROR_CHECK( mdns_init() );
    initDht22(23);

    // Käynnistykseen kulunut aika (ms) , jos alle 1.5s .. odotetaan että aika tulee täyteen
    // .. vaaditaan dht22:n käynnistymiseen
    time = ( esp_timer_get_time()-time ) / 1000;

    if(time < 1500) {
        vTaskDelay( ( 1500-time ) / portTICK_RATE_MS );
    }

    // Luetaan DHT22
    printf("init took: %lld ms\n", time );

    vTaskDelay( ( 5000-time ) / portTICK_RATE_MS );

    //
    xTaskCreate(&app_sensor_task, "app_sensor_task", 2048, NULL, 5, NULL);
    xTaskCreate(&app_primary_task, "app_primary_task", 2048, NULL, 5, NULL);
    xTaskCreate(&app_mqtt_task, "app_mqtt_task", 2048, NULL, 5, NULL);
}