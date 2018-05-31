/* stlinkv2.js
 * ST-Link probe API class
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/stlinkv2.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

import { Exception, Warning, UsbError } from './stlinkex.js';

const STLINK_GET_VERSION = 0xf1;
const STLINK_DEBUG_COMMAND = 0xf2;
const STLINK_DFU_COMMAND = 0xf3;
const STLINK_SWIM_COMMAND = 0xf4;
const STLINK_GET_CURRENT_MODE = 0xf5;
const STLINK_GET_TARGET_VOLTAGE = 0xf7;

const STLINK_MODE_DFU = 0x00;
const STLINK_MODE_MASS = 0x01;
const STLINK_MODE_DEBUG = 0x02;
const STLINK_MODE_SWIM = 0x03;
const STLINK_MODE_BOOTLOADER = 0x04;

const STLINK_DFU_EXIT = 0x07;

const STLINK_SWIM_ENTER = 0x00;
const STLINK_SWIM_EXIT = 0x01;

const STLINK_DEBUG_ENTER_JTAG = 0x00;
const STLINK_DEBUG_STATUS = 0x01;
const STLINK_DEBUG_FORCEDEBUG = 0x02;
const STLINK_DEBUG_APIV1_RESETSYS = 0x03;
const STLINK_DEBUG_APIV1_READALLREGS = 0x04;
const STLINK_DEBUG_APIV1_READREG = 0x05;
const STLINK_DEBUG_APIV1_WRITEREG = 0x06;
const STLINK_DEBUG_READMEM_32BIT = 0x07;
const STLINK_DEBUG_WRITEMEM_32BIT = 0x08;
const STLINK_DEBUG_RUNCORE = 0x09;
const STLINK_DEBUG_STEPCORE = 0x0a;
const STLINK_DEBUG_APIV1_SETFP = 0x0b;
const STLINK_DEBUG_READMEM_8BIT = 0x0c;
const STLINK_DEBUG_WRITEMEM_8BIT = 0x0d;
const STLINK_DEBUG_APIV1_CLEARFP = 0x0e;
const STLINK_DEBUG_APIV1_WRITEDEBUGREG = 0x0f;
const STLINK_DEBUG_APIV1_SETWATCHPOINT = 0x10;
const STLINK_DEBUG_APIV1_ENTER = 0x20;
const STLINK_DEBUG_EXIT = 0x21;
const STLINK_DEBUG_READCOREID = 0x22;
const STLINK_DEBUG_APIV2_ENTER = 0x30;
const STLINK_DEBUG_APIV2_READ_IDCODES = 0x31;
const STLINK_DEBUG_APIV2_RESETSYS = 0x32;
const STLINK_DEBUG_APIV2_READREG = 0x33;
const STLINK_DEBUG_APIV2_WRITEREG = 0x34;
const STLINK_DEBUG_APIV2_WRITEDEBUGREG = 0x35;
const STLINK_DEBUG_APIV2_READDEBUGREG = 0x36;
const STLINK_DEBUG_APIV2_READALLREGS = 0x3a;
const STLINK_DEBUG_APIV2_GETLASTRWSTATUS = 0x3b;
const STLINK_DEBUG_APIV2_DRIVE_NRST = 0x3c;
const STLINK_DEBUG_SYNC = 0x3e;
const STLINK_DEBUG_APIV2_START_TRACE_RX = 0x40;
const STLINK_DEBUG_APIV2_STOP_TRACE_RX = 0x41;
const STLINK_DEBUG_APIV2_GET_TRACE_NB = 0x42;
const STLINK_DEBUG_APIV2_SWD_SET_FREQ = 0x43;
const STLINK_DEBUG_ENTER_SWD = 0xa3;

const STLINK_DEBUG_APIV2_DRIVE_NRST_LOW = 0x00;
const STLINK_DEBUG_APIV2_DRIVE_NRST_HIGH = 0x01;
const STLINK_DEBUG_APIV2_DRIVE_NRST_PULSE = 0x02;

const STLINK_MAXIMUM_TRANSFER_SIZE = 1024;

const STLINK_DEBUG_APIV2_SWD_SET_FREQ_MAP = [
    [4000000, 0],
    [1800000, 1],  // default
    [1200000, 2],
    [950000,  3],
    [480000,  7],
    [240000, 15],
    [125000, 31],
    [100000, 40],
    [50000,  79],
    [25000, 158],
    [15000, 265],
    [5000,  798]
];

export default class Stlink {
    constructor(connector, dbg) {
        this._connector = connector;
        this._dbg = dbg;
    }

    _debug(msg) {
        if (this._dbg) {
            this._dbg.debug(msg);
        }
    }

    async init(swd_frequency = 1800000) {
        await this.read_version();
        await this.leave_state();
        await this.read_target_voltage();
        if (this._ver_jtag >= 22) {
            await this.set_swd_freq(swd_frequency);
        }
        await this.enter_debug_swd();
        await this.read_coreid();
    }

    async clean_exit() {
        // WORKAROUND for OS/X 10.11+
        // ... read from ST-Link, must be performed even times
        // call this function after last send command
        if (this._connector.xfer_counter & 1) {
            await this._connector.xfer([STLINK_GET_CURRENT_MODE], {"rx_len": 2});
        }
    }

    async read_version() {
        // WORKAROUNF for OS/X 10.11+
        // ... retry XFER if first is timeout.
        // only during this command it is necessary
        let rx = await this._connector.xfer([STLINK_GET_VERSION, 0x80], {"rx_len": 6, "retry": 2 });
        let ver = rx.getUint16(0);

        let dev_ver = this._connector.version;
        this._ver_stlink = ((ver >> 12) & 0x0f);
        this._ver_jtag = ((ver >> 6) & 0x3f);
        this._ver_swim = ((dev_ver === "V2") ? (ver & 0x3f) : null);
        this._ver_mass = ((dev_ver === "V2-1") ? (ver & 0x3f) : null);
        this._ver_api = ((this._ver_jtag > 11) ? 2 : 1);
        this._ver_str = `${dev_ver} V${this._ver_stlink}J${this._ver_jtag}`;
        if (dev_ver === "V2") {
            this._ver_str += ("S" + this._ver_swim);
        }
        if (dev_ver === "V2-1") {
            this._ver_str += ("M" + this._ver_mass);
        }
        if (this.ver_api === 1) {
            throw new Warning(`ST-Link/${this._ver_str} is not supported, please upgrade firmware.`);
        }
        if (this.ver_jtag < 21) {
            throw new Warning(`ST-Link/${this._ver_str} is not recent firmware, please upgrade first - functionality is not guaranteed.`);
        }
    }

    get maximum_transfer_size() {
        return STLINK_MAXIMUM_TRANSFER_SIZE;
    }

    get ver_stlink() {
        return this._ver_stlink;
    }

    get ver_jtag() {
        return this._ver_jtag;
    }

    get ver_mass() {
        return this._ver_mass;
    }

    get ver_swim() {
        return this._ver_swim;
    }

    get ver_api() {
        return this._ver_api;
    }

    get ver_str() {
        return this._ver_str;
    }

    async read_target_voltage() {
        let rx = await this._connector.xfer([STLINK_GET_TARGET_VOLTAGE], {"rx_len": 8});
        let a0 = rx.getUint32(0, true);
        let a1 = rx.getUint32(4, true);
        this._target_voltage = (a0 !== 0) ? (2 * a1 * 1.2 / a0) : null;
    }

    get target_voltage() {
        return this._target_voltage;
    }

    async read_coreid() {
        let rx = await this._connector.xfer([STLINK_DEBUG_COMMAND, STLINK_DEBUG_READCOREID], {"rx_len": 4});
        this._coreid = rx.getUint32(0, true);
    }

    get coreid() {
        return this._coreid;
    }

    async leave_state() {
        let rx = await this._connector.xfer([STLINK_GET_CURRENT_MODE], {"rx_len": 2});
        let state = rx.getUint8(0);

        if (state === STLINK_MODE_DFU) {
            this._debug("Leaving state DFU");
            await this._connector.xfer([STLINK_DFU_COMMAND, STLINK_DFU_EXIT]);
        } else if (state === STLINK_MODE_DEBUG) {
            this._debug("Leaving state DEBUG");
            await this._connector.xfer([STLINK_DEBUG_COMMAND, STLINK_DEBUG_EXIT]);
        } else if (state === STLINK_MODE_SWIM) {
            this._debug("Leaving state is SWIM");
            await this._connector.xfer([STLINK_SWIM_COMMAND, STLINK_SWIM_EXIT]);
        }
    }

    async set_swd_freq(freq = 1800000) {
        for (let [f, divisor] of STLINK_DEBUG_APIV2_SWD_SET_FREQ_MAP) {
            if (freq >= f) {
                let cmd = [STLINK_DEBUG_COMMAND, STLINK_DEBUG_APIV2_SWD_SET_FREQ, divisor];
                let rx = await this._connector.xfer(cmd, {"rx_len": 2});
                let status = rx.getUint8(0);
                if (status !== 0x80) {
                    throw new Exception("Error switching SWD frequency");
                }
                this._debug(`Set SWD frequency to ${f}Hz`);
                return;
            }
        }
        throw new Exception("Selected SWD frequency is too low");
    }

    async enter_debug_swd() {
        await this._connector.xfer([STLINK_DEBUG_COMMAND, STLINK_DEBUG_APIV2_ENTER, STLINK_DEBUG_ENTER_SWD], {"rx_len": 2});
        this._debug("Entered SWD debug mode");
    }

    async debug_resetsys() {
        await this._connector.xfer([STLINK_DEBUG_COMMAND, STLINK_DEBUG_APIV2_RESETSYS], {"rx_len": 2});
        this._debug("Sent reset");
    }

    set_debugreg32(addr, data) {
        if (addr % 4) {
            throw new Exception("get_mem_short address is not in multiples of 4");
        }
        let cmd = new ArrayBuffer(10);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_APIV2_WRITEDEBUGREG);
        view.setUint32(2, addr, true);
        view.setUint32(6, data, true);
        return this._connector.xfer(cmd, {"rx_len": 2});
    }

    async get_debugreg32(addr) {
        if (addr % 4) {
            throw new Exception("get_mem_short address is not in multiples of 4");
        }
        let cmd = new ArrayBuffer(6);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_APIV2_READDEBUGREG);
        view.setUint32(2, addr, true);
        let rx = await this._connector.xfer(cmd, {"rx_len": 8});
        return rx.getUint32(4, true);
    }

    async get_debugreg16(addr) {
        if (addr % 2) {
            throw new Exception("get_mem_short address is not in even");
        }
        let val = await this.get_debugreg32(addr & 0xfffffffc);
        if (addr % 4) {
            val >>= 16;
        }
        return (val & 0xffff);
    }

    async get_debugreg8(addr) {
        let val = await this.get_debugreg32(addr & 0xfffffffc);
        val >>= (addr % 4) << 3;
        return (val & 0xff);
    }

    async get_reg(reg) {
        let cmd = [STLINK_DEBUG_COMMAND, STLINK_DEBUG_APIV2_READREG, reg];
        let rx = await this._connector.xfer(cmd, {"rx_len": 8});
        return rx.getUint32(4, true);
    }

    set_reg(reg, data) {
        let cmd = new ArrayBuffer(7);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_APIV2_WRITEREG);
        view.setUint8(2, reg);
        view.setUint32(3, data, true);
        return this._connector.xfer(cmd, {"rx_len": 2});
    }

    get_mem32(addr, size) {
        if (addr % 4) {
            throw new Exception("get_mem32: Address must be in multiples of 4");
        }
        if (size % 4) {
            throw new Exception("get_mem32: Size must be in multiples of 4");
        }
        if (size > STLINK_MAXIMUM_TRANSFER_SIZE) {
            throw new Exception(`get_mem32: Size for reading is ${size} but maximum can be ${STLINK_MAXIMUM_TRANSFER_SIZE}`);
        }
        let cmd = new ArrayBuffer(10);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_READMEM_32BIT);
        view.setUint32(2, addr, true);
        view.setUint32(6, size, true);
        return this._connector.xfer(cmd, {"rx_len": size});
    }

    set_mem32(addr, data) {
        if (addr % 4) {
            throw new Exception("set_mem32: Address must be in multiples of 4");
        }
        if (data.length % 4) {
            throw new Exception("set_mem32: Size must be in multiples of 4");
        }
        if (data.length > STLINK_MAXIMUM_TRANSFER_SIZE) {
            throw new Exception(`set_mem32: Size for writing is ${data.length} but maximum can be ${STLINK_MAXIMUM_TRANSFER_SIZE}`);
        }
        let cmd = new ArrayBuffer(10);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_WRITEMEM_32BIT);
        view.setUint32(2, addr, true);
        view.setUint32(6, data.length, true);
        return this._connector.xfer(cmd, {"data": data});
    }

    get_mem8(addr, size) {
        if (size > 64) {
            throw new Exception(`get_mem8: Size for reading is ${size} but maximum can be 64`);
        }
        let cmd = new ArrayBuffer(10);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_READMEM_8BIT);
        view.setUint32(2, addr, true);
        view.setUint32(6, size, true);
        return this._connector.xfer(cmd, {"rx_len": size});
    }

    set_mem8(addr, data) {
        if (data.length > 64) {
            throw new Exception(`set_mem8: Size for writing is ${data.length} but maximum can be 64`);
        }
        let cmd = new ArrayBuffer(10);
        let view = new DataView(cmd);
        view.setUint8(0, STLINK_DEBUG_COMMAND);
        view.setUint8(1, STLINK_DEBUG_WRITEMEM_8BIT);
        view.setUint32(2, addr, true);
        view.setUint32(6, data.length, true);
        return this._connector.xfer(cmd, {"data": data});
    }
}

