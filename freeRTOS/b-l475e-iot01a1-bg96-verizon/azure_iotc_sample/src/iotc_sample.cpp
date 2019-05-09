/* Standard includes. */
#include "stdio.h"
#include "string.h"

#include "iotc_sample.h"
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"

#include "../lib/iotc/iotc.h"
#include "iotc_json.h"
#include "string_buffer.h"

#define SCOPE_ID "IOT_CENTRAL_SCOPE_ID_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define DEVICE_ID "IOT_CENTRAL_REGISTRATION_ID_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define DEVICE_KEY "IOT_CENTRAL_SAS_KEY_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

#define STRING_BUFFER_256 256
static int isConnected = 0;
static int triggerCountdown = -1;

// iotc mqtt client connect status updates
void onConnectionStatus(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  LOG_VERBOSE("Is connected  ? %s (%d)",
              callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
              callbackInfo->statusCode);
  isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK ? 1 : 0;
}

// updates to device settings (desired properties)
void onSettingsUpdated(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  LOG_VERBOSE("Received a SettingsUpdated event");
  jsobject_t object;
  jsobject_initialize(&object, callbackInfo->payload,
                      callbackInfo->payloadLength);

  if (strcmp(callbackInfo->tag, "setVoltage") == 0) {
    jsobject_t voltage;
    jsobject_get_object_by_name(&object, "setVoltage", &voltage);
    int v = jsobject_get_number_by_name(&voltage, "value");
    LOG_VERBOSE("== Received a 'Voltage' update! New Value => %d", v);
    jsobject_free(&voltage);
  } else if (strcmp(callbackInfo->tag, "fanSpeed") == 0) {
    jsobject_t fan;
    jsobject_get_object_by_name(&object, "fanSpeed", &fan);
    int fanSpeed = jsobject_get_number_by_name(&fan, "value");
    LOG_VERBOSE("== Received a 'FanSpeed' update! New Value => %d", fanSpeed);
    jsobject_free(&fan);
  } else if (strcmp(callbackInfo->tag, "setCurrent") == 0) {
    jsobject_t current;
    jsobject_get_object_by_name(&object, "setCurrent", &current);
    int currentValue = (int)jsobject_get_number_by_name(&current, "value");
    LOG_VERBOSE("== Received a 'Current' update! New Value => %d",
                currentValue);
    jsobject_free(&current);
  } else if (strcmp(callbackInfo->tag, "activateIR") == 0) {
    jsobject_t ir;
    jsobject_get_object_by_name(&object, "activateIR", &ir);
    int irOn = (int)jsobject_get_number_by_name(&ir, "activateIR");
    LOG_VERBOSE("== Received a 'IR' update! New Value => %s",
                irOn == 1 ? "IR ON" : "IR OFF");
    jsobject_free(&ir);
  } else {
    // payload may not have a null ending
    LOG_VERBOSE("Unknown Settings. Payload => %.*s",
                callbackInfo->payloadLength, callbackInfo->payload);
  }
  jsobject_free(&object);
}

// cloud to device command
void onCommand(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  LOG_VERBOSE("Received a Cloud to Device Command event");
  jsobject_t object;
  jsobject_initialize(&object, callbackInfo->payload,
                      callbackInfo->payloadLength);

  if (strcmp(callbackInfo->tag, "echo") == 0) {
    char* value = jsobject_get_string_by_name(&object, "displayedValue");
    LOG_VERBOSE("ECHO command was received with a message %s", value);
    IOTC_FREE(value);
  } else if (strcmp(callbackInfo->tag, "countdown") == 0) {
    int number = (int)jsobject_get_number_by_name(&object, "countFrom");
    LOG_VERBOSE("COUNTDOWN command was received with a countdown %d", number);
    if (number < 0) number = 0;
    triggerCountdown = number;
  } else {
    // payload may not have a null ending
    LOG_VERBOSE("Unknown Command. Payload => %.*s",
                callbackInfo->payloadLength, callbackInfo->payload);
  }
  jsobject_free(&object);
}

// other events
void onEvent(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  LOG_VERBOSE("Received a '%s' event", callbackInfo->eventName);
}

