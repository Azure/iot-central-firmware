using System;
using System.Threading.Tasks;
using Microsoft.Extensions.Configuration;

namespace iotCentral.Http
{
    public class IoTCentralDemo
    {
        private readonly IRequestHelper requestHelper;
        string scopeId = "";
        string deviceId = "";
        string deviceKey = "";

        public IoTCentralDemo(IRequestHelper requestHelper, IConfiguration config)
        {
            this.requestHelper = requestHelper;
            this.scopeId = config["scopeId"];
            this.deviceId = config["deviceId"];
            this.deviceKey = config["deviceKey"];
        }

        public async Task Run()
        {
            // get operationId from Azure IoT DPS
            var result = await requestHelper.AddRegistrationAsync("{\"registrationId\":\"" + deviceId + "\"}", scopeId, deviceId, deviceKey);
            
            if (result.status == DeviceEnrollmentStatus.assigning) {
                // Using the operationId and the rest of the credentials, get hostname for the Azure IoT hub
                // The reason for the loop is that the async op. we had triggered with the operationId request
                // may take more than 2secs.
                // So, wait for 2 secs+ each time and then try again
                for (int i = 0; i < 10; i++) // try 5 times
                {
                    System.Threading.Thread.Sleep(2500);
                    result = await requestHelper.GetRegistrationAsync(scopeId, result.operationId, deviceId, deviceKey);
                    
                    if (result.registrationState != null && 
                        result.registrationState.assignedHub != null)
                    {    
                        break;
                    }
                }
            }
            if (!string.IsNullOrEmpty(result.registrationState.errorMessage))
            {
                Console.WriteLine($"ERROR: {result.registrationState.errorMessage}");
                return;
            }

            bool successfullySentTelemetry = true;
            do {
                var random = (new Random().NextDouble()*3d) + 15d;
                successfullySentTelemetry = await requestHelper.PostTelemetryAsync($"{{\"temp\" : {random}}}", result.registrationState.assignedHub, deviceId, deviceKey);
                
                if (successfullySentTelemetry)
                {
                    Console.WriteLine($"Success!! Sent to IoT Central {{\"temp\" : {random}}}");
                }
                else
                {
                    Console.WriteLine("Unable to send telemetry.");
                }
            } while (successfullySentTelemetry);
            
        }
    }
}
