using System;
using System.Security.Cryptography;
using System.Text;

namespace iotCentral.Http
{
    public static class AuthHelper {
        public static string GetAuthenticationValue(string scope, string target, string deviceId, string deviceKey)
        {
            long utcTimeInMilliseconds = DateTime.UtcNow.Ticks / 10000;
            long expires = ((utcTimeInMilliseconds + (7200 * 1000)) / 1000);
            string sr = $"{scope}%2f{target}%2f{deviceId}";
            string sigNoEncode = ComputeHash(deviceKey, $"{sr}\n{expires.ToString()}");
            string sigEncoded = Uri.EscapeDataString(sigNoEncode);
            return $"sr={sr}&sig={sigEncoded}&se={expires.ToString()}";
        }
        
        private static string ComputeHash(string key, string Id)
        {
            HMACSHA256 hmac = new HMACSHA256(System.Convert.FromBase64String(key));
            return System.Convert.ToBase64String(hmac.ComputeHash(Encoding.ASCII.GetBytes(Id)));
        }
    }
}
