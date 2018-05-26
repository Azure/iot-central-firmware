// ----------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//  Licensed under the MIT license.
// ----------------------------------------------------------------------------

const http = require('http');
const fs   = require('fs');
const path = require('path');
const cmd = require('node-cmd');

const hostname = '0.0.0.0';
const port = 8080;
var HTTP_START_PAGE_HTML = fs.readFileSync(__dirname + '/index.html');
var servCounter = 0; // it's okay that this will overflow

cmd.get(`find ${__dirname}/../../ -name "15*" | while read line; do rm -rf $line; done`, function(errorCode, e__, o__) {
  if (errorCode) {
    console.log(errorCode, e__, o__);
    process.exit(1);
  }
  const server = http.createServer((req, res) => {
    res.statusCode = 200;
    if (req.url == '/') {
      res.setHeader('Content-Type', 'text/html');
      res.end(HTTP_START_PAGE_HTML);
    } else if (req.url.startsWith('/program.bin')) {
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
      } else {
        res.end('404 :p');
      }
    } else {
      res.end("404 :p");
    }
  });
  server.listen(port, hostname, () => {
    console.log(`Server running at http://${hostname}:${port}/`);
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
  var writeString = '<html><body style="font-size:120%;">'
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
    var NAME = Date.now() + (servCounter++);
    cmd.get(`
    cp -R ../../AZ3166 ../../${NAME}
    cd ../../${NAME}
    echo "${defs}" > inc/definitions.h
    docker image prune -f
    iotc iotCentral.ino -c=a -t=AZ3166:stm32f4:MXCHIP_AZ3166
    `, function(errorCode, stdout, stderr) {
      if (errorCode) {
        res.write(writeString)
        res.write('<p>something bad has happened</p>' +
          (stdout + stderr).replace(/\n/g, "<br>"));
        res.end('</body></html>');
        cmd.get(`rm -rf ../../${NAME}`);
      } else {
        var filename = path.join(__dirname, `../../AZ3166 ../../${NAME}`, `out/program.bin`);
        var stat = fs.statSync(filename);
        var reader = fs.createReadStream(filename);
        var header = {
          'Content-Length': stat.size,
          'Content-Type': 'binary',
          'Access-Control-Allow-Origin': req.headers.origin || "*",
          'Access-Control-Allow-Methods': 'POST, GET, OPTIONS',
          'Access-Control-Allow-Headers': 'POST, GET, OPTIONS'
        };
        res.writeHead(200, header);

        reader.pipe(res);
        reader.on('close', function () {
          res.end(0);
          cmd.get(`rm -rf ../../${NAME}`);
        });
      }
    });
  } else {
    res.write(writeString)
    res.write('<h5>Click <a href="http://ozzzzz.westus.cloudapp.azure.com:8080/" target=_blank>here</a> to go back</h5>');
    res.end('</body></html>');
  }
}
