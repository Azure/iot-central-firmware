// Copyright (c) Oguz Bastemur. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "../common/iotc_platform.h"
#if defined(ARDUINO) && defined(USE_LIGHT_CLIENT)
#include "../common/json.h"
#include "../common/iotc_internal.h"
#include <SPI.h>
#include <stdarg.h>

int mqtt_publish(IOTContextInternal *internal, const char* topic, unsigned long topic_length,
    const char* msg, unsigned long msg_length) {

    if (!internal->mqttClient->publish(topic, (const uint8_t*)msg, msg_length, false)) {
        return 1;
    }
    return 0;
}

void IOTC_LOG(const __FlashStringHelper *format, ...) {
    if (getLogLevel() > IOTC_LOGGING_DISABLED) {
        va_list ap;
        va_start(ap, format);
        AzureIOT::StringBuffer buffer(STRING_BUFFER_1024);
        buffer.setLength(vsnprintf(*buffer, STRING_BUFFER_1024, (const char *)format, ap));
        Serial.println(*buffer);
        va_end(ap);
    }
}

#endif // defined(ARDUINO) && defined(USE_LIGHT_CLIENT)
