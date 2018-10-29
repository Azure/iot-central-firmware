# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license.

import httplib2 as http
import urllib
import json
import hashlib
import hmac
import base64
import calendar
import time

try:
    from urlparse import urlparse
except ImportError:
    from urllib.parse import urlparse

EXPIRES = 7200 # 2 hours

def computeDrivedSymmetricKey(secret, regId):
  secret = bytes(secret).decode('base64')
  return base64.b64encode(hmac.new(secret, regId, digestmod=hashlib.sha256).digest())


def loopAssign(operationId, headers, deviceId, scopeId, deviceKey, callback):
  uri = "https://global.azure-devices-provisioning.net/%s/registrations/%s/operations/%s?api-version=2018-09-01-preview" % (scopeId, deviceId, operationId)

  target = urlparse(uri)
  method = 'GET'

  h = http.Http()
  response, content = h.request(
          target.geturl(),
          method,
          "",
          headers)
  data = json.loads(content)

  if 'status' in data:
    if data['status'] == 'assigning':
      loopAssign(operationId, headers, deviceId, scopeId, deviceKey, callback)
      return
    elif data['status'] == "assigned":
      state = data['registrationState']
      hub = state['assignedHub']
      callback(None, "HostName=" + hub + ";DeviceId=" + deviceId + ";SharedAccessKey=" + deviceKey)
      return

  callback(data)

def getConnectionString(deviceId, mkey, scopeId, isMasterKey, callback):
  global EXPIRES
  body = "{\"registrationId\":\"%s\"}" % (deviceId)
  expires = calendar.timegm(time.gmtime()) + EXPIRES

  deviceKey = mkey
  if isMasterKey:
    deviceKey = computeDrivedSymmetricKey(mkey, deviceId)

  sr = scopeId + "%2fregistrations%2f" + deviceId;
  sigNoEncode = computeDrivedSymmetricKey(deviceKey, sr + "\n" + str(expires))
  sigEncoded = urllib.quote(sigNoEncode, safe='~()*!.\'')

  authString = "SharedAccessSignature sr=" + sr + "&sig=" + sigEncoded + "&se=" + str(expires) + "&skn=registration"
  print authString
  headers = {
    "Accept": "application/json",
    "Content-Type": "application/json; charset=utf-8",
    "Connection": "keep-alive",
    "UserAgent": "prov_device_client/1.0",
    "Authorization" : authString
  }

  uri = "https://global.azure-devices-provisioning.net/%s/registrations/%s/register?api-version=2018-09-01-preview" % (scopeId, deviceId)
  target = urlparse(uri)
  method = 'PUT'

  h = http.Http()
  response, content = h.request(
          target.geturl(),
          method,
          body,
          headers)
  data = json.loads(content)

  if 'errorCode' in data:
    callback(data, None)
  else:
    loopAssign(data['operationId'], headers, deviceId, scopeId, "KEY", callback)


### USAGE

# def myCallback(error, connstr):
#   if error != None:
#     print error
#   else:
#     print "CONNECTION STRING=" + connstr

# deviceId = "DEVICE_ID"
# scopeId = "SCOPE_ID"
# mkey = "MASTER Primary OR Secondary key or DEVICE Primary or Secondary key"

# getConnectionString(deviceId, mkey, scopeId, True, myCallback)