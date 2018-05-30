// ----------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//  Licensed under the MIT license.
// ----------------------------------------------------------------------------

const https = require('https');
const fs   = require('fs');
const path = require('path');
const cmd = require('node-cmd');
var connect = require('connect');
var serveStatic = require('serve-static');

const hostname = '0.0.0.0';
const port = 443;
var servCounter = 0; // it's okay that this will overflow

var key = fs.readFileSync('keys/domain.key');
var cert = fs.readFileSync( 'keys/domain.crt' );

var options = {
  key: key,
  cert: cert
};

cmd.get(`find ${__dirname}/../../ -name "15*" | while read line; do rm -rf $line; done`, function(errorCode, e__, o__) {
  if (errorCode) {
    console.log("FAILED", errorCode, e__, o__);
    process.exit(1);
  }

  var app = connect();

  app.use(function (req, res, next) {
      //res.setHeader('Access-Control-Allow-Credentials', 'true');
      res.setHeader("Access-Control-Allow-Origin", "*");
      res.setHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, PUT, OPTIONS, HEAD");
      next();
  });

  app.use(serveStatic(__dirname + '/static'))
  app.use(serveStatic(__dirname + '/hosted'))

  app.use("/compile", function(req, res, next) {
    if (req.method == 'POST') {
      var body = '';
      req.on('data', function (data) {
          body += data;
          if (body.length > 8192)
              req.connection.destroy();
      });

      req.on('end', function () {
          createBinary(req, res, body);
      });
    }
  });

  const server = https.createServer(options, app);
  server.listen(port, hostname, () => {
    console.log(`Server running at https://${hostname}/`);
  });
});

function createBinary(req, res, body) {
  var configs = {
    "SSID": { name: "WIFI_NAME", value: 0},
    "PASS": { name: "WIFI_PASSWORD", value: 0},

    "CONN": { name: "IOT_CENTRAL_CONNECTION_STRING", value: 0},

    "HUM": { name: "DISABLE_HUMIDITY", value: 0},
    "TEMP": { name: "DISABLE_TEMPERATURE", value: 0},
    "GYRO": { name: "DISABLE_GYROSCOPE", value: 0},
    "ACCEL": { name: "DISABLE_ACCELEROMETER", value: 0},
    "MAG": { name: "DISABLE_MAGNETOMETER", value: 0},
    "PRES":  { name: "DISABLE_PRESSURE", value: 0 }
  };
  var list = body.split('&');
  for (var i = 0; i < list.length; i++) {
    var arg = list[i].trim();
    if (arg.length == 0) continue;

    arg = arg.split('=');
    if (configs.hasOwnProperty(arg[0])) {
      var name = arg[0];
      if (arg.length > 1) {
        if (arg[1] == 'on') {
          configs[name].value = 1;
        } else {
          configs[name].value = arg[1];
        }
      } else {
        configs[name].value = 1;
      }

      if (arg.length > 2) {
        for (var j = 2; j < arg.length; j++) {
          configs[name].value += "=" + arg[j]
        }
      }
    }
  }

  var failed = false;
  var writeString = '';

  if (!configs.SSID.value || configs.SSID.value.length == 0) {
    failed = true;
    writeString += ('<p>- Missing SSID</p>');
  }

  if (!configs.PASS.value || configs.PASS.value.length == 0) {
    failed = true;
    writeString += ('<p>- Missing WiFi password</p>');
  }

  if (!configs.CONN.value || configs.CONN.value.length == 0) {
    failed = true;
    writeString += ('<p>- Missing IoT Central device connection string</p>');
  }

  if (!failed) {
    var defs = [];
    for(var o in configs) {
      if (!configs.hasOwnProperty(o)) continue;
      if (configs[o] == 0) continue;

      var line = '#define ' + configs[o].name;
      if (configs[o].value != 1) {
        line += '  \"' + unescape(configs[o].value) + '\"'
        defs.push(line);
      }
    }

    defs.push('#define COMPILE_TIME_DEFINITIONS_SET');
    defs = defs.join('\n').replace(/\"/g, "\\\"");
    var NAME = (Date.now() + "" + servCounter);
    servCounter += Date.now() % 2 == 0 ? 11 : 13;
    cmd.get(`
    cp -R ../../AZ3166 ../../${NAME}
    cd ../../${NAME}
    echo "${defs}" > inc/definitions.h
    docker image prune -f
    iotc iotCentral.ino -c=a -t=AZ3166:stm32f4:MXCHIP_AZ3166
    `, function(errorCode, stdout, stderr) {
      if (errorCode) {
        writeString += ('<p>something bad has happened</p>' +
          (stdout + stderr).replace(/\n/g, "<br>"));
        cmd.get(`rm -rf ../../${NAME}`);
        failed = true;
        renderDownload(res, failed, writeString)
      } else {
        var filename = path.join(__dirname, `../../${NAME}`, `out/program.bin`);
        var appName = "program.bin"
        cmd.get(`mkdir -p hosted/${NAME} && mv ${filename} hosted/${NAME}/ && rm -rf ../../${NAME}`, function(e, s, r) {
          res.write(writeString);

          if (errorCode) {
            writeString += ('<p>something bad has happened</p>' +
              (s + r).replace(/\n/g, "<br>"));
            failed = true;
          } else {
            writeString += `<a id='btnD' href="${NAME}/program.bin">Download Firmware</a> <br/>`;
            writeString += `<a id='btn' style='display:none' href="#">Flash Firmware to USB</a> <br/>`;
          }
          renderDownload(res, failed, writeString)
        });
      }
    });
  } else {
    renderDownload(res, failed, writeString)
  }
}

function renderDownload(res, failed, writeString) {
  var render = fs.readFileSync(__dirname + "/static/download.html") + "";
  render = render.replace("$OUTPUT", writeString)
                 .replace("$NO_ISSUE", !failed);

  res.end(render);
}
