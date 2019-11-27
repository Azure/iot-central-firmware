using System;
using System.Threading.Tasks;

namespace iotCentral.Http
{
    public class IoTCentralDemo
    {
        private readonly IRequestHelper requestHelper;
        string scopeId = "ENTER SCOPE ID HERE";
        string deviceId = "ENTER DEVICE ID HERE";
        string deviceKey = "ENTER DEVICE KEY HERE=";

        public IoTCentralDemo(IRequestHelper requestHelper)
        {
            this.requestHelper = requestHelper;
        }

        public async Task Run()
        {
            // get operationId from Azure IoT DPS
            var result = await requestHelper.AddRegistrationAsync("{\"registrationId\":\"" + deviceId + "\"}", scopeId, deviceId, deviceKey);
            
            //todo check on "error"

            if (result.status == DeviceEnrollmentStatus.assigning) {
                // Using the operationId and the rest of the credentials, get hostname for the Azure IoT hub
                // The reason for the loop is that the async op. we had triggered with the operationId request
                // may take more than 2secs.
                // So, wait for 2 secs+ each time and then try again
                for (int i = 0; i < 10; i++) // try 5 times
                {
                    System.Threading.Thread.Sleep(2500);
                    result = await requestHelper.GetRegistrationAsync(scopeId, result.operationId, deviceId, deviceKey);
                    
                    //todo check on "error"

                    if (result.registrationState != null)
                    {
                        if (result.registrationState.assignedHub != null)
                            break;
                    }
                }
            }
            if (!string.IsNullOrEmpty(result.registrationState.errorMessage))
            {
                Console.WriteLine("ERROR: " + result.registrationState.errorMessage);
                return;
            }

            var successfullySentTelemetry = await requestHelper.PostTelemetryAsync("{\"temp\" : 15}", result.registrationState.assignedHub, deviceId, deviceKey);
            
            if (successfullySentTelemetry)
            {
                Console.WriteLine("Success!! Check the new temperature telemetry on Azure IoT Central Device window");
            }
            else
            {
                Console.WriteLine("Unable to send telemetry.");
            }
            
        }
    }
}
