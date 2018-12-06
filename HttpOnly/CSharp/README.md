### Connecting to Azure IoT Central via HTTP (C#)

To Run;
```
csc app.cs JSONParser.cs
```

and then execute `app.exe`

Don't forget filling the required variables under `app.cs`
```
string scopeId = "ENTER SCOPE ID HERE";
string deviceId = "ENTER DEVICE ID HERE";
string deviceKey = "ENTER DEVICE KEY HERE=";
```

Open [apps.azureiotcentral.com](apps.azureiotcentral.com) `>` Device Explorer (explorer)
`>` Select one of the templates (i.e. MXCHIP) `>` Click to `+` button on the tab `>`
Select `Real device`

You should see a `Connect` link on top-right of the device page. Once you click
to that button, you will find all the `deviceId`, `deviceKey` (Primary_key) and
`scopeId` there.