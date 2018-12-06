using System;
using System.IO;
using System.Security.Cryptography;
using System.Net;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using TinyJson;

public class IoTCentralDemo
{
  string scopeId = "Enter SCOPE ID here";
  string deviceId = "Enter DEVICE ID here";
  string deviceKey = "Enter DEVICE KEY here";

  string computeHash(string key, string Id) {
    HMACSHA256 hmac = new HMACSHA256(System.Convert.FromBase64String(key));
    return System.Convert.ToBase64String(hmac.ComputeHash(Encoding.ASCII.GetBytes(Id)));
  }

  string getAuth(string scope, string target) {
    long utcTimeInMilliseconds = DateTime.UtcNow.Ticks / 10000;
    long expires = ((utcTimeInMilliseconds + (7200 * 1000)) / 1000);
    string sr = scope + "%2f" + target + "%2f" + deviceId;
    string sigNoEncode = computeHash(deviceKey, sr + '\n' + expires.ToString());
    string sigEncoded = Uri.EscapeDataString(sigNoEncode);
    return "SharedAccessSignature sr=" + sr + "&sig=" + sigEncoded + "&se=" + expires.ToString();
  }

  public string MakeRequest(string method, string payload, string authScope, string address, bool isDPS)
  {
    HttpWebRequest request = (HttpWebRequest)WebRequest.Create(address);
    request.Method = method;
    request.ContentType = "application/json";
    string authTarget = "";
    string reason = "";
    if (isDPS) {
      reason = "registrations";
      authTarget = "&skn=registration";
    } else {
      reason = "devices";
      request.Headers.Add("iothub-to", "/devices/" + deviceId + "/messages/events");
    }

    request.Headers.Add("Authorization", getAuth(authScope, reason) + authTarget);

    if (payload.Length > 0) {
      request.ContentLength = payload.Length;
      Stream dataStream = request.GetRequestStream();
      dataStream.Write(Encoding.ASCII.GetBytes(payload), 0, payload.Length);
      dataStream.Close();
    }

    HttpWebResponse response = (HttpWebResponse)request.GetResponse();
    Stream receiveStream = response.GetResponseStream();
    Encoding encode = System.Text.Encoding.GetEncoding("utf-8");
    StreamReader readStream = new StreamReader( receiveStream, encode );
    return readStream.ReadToEnd(); // bad hack
  }

  public void Run() {
    // get operationId from Azure IoT DPS
    string address = "https://global.azure-devices-provisioning.net/" + scopeId + "/registrations/" + deviceId + "/register?api-version=2018-11-01";
    string json = MakeRequest("PUT", "{\"registrationId\":\"" + deviceId + "\"}", scopeId, address, true);
    var obj = json.FromJson<object>();
    Dictionary<string,object> dict = (Dictionary<string,object>)obj;

    if (dict.ContainsKey("errorCode")) {
      Console.WriteLine("ERROR: " + json);
      return;
    }

    string operationId = dict["operationId"].ToString();

    // Using the operationId and the rest of the credentials, get hostname for the Azure IoT hub
    // The reason for the loop is that the async op. we had triggered with the operationId request
    // may take more than 2secs.
    // So, wait for 2 secs+ each time and then try again
    for (int i = 0; i < 10; i++) // try 5 times
    {
      System.Threading.Thread.Sleep(2500);
      address = "https://global.azure-devices-provisioning.net/" + scopeId + "/registrations/" + deviceId + "/operations/" + operationId + "?api-version=2018-11-01";
      json = MakeRequest("GET", "", scopeId, address, true);
      obj = json.FromJson<object>();
      dict = (Dictionary<string,object>)obj;

      if (dict.ContainsKey("errorCode")) {
        Console.WriteLine("ERROR: " + json);
        return;
      }
      if (dict.ContainsKey("registrationState")) {
        if ( ((Dictionary<string,object>)dict["registrationState"]).ContainsKey("assignedHub"))
          break;
      }
    }

    string hostName = ((Dictionary<string,object>)dict["registrationState"])["assignedHub"].ToString();


    // finally, send our temperature telemetry to Azure IoT Hub
    address = "https://" + hostName + "/devices/" + deviceId + "/messages/events/?api-version=2016-11-14";
    json = MakeRequest("POST", "{\"temp\" : 15}", hostName, address, false);

    if (json.Length == 0) {
      Console.WriteLine("Success!! Check the new temperature telemetry on Azure IoT Central Device window");
    } else {
      Console.WriteLine(json);
    }
  }

  static void Main() {
    IoTCentralDemo demo = new IoTCentralDemo();
    demo.Run();
  }
}