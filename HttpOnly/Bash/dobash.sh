#/bin/bash

DEVICE_ID="ENTER DEVICE ID HERE"
DEVICE_KEY="ENTER DEVICE SYM KEY HERE"
SCOPE="ENTER SCOPE ID HERE"

MESSAGE='{"temp":15}'

function getAuth {
  # A node script that prints an auth signature using SCOPE, DEVICE_ID and DEVICE_KEY
  AUTH=`node -e "\
  const crypto   = require('crypto');\

  function computeDrivedSymmetricKey(masterKey, regId) {\
    return crypto.createHmac('SHA256', Buffer.from(masterKey, 'base64'))\
      .update(regId, 'utf8')\
      .digest('base64');\
  }\
  \
  var expires = parseInt((Date.now() + (7200 * 1000)) / 1000);\
  var sr = '${SCOPE}%2f${TARGET}%2f${DEVICE_ID}';\
  var sigNoEncode = computeDrivedSymmetricKey('${DEVICE_KEY}', sr + '\n' + expires);\
  var sigEncoded = encodeURIComponent(sigNoEncode);\
  console.log('SharedAccessSignature sr=' + sr + '&sig=' + sigEncoded + '&se=' + expires)\
  "`
}

SCOPEID="$SCOPE"
TARGET="registrations"
# get auth for Azure IoT DPS service
getAuth

# use the Auth and make a PUT request to Azure IoT DPS service
OUT=`curl \
  -H "authorization: ${AUTH}&skn=registration" \
  -H 'content-type: application/json; charset=utf-8' -H 'content-length: 25' \
  -s \
  --request PUT --data "{\"registrationId\":\"$DEVICE_ID\"}" "https://global.azure-devices-provisioning.net/$SCOPEID/registrations/$DEVICE_ID/register?api-version=2018-11-01"`

OPERATION=`node -e "c=JSON.parse('$OUT');if(c.errorCode){console.log(c);process.exit(1);}console.log(c.operationId)"`

if [[ $? != 0 ]]; then
  # if there was an error, print the stdout part and exit
  echo "$OPERATION"
  exit
else
  echo "Authenticating.."
  # wait 2 secs before making the GET request to Azure IoT DPS service
  # the second call will bring us the Azure IoT Hub endpoint we are supposed to talk
  sleep 2

  OUT=`curl -s \
  -H "authorization: ${AUTH}&skn=registration" \
  -H "content-type: application/json; charset=utf-8" \
  --request GET "https://global.azure-devices-provisioning.net/$SCOPEID/registrations/$DEVICE_ID/operations/$OPERATION?api-version=2018-11-01"`

  # parse the return value from the DPS host and try to grab the assigned hub
  OUT=`node -pe "a=JSON.parse('$OUT');if(a.errorCode){a}else{a.registrationState.assignedHub}"`

  if [[ $OUT =~ 'errorCode' ]]; then
    # if there was an error, print the stdout part and exit
    echo "$OUT"
    exit
  fi

  TARGET="devices"
  SCOPE="$OUT"
  # get Auth for Azure IoThub service
  getAuth

  echo "OK"
  echo

  echo "SENDING => " $MESSAGE

  # send a telemetry to Azure IoT hub service
  curl -s \
  -H "authorization: ${AUTH}" \
  -H "iothub-to: /devices/$DEVICE_ID/messages/events" \
  --request POST --data "$MESSAGE" "https://$SCOPE/devices/$DEVICE_ID/messages/events/?api-version=2016-11-14"

  echo "DONE"
fi

echo
