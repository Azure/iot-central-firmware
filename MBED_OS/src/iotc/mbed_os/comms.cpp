// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if defined(__MBED__)
#include "../common/iotc_platform.h"
#if defined(USE_LIGHT_CLIENT)
#include "../common/json.h"
#include "../common/iotc_internal.h"

int mqtt_publish(IOTContextInternal *internal, const char *topic, unsigned long topic_length,
                 const char *msg, unsigned long msg_length)
{

    MQTT::Message message;
    message.retained = false;
    message.dup = false;

    message.payload = (void *)msg;
    message.payloadlen = msg_length;

    message.qos = MQTT::QOS0;
    message.id = ++internal->messageId;
    int rc = internal->mqttClient->publish(topic, message);
    if (rc != MQTT::SUCCESS)
    {
        return rc;
    }
    return iotc_do_work(internal);
}

#endif // defined(USE_LIGHT_CLIENT)
#endif // defined(__MBED__)
