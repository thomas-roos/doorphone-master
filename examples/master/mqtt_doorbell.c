#include "mqtt_doorbell.h"
#include "demo_config.h"
#include "transport_mbedtls.h"
#include "core_mqtt.h"
#include <stdio.h>
#include <string.h>

#define MQTT_BROKER_PORT    8883
#define MQTT_CLIENT_ID      "doorbell-master-ameba"

static TlsNetworkContext_t tlsContext;
static NetworkCredentials_t credentials;
static MQTTContext_t mqttContext;
static uint8_t networkBuffer[1024];
static bool mqttConnected = false;
static TlsTransportParams_t tlsParams;

static uint32_t getTimeMs(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static void mqttEventCallback(MQTTContext_t *pContext, MQTTPacketInfo_t *pPacketInfo, MQTTDeserializedInfo_t *pDeserializedInfo) {
    // Simple event callback - just log events
    (void)pContext;
    (void)pPacketInfo;
    (void)pDeserializedInfo;
}

static int32_t transportSend(NetworkContext_t *pNetworkContext, const void *pBuffer, size_t bytesToSend) {
    return TLS_FreeRTOS_send((TlsNetworkContext_t*)pNetworkContext, pBuffer, bytesToSend);
}

static int32_t transportRecv(NetworkContext_t *pNetworkContext, void *pBuffer, size_t bytesToRecv) {
    return TLS_FreeRTOS_recv((TlsNetworkContext_t*)pNetworkContext, pBuffer, bytesToRecv);
}

int32_t mqtt_doorbell_init(void) {
    printf("MQTT doorbell client initializing TLS to AWS IoT...\n");
    printf("Target: %s:%d\n", AWS_IOT_CORE_ENDPOINT, MQTT_BROKER_PORT);
    
    // Initialize TLS context properly
    memset(&tlsContext, 0, sizeof(tlsContext));
    tlsContext.pParams = &tlsParams;
    
    // Clear and setup credentials properly
    memset(&credentials, 0, sizeof(credentials));
    credentials.pRootCa = (const uint8_t*)AWS_CA_CERT_PEM;
    credentials.rootCaSize = sizeof(AWS_CA_CERT_PEM);
    credentials.pClientCert = (const uint8_t*)AWS_IOT_THING_CERT;
    credentials.clientCertSize = sizeof(AWS_IOT_THING_CERT);
    credentials.pPrivateKey = (const uint8_t*)AWS_IOT_THING_PRIVATE_KEY;
    credentials.privateKeySize = sizeof(AWS_IOT_THING_PRIVATE_KEY);
    
    printf("   Cert size: %zu, Key size: %zu\n", credentials.clientCertSize, credentials.privateKeySize);
    
    printf("1. Connecting TLS...\n");
    TlsTransportStatus_t status = TLS_FreeRTOS_Connect(
        &tlsContext, AWS_IOT_CORE_ENDPOINT, MQTT_BROKER_PORT, &credentials, 10000, 10000, 0);
    
    if (status != TLS_TRANSPORT_SUCCESS) {
        printf("MQTT TLS connection FAILED: %d\n", status);
        return -1;
    }
    printf("   TLS connection established\n");
    
    printf("2. Initializing MQTT...\n");
    TransportInterface_t transport = {0};
    transport.pNetworkContext = (NetworkContext_t*)&tlsContext;
    transport.send = transportSend;
    transport.recv = transportRecv;
    
    MQTTFixedBuffer_t networkBuffer_config = { .pBuffer = networkBuffer, .size = sizeof(networkBuffer) };
    
    MQTTStatus_t mqttStatus = MQTT_Init(&mqttContext, &transport, getTimeMs, mqttEventCallback, &networkBuffer_config);
    if (mqttStatus != MQTTSuccess) {
        printf("MQTT_Init failed: %d\n", mqttStatus);
        return -1;
    }
    printf("   MQTT initialized\n");
    
    printf("3. Connecting to MQTT broker...\n");
    MQTTConnectInfo_t connectInfo = {0};
    connectInfo.pClientIdentifier = MQTT_CLIENT_ID;
    connectInfo.clientIdentifierLength = strlen(MQTT_CLIENT_ID);
    connectInfo.keepAliveSeconds = 60;
    connectInfo.cleanSession = true;
    
    bool sessionPresent = false;
    mqttStatus = MQTT_Connect(&mqttContext, &connectInfo, NULL, 10000, &sessionPresent);
    if (mqttStatus != MQTTSuccess) {
        printf("MQTT_Connect failed: %d\n", mqttStatus);
        return -1;
    }
    
    mqttConnected = true;
    printf("MQTT connection SUCCESS\n");
    return 0;
}

void mqtt_doorbell_send_ring_event(void) {
    if (!mqttConnected) {
        printf("MQTT not connected - cannot publish\n");
        return;
    }
    
    char topic[128];
    char payload[128];
    
    snprintf(topic, sizeof(topic), "doorbell/%s/ring", AWS_KVS_CHANNEL_NAME);
    snprintf(payload, sizeof(payload), "{\"event\":\"ring\",\"device\":\"doorbell-master\"}");
    
    printf("Publishing to topic: %s\n", topic);
    printf("Payload: %s\n", payload);
    
    MQTTPublishInfo_t publishInfo = {0};
    publishInfo.pTopicName = topic;
    publishInfo.topicNameLength = strlen(topic);
    publishInfo.pPayload = payload;
    publishInfo.payloadLength = strlen(payload);
    publishInfo.qos = MQTTQoS0;
    
    static uint16_t packetId = 1;
    MQTTStatus_t status = MQTT_Publish(&mqttContext, &publishInfo, packetId++);
    printf("MQTT_Publish status: %d\n", status);
    
    if (status == MQTTSuccess) {
        printf("MQTT doorbell ring event sent to %s\n", topic);
    } else {
        printf("MQTT publish failed with status: %d\n", status);
        mqttConnected = false;  // Mark as disconnected on failure
    }
}

int32_t MQTT_SendDoorbellRing(void) {
    mqtt_doorbell_send_ring_event();
    return 0;
}

int32_t mqtt_doorbell_ping(void) {
    if (!mqttConnected) {
        printf("MQTT not connected - cannot ping\n");
        return -1;
    }
    
    MQTTStatus_t status = MQTT_Ping(&mqttContext);
    if (status == MQTTSuccess) {
        printf("MQTT ping sent successfully\n");
        return 0;
    } else {
        printf("MQTT ping failed: %d\n", status);
        mqttConnected = false;  // Mark as disconnected on failure
        return -1;
    }
}

void mqtt_doorbell_cleanup(void) {
    MQTT_Disconnect(&mqttContext);
    TLS_FreeRTOS_Disconnect(&tlsContext);
}
