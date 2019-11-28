using System.Net.Http;
using System.Net.Http.Headers;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace iotCentral.Http
{
    public class RequestHelper : IRequestHelper {
        private readonly IHttpClientFactory factory;
        private JsonSerializerOptions options;

        public RequestHelper(IHttpClientFactory factory)
        {
            this.factory = factory;
            this.options = new JsonSerializerOptions();
            this.options.Converters.Add(new JsonStringEnumConverter(JsonNamingPolicy.CamelCase));
        }
        public async Task<RegistrationOperationStatus> AddRegistrationAsync(string payload, string authScope, string deviceId, string deviceKey)
        {
            string address = $"https://global.azure-devices-provisioning.net/{authScope}/registrations/{deviceId}/register?api-version=2018-11-01";
            
            var httpClient = factory.CreateClient();
            var requestBody = new StringContent(payload);
            requestBody.Headers.ContentType = new MediaTypeHeaderValue("application/json");

            string authTarget = "&skn=registration";
            string reason = "registrations";
            httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("SharedAccessSignature", AuthHelper.GetAuthenticationValue(authScope, reason, deviceId, deviceKey) + authTarget);

            var response = await httpClient.PutAsync(address, requestBody);

            return await JsonSerializer.DeserializeAsync<RegistrationOperationStatus>(
                await response.Content.ReadAsStreamAsync(), this.options);
        }

        public async Task<RegistrationOperationStatus> GetRegistrationAsync(string authScope, string operationId, string deviceId, string deviceKey)
        {
            string address = $"https://global.azure-devices-provisioning.net/{authScope}/registrations/{deviceId}/operations/{operationId}?api-version=2018-11-01";

            var httpClient = factory.CreateClient();
            string authTarget = "&skn=registration";
            string reason = "registrations";
            httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("SharedAccessSignature", AuthHelper.GetAuthenticationValue(authScope, reason, deviceId, deviceKey) + authTarget);
            var response = await httpClient.GetAsync(address);
            
            return await JsonSerializer.DeserializeAsync<RegistrationOperationStatus>(
                await response.Content.ReadAsStreamAsync(), this.options);
        }

        public async Task<bool> PostTelemetryAsync(string payload, string hostName, string deviceId, string deviceKey) 
        {
            string address = $"https://{hostName}/devices/{deviceId}/messages/events/?api-version=2016-11-14";
            var httpClient = factory.CreateClient();
            var requestBody = new StringContent(payload);
            requestBody.Headers.ContentType = new MediaTypeHeaderValue("application/json");

            string reason = "devices";
            httpClient.DefaultRequestHeaders.Add("iothub-to", $"/devices/{deviceId}/messages/events");
            httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("SharedAccessSignature", AuthHelper.GetAuthenticationValue(hostName, reason, deviceId, deviceKey));
            var response = await httpClient.PostAsync(address, requestBody);

            return response.IsSuccessStatusCode;
        }
    }
}