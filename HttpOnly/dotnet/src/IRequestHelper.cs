using System.Threading.Tasks;

namespace iotCentral.Http
{
    public interface IRequestHelper
    {
        Task<RegistrationOperationStatus> AddRegistrationAsync(string payload, string authScope, string deviceId, string deviceKey);
        Task<RegistrationOperationStatus> GetRegistrationAsync(string authScope, string operationId, string deviceId, string deviceKey);
        Task<bool> PostTelemetryAsync(string payload, string hostName, string deviceId, string deviceKey);
    }
}
