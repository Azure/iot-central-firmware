#!/usr/bin/env node

// ----------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//  Licensed under the MIT license.
// ----------------------------------------------------------------------------

const colors = require('colors/safe');
const fs     = require('fs');
const path   = require('path');
const cmd    = require('node-cmd');
const rimraf = require('rimraf');

var args = {};
var printHelp = function printHelp() {
  var params = [
    {option: "--help", text: "display available options"},
    {option: "-v", text: "show version"},
    {option: "", text: ""}, // placeholder
    {option: "-c, --compile=[target platform]", text: "a=arduino m=ARMmbed"},
    {option: "    --update", text: "update base container to latest"},
    {option: "-t, --target=[target name]", text: "ARM mbed target board name"}
  ];
  console.log(' ', "usage:", colors.magenta('iotc'), '<path>', '[options]\n\n',
              colors.bold(' options:\n'));

  for (var i in params) {
    var param = params[i];
    if (param.hidden) continue;
    var space = new Buffer(40 - param.option.length);
    space.fill(" ");
    console.log(' ', param.option, space + ": ", param.text);
  }
  console.log('\n ', colors.green('example:'), 'iotc . -c=m -t=DISCO_L475VG_IOT01A');
  console.log(' ',   colors.green('        '), 'iotc ./app.ino -c=a -t=AZ3166:stm32f4:MXCHIP_AZ3166\n');
  console.log(' ',   colors.yellow('   CAUTION: target names are case sensitive '));
};

console.log("\n [ IOTC - a wrapper for ARMmbed && Arduino toolchains ]");
console.log(colors.yellow('\t\t\t by Azure-IOT\n'));

