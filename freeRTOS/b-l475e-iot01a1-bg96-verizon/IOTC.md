## iotc - Azure IoT - C/CPP (light) device SDK Documentation

### Supported platforms

IoT platforms with 8KB+ available dynamic memory. (arduino / mbed / rtos ....)

**Some platforms may require an additional configuration**

It comes with builtin

- [JSON parser](azure_iotc_sample/lib/iotc/common/iotc_json.h)
- [String buffer](azure_iotc_sample/lib/iotc/common/string_buffer.h)

### API

```
// Set the level of logging (see the options above)
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_logging(IOTLogLevel level);
```

```
// Initialize the device context. The context variable will be used by rest of the API
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_init_context(IOTContext *ctx);
```

```
// Free device context.
// Call this after `init_context`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_free_context(IOTContext ctx);
```

```
// Connect to Azure IoT Central
// Call this after `init_context`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_connect(IOTContext ctx, const char* scope, const char* keyORcert,
                                 const char* deviceId, IOTConnectType type);
```

```
// Disconnect
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_disconnect(IOTContext ctx);
```

```
// If your endpoint is different than the default AzureIoTCentral endpoint, set it
// using this API.
// Call this before `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_global_endpoint(IOTContext ctx, const char* endpoint_uri);
```

```
// Set the custom certificates for custom endpoints
// Call this before `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_trusted_certs(IOTContext ctx, const char* certs);
```

```
// Set the proxy settings
// Call this before `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_set_proxy(IOTContext ctx, IOTC_HTTP_PROXY_OPTIONS proxy);
```

```
// Sends a telemetry payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_telemetry(IOTContext ctx, const char* payload, unsigned length);
```

```
// Sends a telemetry payload (JSON) with iothub system properties
// see also;
// https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-construct

// Call this after `connect`

// i.e. => iotc_send_telemetry_with_system_properties(
//                          ctx, "{\"temp\":22}", 11,
//                          "iothub-creation-time-utc=1534434323&message-id=abc123", 53);

// returns 0 if there is no error. Otherwise, error code will be returned.

int iotc_send_telemetry_with_system_properties(IOTContext ctx,
                                               const char* payload,
                                               unsigned length,
                                               const char* sysPropPayload,
                                               unsigned sysPropPayloadLength);
```

```
// Sends a state payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_state    (IOTContext ctx, const char* payload, unsigned length);
```

```
// Sends an event payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_event    (IOTContext ctx, const char* payload, unsigned length);
```

```
// Sends a property payload (JSON)
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_send_property (IOTContext ctx, const char* payload, unsigned length);
```

```
/*
eventName:
  ConnectionStatus
  MessageSent
  Command
  SettingsUpdated
  Error
*/
typedef void(*IOTCallback)(IOTContext, IOTCallbackInfo*);

// Register to one of the events listed above
// Call this after `init_context`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_on(IOTContext ctx, const char* eventName, IOTCallback callback, void* appContext);
```

```
// Lets SDK to do background work
// Call this after `connect`
// returns 0 if there is no error. Otherwise, error code will be returned.
int iotc_do_work(IOTContext ctx);
```

```
// Provide platform dependent NetworkInterface
int iotc_set_network_interface(void* networkInterface);
```
