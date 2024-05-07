#include "mqttHandler.h"

#include "esp_log.h"
#include "secrets.h"
#include "wifi.h"
#include "capactiveSensor.h"
// #include "piezoSensor.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
esp_mqtt_client_handle_t clientHandle;

#define TAG "mqttHandler"

#define maxSubscriptions 10
void (*subscribeCallbacks[maxSubscriptions])(char *payload, int payloadLength, char *topic);
char *subscribedTopics[maxSubscriptions];
int subscriptionCount = 0;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        for (int i = 0; i < subscriptionCount; i++)
        {
            if (strncmp(subscribedTopics[i], event->topic, event->topic_len) == 0)
            {
                subscribeCallbacks[i](event->data, event->data_len, event->topic);
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        ESP_LOGI(TAG, "event info: %u", event->error_handle->connect_return_code);
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void initMQTTapp(void)
{
    ESP_LOGI(TAG, "Connecting to MQTT Broker");

        // register will message
    struct last_will_t will = {
        .topic = MQTT_TOPIC_WILL,
        .msg = "{\"device\":\"SleepMonitor\", \"state\":\"offline\"}",
        .qos = 1,
        .retain = 1,
    };

    esp_mqtt_client_config_t mqtt_cfg = {
        // PLAY AROUND WITH THESE SETTINGS
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .broker.address.hostname = MQTT_HOST,
        .broker.address.port = 1883,
        .credentials.authentication.password = MQTT_PASS,
        .credentials.username = MQTT_USER,
        .session.last_will = will,
    };

    clientHandle = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(clientHandle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(clientHandle);

    esp_mqtt_client_publish(clientHandle, MQTT_TOPIC_WILL, "{\"device\":\"SleepMonitor\", \"state\":\"online\"}", 0, 1, 1);

    ESP_LOGI(TAG, "MQTT Client started");
}

void sendMqttData(char *payload, char *topic, int retain)
{
    ESP_LOGI(TAG, "Sending MQTT Data");
    
    stopCapacitiveSensor();
    int msg_id = esp_mqtt_client_publish(clientHandle, topic, payload, 0, 1, retain);
    startCapacitiveSensor();
    ESP_LOGI(TAG, "sent publish done, msg_id=%d", msg_id);
}

void subscribeToTopic(char *topic, void (*callback)(char *payload, int payloadLength, char *topic))
{
    ESP_LOGI(TAG, "Subscribing to topic");
    stopCapacitiveSensor();
    esp_mqtt_client_subscribe(clientHandle, topic, 1);
    startCapacitiveSensor();
    subscribeCallbacks[subscriptionCount] = callback;
    subscribedTopics[subscriptionCount] = topic;
    subscriptionCount++;
}

void initMQTT()
{
    initMQTTapp();
}
