#ifndef MQTT_DOORBELL_H
#define MQTT_DOORBELL_H

#include <stdint.h>

/**
 * @brief Initialize MQTT client for doorbell functionality
 * @return 0 on success, -1 on failure
 */
int32_t mqtt_doorbell_init(void);

/**
 * @brief Send doorbell ring event via MQTT
 * @return 0 on success, -1 on failure
 */
int32_t MQTT_SendDoorbellRing(void);

#endif /* MQTT_DOORBELL_H */