for (var i = 2; i < process.argv.length; i++) {
  var arg = process.argv[i];
  arg = arg.split('=');
  args[arg[0].replace(/"/g, "")] = arg.length > 1 ? arg[1] : 1;
}

if (args['-v']) {
  try {
    var version = JSON.parse(fs.readFileSync(__dirname + '/package.json') + "").version;
    console.log("  version ", colors.green(version));
  } catch(e) {
    console.log("  - error : " + e.message);
  }
  process.exit(0);
}

cmd.get(
  'docker -v', function(err, data, stderr){
    if (err) {
      console.error(
          ' - ' + colors.red('error') + ' : '
          + colors.red('docker'), 'is required and not found.\n',
          '          visit', colors.green('https://docs.docker.com/install/'),
          '\n');
      process.exit(1);
    } else {
      builder();
    }
  }
);

function builder() {
  if (args['--update']) {
    cmd.get('docker pull azureiot/mbed:latest',
      function(err, data, stderr) {
        if (err) {
          console.log(stderr.replace(/\\n/g, '\n'), '\n', data);
          console.error(colors.red(' - error: update has failed. See the output above.'))
          process.exit(1);
        } else {
          console.log(data.replace(/\\n/g, '\n'));
          console.log(colors.green(' - update was succesfull'));
        }
      });
    return;
  }

  if (args.hasOwnProperty('--help') || args.hasOwnProperty('-h')) {
    printHelp();
    process.exit(0);
    return;
  }

  if (args.hasOwnProperty('--compile') && args.hasOwnProperty('-c')) {
    console.error(colors.red(' - error: both -c and --compile are set??'))
    process.exit(1);
  }

  if (args.hasOwnProperty('--compile')) {
    args['-c'] = args['--compile']
  }

  if (args.hasOwnProperty('--target')) {
    args['-t'] = args['--target']
  }

  if (args.hasOwnProperty('-c') && typeof(args['-c']) == "string") {
    args['-c'] = args['-c'].toLowerCase();
  } else {
    console.error(colors.red(' - error: missing "-c". Try --help ?'))
    process.exit(1);
  }

  var target_board;
  if (!args.hasOwnProperty('-t')) {
    console.error(' ', '-', colors.red('error'), ': no', colors.red('target'), 'given\n');
    process.exit(1);
  } else {
    target_board = args['-t'];
    if (typeof target_board !== 'string') {
      console.error(' ', '-', colors.red('error'), ': no', colors.red('target'), 'given\n');
      process.exit(1);
    }
  }

  var compile_path = process.argv.length > 2 ? process.argv[2] : '';
  var filename;

  if (!fs.existsSync(compile_path)) {
    compile_path = process.cwd();
  }
  compile_path = path.resolve(compile_path)

  if (args['-c'] == 'a') {
    if (path.extname(compile_path) != '.ino') {
      filename = 'app.ino';
    } else {
      filename = path.basename(compile_path);
      compile_path = path.dirname(compile_path);
    }

    if (!fs.existsSync(path.join(compile_path, filename))) {
      console.error(' ', '-', colors.red('error'), ': you should provide the path for ', colors.red('<...>.ino'), ' file for arduino builds\n');
      process.exit(1);
    }
  }

  console.log("\n- " + colors.green('compile path'), ':', compile_path, '\n');
  if (args['-c'] == 'a') {
    console.log("\n- " + colors.green('project file'), ':', filename, '\n');
  }

  if (args['-c'] == 'm') {
    mbedBuild();
  } else {
    arduinoBuild();
  }

  function mbedBuild() {
    if (fs.existsSync(path.join(compile_path, 'out'))) {
      rimraf.sync(path.join(compile_path, 'out/'));
    }

    fs.mkdirSync(path.join(compile_path, 'out/'));

    var dockerfile = `
FROM azureiot/mbed:latest

WORKDIR /src/program

COPY . /src/program/

RUN mbed new . && mbed target ${target_board} && mbed toolchain GCC_ARM

# always do a clean build to prevent missing header files :/
RUN rm -rf BUILD && mbed deploy && mbed compile
`;

    var ino = fs.statSync(compile_path).ino;
    fs.writeFileSync(path.join(compile_path, 'Dockerfile'), dockerfile);

    var container_name = "aiot_iotc_" + target_board.toLowerCase() + "_" + ino;
    var subProcess = cmd.get(`
      cd ${compile_path}
      docker build . --force-rm -t ${container_name} && \
      docker run --rm --entrypoint cat "${container_name}:latest" \
        "./BUILD/${target_board}/GCC_ARM/program.bin" > out/program.bin
      `);

    var dataLine = "";
    var dockerFileDeleted = false;
    var stdoutput = function(data) {
      if (!dockerFileDeleted) {
        rimraf.sync(path.join(compile_path, 'Dockerfile'));
      }

      dataLine += data;
      if (dataLine[dataLine.length - 1] == '\n') {
        console.log(dataLine);
        dataLine = "";
      }
    };

    subProcess.stdout.on('data', stdoutput);
    subProcess.stderr.on('data', stdoutput);

    subProcess.on('exit', function(errorCode) {
      if (errorCode) {
        console.error(colors.red(' - error: build has failed. See the output above.'))
        process.exit(1);
      } else {
        console.log(colors.green(' - build was succesfull'));
        var stat = fs.statSync(path.join(compile_path, 'out/program.bin'));

        console.log(' - check', colors.yellow('out/'), 'folder for program.bin('
                    + (stat ? parseInt(stat.size / 1024) : 0) + ' kb)');
      }
    });
  } // mbedBuild

  function arduinoBuild() {
    if (fs.existsSync(path.join(compile_path, 'out'))) {
      rimraf.sync(path.join(compile_path, 'out/'));
    }

    fs.mkdirSync(path.join(compile_path, 'out/'));

    var install_board = "";
    if (target_board != "AZ3166:stm32f4:MXCHIP_AZ3166"
      && !target_board.startsWith("arduino:avr:")) {
      install_board = "RUN arduino --install-boards " + target_board;
    }

    var dockerfile = `
FROM azureiot/mbed:latest

WORKDIR /src/program

${install_board}

COPY . /src/program/

RUN mkdir build && \
    arduino --board "${target_board}" \
    --verify "${filename}" --pref build.path=/src/program/build \
    && ls -l build
`;

    var ino = fs.statSync(compile_path).ino;
    fs.writeFileSync(path.join(compile_path, 'Dockerfile'), dockerfile);

    var container_name = "aiot_iotc_"
      + target_board.toLowerCase().replace(/\:/g, "_") + "_" + ino;
    var subProcess = cmd.get(`
      cd ${compile_path}
      docker build . --force-rm -t ${container_name} && echo PLACEHOLDER_IOTC_COPY1_
      echo PLACEHOLDER_IOTC_COPY2_
      docker run --rm --entrypoint cat "${container_name}:latest" \
        "/src/program/build/${filename}.eep" > out/program.eep
      docker run --rm --entrypoint cat "${container_name}:latest" \
        "/src/program/build/${filename}.bin" > out/program.bin
      docker run --rm --entrypoint cat "${container_name}:latest" \
        "/src/program/build/${filename}.hex" > out/program.hex
      docker run --rm --entrypoint cat "${container_name}:latest" \
        "/src/program/build/${filename}.with_bootloader.hex" > out/program.with_bootloader.hex
      `);

    var dataLine = "";
    var dockerFileDeleted = false;
    var skipErrors = false;
    var skipOutput = false;
    var stdoutput = function(data) {
      if (!dockerFileDeleted) {
        rimraf.sync(path.join(compile_path, 'Dockerfile'));
      }

      dataLine += data;
      if (dataLine.indexOf('PLACEHOLDER_IOTC_COPY1_') >= 0) {
        skipErrors = true;
      }

      if (dataLine.indexOf('PLACEHOLDER_IOTC_COPY2_') >= 0) {
        skipOutput = true;
      }

      if (skipErrors || skipOutput) return;

      if (dataLine[dataLine.length - 1] == '\n') {
        console.log(dataLine);
        dataLine = "";
      }
    };

    subProcess.stdout.on('data', stdoutput);
    subProcess.stderr.on('data', stdoutput);

    subProcess.on('exit', function(errorCode) {
      if (!skipErrors && errorCode) {
        console.error(colors.red(' - error: build has failed. See the output above.'))
        process.exit(1);
      } else {
        console.log(colors.green(' - build was succesfull'));
        var psep = path.sep;
        console.log(` - see '${compile_path}${psep}out' folder`);
      }
    });
  }
} // builder()
