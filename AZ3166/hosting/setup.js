// ----------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//  Licensed under the MIT license.
// ----------------------------------------------------------------------------

const fs     = require('fs');
const path   = require('path');

var configs = {
  "WIFI_NAME":  "put the WiFi network name here and uncomment",
  "WIFI_PASSWORD": "put the WiFi network password here and uncomment",

  "IOT_CENTRAL_CONNECTION_STRING": "put the iot central connect string here",

  "DISABLE_HUMIDITY": 0,
  "DISABLE_TEMPERATURE": 0,
  "DISABLE_SHAKE": 0,
  "DISABLE_GYROSCOPE": 0,
  "DISABLE_ACCELEROMETER": 0,
  "DISABLE_MAGNETOMETER": 0,
  "DISABLE_PRESSURE": 0
};

if (process.argv.length < 3) {
  console.log('usage: node setup.js <preprocessor list> \n');
  for(var o in configs) {
    if (!configs.hasOwnProperty(o)) continue;
    console.log(o)
    if (configs[o] != 0) {
      console.log('\t', configs[o])
    }
  }
  console.log("\ni.e. node setup.js WIFI_NAME=\"Ozzzz\" WIFI_PASSWORD=\"password\" DISABLE_SHAKE");
  console.log("");
  process.exit(1);
}

configs.WIFI_NAME = 0;
configs.WIFI_PASSWORD = 0;
configs.IOT_CENTRAL_CONNECTION_STRING = 0;

for (var i = 2; i < process.argv.length; i++) {
  var arg = process.argv[i];
  arg = arg.split('=');
  if (configs.hasOwnProperty(arg[0])) {
    configs[arg[0].replace(/"/g, "")] = arg.length > 1 ? arg[1] : 1;
    if (arg.length > 2) {
      for (var j = 2; j < arg.length; j++) {
        configs[arg[0].replace(/"/g, "")] += "=" + arg[j]
      }
    }
  }
}

var pathToDefinitions = path.join(__dirname, 'inc');

if (!fs.existsSync(pathToDefinitions)) {
  console.error('\n - error', 'inc/ folder is not on the relative path');
  process.exit(1);
}

var defs = [];
for(var o in configs) {
  if (!configs.hasOwnProperty(o)) continue;
  if (configs[o] == 0) continue;

  var line = '#define ' + o;
  if (configs[o] != 0) {
    line += '  \"' + configs[o] + '\"'
  }
  defs.push(line);
}

if (defs.length == 0) {
  // unlikely but don't do anything
  console.log('\n - warning: nothing to update...')
  process.exit(1)
}
defs.push('#define COMPILE_TIME_DEFINITIONS_SET');

fs.writeFileSync(path.join(pathToDefinitions, 'definitions.h'), defs.join('\n'));

console.log('\ncurrent config is written to inc/definitions.h;\n')
console.log(defs.join('\n'));
console.log('\ndone!');