static void iotc_main(void* pvParameters) {
  IOTContext context = NULL;

  // triggerCountdown < 0 condition satisfies only once in whole app lifetime
  if (triggerCountdown < 0) {
    triggerCountdown = 0;
    BSP_ACCELERO_Init();
    BSP_MAGNETO_Init();
    BSP_TSENSOR_Init();
    BSP_HSENSOR_Init();
    BSP_GYRO_Init();
    BSP_PSENSOR_Init();
    BL_LED_Init(BL_LED_GREEN);
  }

  // wait for modem to come online
  while (isBG96Online() == 0) {
    vTaskDelay(1000);
  }

  LOG_VERBOSE("modem is online");

  // initialize iotc device context
  int errorCode = iotc_init_context(&context);
  if (errorCode != 0) {
    LOG_VERBOSE("Error initializing IOTC. Code %d", errorCode);
    return;
  }

  iotc_set_logging(IOTC_LOGGING_API_ONLY);

  // setup the event callbacks
  iotc_on(context, "MessageSent", onEvent, NULL);
  iotc_on(context, "Command", onCommand, NULL);
  iotc_on(context, "ConnectionStatus", onConnectionStatus, NULL);
  iotc_on(context, "SettingsUpdated", onSettingsUpdated, NULL);
  iotc_on(context, "Error", onEvent, NULL);

  LOG_VERBOSE("Connecting to Azure IoT");
  // connect to azure iot
  errorCode = iotc_connect(context, SCOPE_ID, DEVICE_KEY, DEVICE_ID,
                           IOTC_CONNECT_SYMM_KEY);
  if (errorCode != 0) {
    LOG_VERBOSE("Error @ iotc_connect. Code %d", errorCode);
    return;
  }

  StringBuffer msg(STRING_BUFFER_256);

  int telemetryCounter = 2;
  int reconnectCounter = 0;
  int messageCounter = 0;

  // application lifecycle loop
  while (true) {
    while (isBG96Online() != 1) {
      logCellInfo("modem is reconnecting..", 0);
      vTaskDelay(5000);  // 5 secs
      if (reconnectCounter++ > 12)
        break;  // reset the thread if modem wasn't able to connect up to a
                // minute
    }

    if (telemetryCounter >= 2) {
      telemetryCounter = 0;

      int16_t magData[3], accData[3];
      float gyroData[3];
      BSP_MAGNETO_GetXYZ(magData);
      BSP_ACCELERO_AccGetXYZ(accData);
      BSP_GYRO_GetXYZ(gyroData);
      float pressure = BSP_PSENSOR_ReadPressure();
      float temperature = BSP_TSENSOR_ReadTemp();
      float humidity = BSP_HSENSOR_ReadHumidity();

      // create a JSON telemetry

      /* Emulated Fixed point version */
      int temperature_decimal = (int)(temperature * 10) % 10;
      int humidity_decimal = (int)(humidity * 100) % 100;
      // create a telemetry message
      msg.setLength(snprintf(
          *msg, STRING_BUFFER_256,
          "{\"pressure\":%d,\"magnetometerX\":%d,\"magnetometerY\":%d,\"magnetometerZ\":%d,\
 \"gyroscopeX\":%d,\"gyroscopeY\":%d,\"gyroscopeZ\":%d,\"accelerometerX\":%d,\
 \"accelerometerY\":%d,\"accelerometerZ\":%d,\"temp\":%d.%d,\"humidity\":%d.%d}",
          (int)pressure, (int)magData[0], (int)magData[1], (int)magData[2],
          (int)gyroData[0], (int)gyroData[1], (int)gyroData[2], (int)accData[0],
          (int)accData[1], (int)accData[2], (int)temperature,
          temperature_decimal, (int)humidity, humidity_decimal));

      messageCounter++;

      // send telemetry to azure iot central
      if ((errorCode = iotc_send_telemetry(context, *msg, msg.getLength())) !=
          0) {
        LOG_VERBOSE("Error @ iotc_send_telemetry (%s). Code %d", *msg,
                    errorCode);
        break;
      }
      LOG_VERBOSE("Telemetry sent [%d]\r\n%s", messageCounter, *msg);
    }

    iotc_do_work(context); // do background work

    if (isConnected) {
      if (triggerCountdown > 0) BL_LED_On(BL_LED_GREEN);
      vTaskDelay(500);
      if (triggerCountdown > 0) {
        msg.setLength(snprintf(*msg, STRING_BUFFER_256,
                              "{\"countdown\":{\"value\": %d}}",
                              triggerCountdown--));
        if ((errorCode = iotc_send_property(context, *msg, msg.getLength())) !=
            0) {
          LOG_VERBOSE("Error @ iotc_send_property (%s). Code %d", *msg,
                      errorCode);
          break;
        }
      }
      if (triggerCountdown > 0) BL_LED_Off(BL_LED_GREEN);
      vTaskDelay(500);

      telemetryCounter++;
    } else {
      break;
    }
  }

  iotc_free_context(context);
  context = NULL;
  iotc_create_task();
  vTaskDelete(NULL);
}
/*-----------------------------------------------------------*/

extern "C" {
void iotc_create_task() {
  xTaskCreate(iotc_main, "IOTCmain", configMINIMAL_STACK_SIZE * 14, NULL,
              tskIDLE_PRIORITY, NULL);
}
}
