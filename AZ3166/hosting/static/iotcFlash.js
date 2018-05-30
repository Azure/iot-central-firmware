import * as libstlink from './stlink/lib/package.js';
import WebStlink from './stlink/webstlink.js';
import { hex_octet, hex_word, hex_octet_array } from './stlink/lib/util.js';
import cs from './capstone-arm.min.js';

var stlink = null;
var curr_device = null;
let log = document.querySelector("#log");
var logger = new libstlink.Logger(1, log);
const mode = cs.MODE_THUMB + cs.MODE_MCLASS + cs.MODE_LITTLE_ENDIAN;
let decoder = new cs.Capstone(cs.ARCH_ARM, mode);

window.connectButton = document.getElementById('connectButton');
window.flashButton = document.getElementById('flashButton');

window.connectButton = async function() {
    console.log('connect button pressed');

    if (stlink !== null) {
        await stlink.detach();
        on_disconnect();
        return;
    }

    try {
        let device = await navigator.usb.requestDevice({
            filters: libstlink.usb.filters
        });
        logger.clear();
        let next_stlink = new WebStlink(logger);
        await next_stlink.attach(device, logger);
        stlink = next_stlink;
        curr_device = device;
    } catch (err) {
        logger.error(err);
    }

    if (stlink !== null) {
        await on_successful_attach(stlink, curr_device);
    }
};

window.getSTLink = function() {
    return stlink;
}

async function haltDevice() {
    if (stlink !== null && stlink.connected) {
        if (!stlink.last_cpu_status.halted) {
            await stlink.halt();
            await stlink.inspect_cpu();
        }
    }
}

async function runDevice() {
    if (stlink !== null && stlink.connected) {
        if (stlink.last_cpu_status.halted) {
            await stlink.run();
            await stlink.inspect_cpu();
        }
    }
}

async function resetDevice() {
    if (stlink !== null) {
        await stlink.reset(stlink.last_cpu_status.halted);
        await stlink.inspect_cpu();
    }
}

function show_error_dialog(error) {
    let dialog = document.createElement("dialog");
    let header = document.createElement("h1");
    header.textContent = "Uh oh! Something went wrong.";
    let contents = document.createElement("p");
    contents.textContent = error.toString();
    let button = document.createElement("button");
    button.textContent = "Close";

    button.addEventListener("click", (evt) => {
        dialog.close();
    });

    dialog.addEventListener("close", (evt) => {
        dialog.remove();
    });

    dialog.appendChild(header);
    dialog.appendChild(contents);
    dialog.appendChild(document.createElement("br"));
    dialog.appendChild(button);

    document.querySelector("body").appendChild(dialog);

    dialog.showModal();
}

window.flashButton = async function() {
    console.log('flash button pressed');
    document.getElementById("log").hidden = false;
    if (stlink !== null && stlink.connected) {
        if (!stlink.last_cpu_status.halted) {
            haltDevice()
        }
        var oReq = new XMLHttpRequest();
        var btnD = document.getElementById('btnD');
        oReq.open("GET", btnD.href, true);
        oReq.responseType = "arraybuffer";

        oReq.onload = async function (oEvent) {
            var flashData = oReq.response;
            if (flashData) {
                // flash the data to the device
                try {
                    var myNode = document.getElementById("log");
                    while (myNode.firstChild) {
                        myNode.removeChild(myNode.firstChild);
                    }
                    await stlink.flash(window.deviceTarget.flash_start, flashData);
                        await resetDevice();
                        await runDevice();
                } catch (err) {
                    logger.error(err);
                    show_error_dialog(err);
                }
            }
        };

        oReq.send(null);
    }
};

async function on_successful_attach(stlink, device) {
    // Export for manual debugging
    stlink = stlink;
    device = device;

    // Reset settings
    connectButton.textContent = "Disconnect";

    // Add disconnect handler
    navigator.usb.addEventListener('disconnect', function (evt) {
        if (evt.device === device) {
            navigator.usb.removeEventListener('disconnect', this);
            if (device === curr_device) {
                on_disconnect();
            }
        }
    });

    // Detect attached target CPU
    window.deviceTarget = await stlink.detect_cpu([], null);
    await stlink.inspect_cpu();
}

function getDevice() {
    if (window.deviceTarget) {
        return window.deviceTarget;
    } else {
        return null;
    }
}

function on_disconnect() {
    logger.info("Device disconnected");
    connectButton.textContent = "Connect";

    document.getElementById("flashDiv").hidden = true;
    if (document.getElementById("flashimage"))
        document.getElementById("flashimage").remove();
    document.getElementById("log").hidden = true;
    stlink = null;
    curr_device = null;
}

