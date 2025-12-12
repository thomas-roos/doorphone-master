#include <stdio.h>
#include <string.h>
#include <time.h>
#include "FreeRTOS.h"
#include "task.h"
#include "mqtt_doorbell.h"

#define MQTT_TOPIC              "doorbell/doorbell-channel/ring"

int32_t MQTT_Init(void)
{
    printf("MQTT doorbell client initialized (simulation mode)\n");
    return 0;
}

int32_t MQTT_SendDoorbellRing(void)
{
    // Simulate MQTT message - in real implementation this would connect to AWS IoT
    char message[256];
    snprintf(message, sizeof(message), 
             "{\"event\":\"ring\",\"channel\":\"doorbell-channel\",\"timestamp\":%ld}", 
             (long)time(NULL));
    
    printf("📡 MQTT Publish to %s: %s\n", MQTT_TOPIC, message);
    
    // Here you would implement actual MQTT publish to AWS IoT Core
    // For now, just simulate successful send
    return 0;
}
