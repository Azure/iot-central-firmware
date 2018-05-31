/* stm32fp.js
 * stm32 flash driver classes for parts with flash page-level access
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/stm32fp.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

import { Exception, Warning, UsbError } from './stlinkex.js';
import { Stm32 } from './stm32.js';
import {
    hex_word as H32,
    async_sleep,
    async_timeout
} from './util.js';

const FLASH_REG_BASE = 0x40022000;
const FLASH_REG_BASE_STEP = 0x40;
const FLASH_KEYR_INDEX = 0x04;
const FLASH_SR_INDEX = 0x0c;
const FLASH_CR_INDEX = 0x10;
const FLASH_AR_INDEX = 0x14;
const FLASH_KEYR_REG = FLASH_REG_BASE + FLASH_KEYR_INDEX;
const FLASH_SR_REG = FLASH_REG_BASE + FLASH_SR_INDEX;
const FLASH_CR_REG = FLASH_REG_BASE + FLASH_CR_INDEX;
const FLASH_AR_REG = FLASH_REG_BASE + FLASH_AR_INDEX;

const FLASH_CR_LOCK_BIT = 0x00000080;
const FLASH_CR_PG_BIT = 0x00000001;
const FLASH_CR_PER_BIT = 0x00000002;
const FLASH_CR_MER_BIT = 0x00000004;
const FLASH_CR_STRT_BIT = 0x00000040;
const FLASH_SR_BUSY_BIT = 0x00000001;
const FLASH_SR_PGERR_BIT = 0x00000004;
const FLASH_SR_WRPRTERR_BIT = 0x00000010;
const FLASH_SR_EOP_BIT = 0x00000020;

// PARAMS
// R0: SRC data
// R1: DST data
// R2: size
// R4: STM32_FLASH_SR
// R5: FLASH_SR_BUSY_BIT
// R6: FLASH_SR_EOP_BIT
const FLASH_WRITER_F0_CODE = new Uint8Array([
    // write:
    0x03, 0x88,  // 0x8803    // ldrh r3, [r0]
    0x0b, 0x80,  // 0x800b    // strh r3, [r1]
    // test_busy:
    0x23, 0x68,  // 0x6823    // ldr r3, [r4]
    0x2b, 0x42,  // 0x422b    // tst r3, r5
    0xfc, 0xd1,  // 0xd1fc    // bne <test_busy>
    0x33, 0x42,  // 0x4233    // tst r3, r6
    0x04, 0xd0,  // 0xd104    // beq <exit>
    0x02, 0x30,  // 0x3002    // adds r0, //2
    0x02, 0x31,  // 0x3102    // adds r1, //2
    0x02, 0x3a,  // 0x3a02    // subs r2, //2
    0x00, 0x2a,  // 0x2a00    // cmp r2, //0
    0xf3, 0xd1,  // 0xd1f3    // bne <write>
    // exit:
    0x00, 0xbe,  // 0xbe00    // bkpt 0x00
]);

class Flash {
    constructor(driver, stlink, dbg, bank = 0) {
        this._driver = driver;
        this._stlink = stlink;
        this._dbg = dbg;

        let reg_bank = (FLASH_REG_BASE + (FLASH_REG_BASE_STEP * bank));
        this.FLASH_KEYR_REG = (reg_bank + FLASH_KEYR_INDEX);
        this.FLASH_SR_REG = (reg_bank + FLASH_SR_INDEX);
        this.FLASH_CR_REG = (reg_bank + FLASH_CR_INDEX);
        this.FLASH_AR_REG = (reg_bank + FLASH_AR_INDEX);
    }

    async init() {
        await this._stlink.read_target_voltage();
        if (this._stlink.target_voltage < 2.0) {
            throw new Exception(`Supply voltage is ${this._stlink.target_voltage.toFixed(2)}V, but minimum for FLASH program or erase is 2.0V`);
        }
        await this.unlock();
    }

    async unlock() {
        await this._driver.core_reset_halt();
        // programming locked
        let wait_count = 0;

        // TODO: fix this!
        // issue here is the success of the code depends on flash was actually
        // CR locked. However, init_write -> flash init kicks in a bit earlier
        // this is an ugly hack to keep things synced.
        while(wait_count++ < 10) {
            let cr_reg = await this._stlink.get_debugreg32(this.FLASH_CR_REG);
            if (cr_reg & FLASH_CR_LOCK_BIT) {
                wait_count = 10;
                // unlock keys
                await this._stlink.set_debugreg32(this.FLASH_KEYR_REG, 0x45670123);
                await this._stlink.set_debugreg32(this.FLASH_KEYR_REG, 0xcdef89ab);

                cr_reg = await this._stlink.get_debugreg32(this.FLASH_CR_REG);
                // programming locked
                if (cr_reg & FLASH_CR_LOCK_BIT) {
                    throw new Exception("Error unlocking FLASH");
                }
            }
        }
    }

    async lock() {
        await this._stlink.set_debugreg32(this.FLASH_CR_REG, FLASH_CR_LOCK_BIT);
        await this._driver.core_reset_halt();
    }

    async erase_all() {
        await this._stlink.set_debugreg32(this.FLASH_CR_REG, FLASH_CR_MER_BIT);
        await this._stlink.set_debugreg32(this.FLASH_CR_REG, (FLASH_CR_MER_BIT | FLASH_CR_STRT_BIT));
        await this.wait_busy(2, "Erasing FLASH");
    }

    async erase_page(page_addr) {
        await this._stlink.set_debugreg32(this.FLASH_CR_REG, FLASH_CR_PER_BIT);
        await this._stlink.set_debugreg32(this.FLASH_AR_REG, page_addr);
        await this._stlink.set_debugreg32(this.FLASH_CR_REG, (FLASH_CR_PER_BIT | FLASH_CR_STRT_BIT));
        await this.wait_busy(0.2);
    }

    async erase_pages(flash_start, erase_sizes, addr, size) {
        if (size === undefined) {
            throw new Exception("erase size is undefined");
        }
        let page_addr = flash_start;
        this._dbg.bargraph_start("Erasing FLASH", {"value_min": addr, "value_max": (addr + size)});
        while (true) {
            for (let page_size of erase_sizes) {
                if (addr < (page_addr + page_size)) {
                    this._dbg.bargraph_update({"value": page_addr});
                    await this.erase_page(page_addr);
                }
                page_addr += page_size;
                if ((addr + size) < page_addr) {
                    this._dbg.bargraph_done();
                    return;
                }
            }
        }
    }

    async init_write(sram_offset) {
        this._dbg.debug("Loading flash algorithm at 0x" + H32(sram_offset));
        this._flash_writer_offset = sram_offset;
        this._flash_data_offset = (sram_offset + 256);
        await this._stlink.set_mem8(this._flash_writer_offset, FLASH_WRITER_F0_CODE);
        // set configuration for flash writer
        await this._driver.set_reg("R4", this.FLASH_SR_REG);
        await this._driver.set_reg("R5", FLASH_SR_BUSY_BIT);
        await this._driver.set_reg("R6", FLASH_SR_EOP_BIT);
        // enable PG
        await this._stlink.set_debugreg32(this.FLASH_CR_REG, FLASH_CR_PG_BIT);
    }

    async write(addr, block) {
        // if all data are 0xff then will be not written
        if (block.every(b => (b == 0xff))) {
            return;
        }

        this._dbg.debug(`Flashing ${block.length} bytes at 0x${H32(addr)}`);
        try {
            this._dbg.debug("Loading working buffer at 0x" + H32(this._flash_data_offset));
            await this._stlink.set_mem32(this._flash_data_offset, block);
            await this._driver.set_reg("PC", this._flash_writer_offset);
            await this._driver.set_reg("R0", this._flash_data_offset);
            await this._driver.set_reg("R1", addr);
            await this._driver.set_reg("R2", block.length);
            await this._driver.core_run();
            await this.wait_for_breakpoint(0.2);
        } catch (e) {
            throw new Exception(`Failed to flash ${block.length} bytes at 0x${H32(addr)}: ` + e);
        }
    }

    async wait_busy(wait_time, bargraph_msg = null) {
        if (bargraph_msg) {
            this._dbg.bargraph_start(bargraph_msg, {
                "value_min": Date.now()/1000.0,
                "value_max": (Date.now()/1000.0 + wait_time)
            });
        }

        // all times are from data sheet, will be more safe to wait 2 time longer
        const end_time = (Date.now() + (wait_time * 2 * 1000));
        while (Date.now() < end_time) {
            if (bargraph_msg) {
                this._dbg.bargraph_update({"value": Date.now()/1000.0});
            }
            let status = await this._stlink.get_debugreg32(this.FLASH_SR_REG);
            if (!(status & FLASH_SR_BUSY_BIT)) {
                await this.end_of_operation(status);
                if (bargraph_msg) {
                    this._dbg.bargraph_done();
                }
                return;
            }
            await async_sleep(wait_time / 20);
        }
        throw new Exception("Operation timeout");
    }

    async wait_for_breakpoint(wait_time) {
        const end_time = Date.now() + (wait_time * 1000);
        do {
            let dhcsr = await this._stlink.get_debugreg32(Stm32.DHCSR_REG);
            this._dbg.debug("DHCSR = 0x" + H32(dhcsr));
            if (dhcsr & Stm32.DHCSR_STATUS_LOCKUP_BIT) {
                throw new Exception("Flash algorithm crashed");
            }
            if (dhcsr & Stm32.DHCSR_STATUS_HALT_BIT) {
                break;
            }
            await async_sleep(wait_time / 20);
        } while (Date.now() < end_time);

        let sr = await this._stlink.get_debugreg32(this.FLASH_SR_REG);
        await this.end_of_operation(sr);
    }

    async end_of_operation(status) {
        if (status !== FLASH_SR_EOP_BIT) {
            throw new Exception("Error writing FLASH with status (FLASH_SR) " + H32(status));
        }
        await this._stlink.set_debugreg32(this.FLASH_SR_REG, status);
    }
}

// support all STM32F MCUs with page access to FLASH
// (STM32F0xx, STM32F1xx and also STM32F3xx)
class Stm32FP extends Stm32 {
    async _flash_erase_all(bank = 0) {
        var flash;
        flash = new Flash(this, this._stlink, this._dbg, bank);
        await flash.init();
        await flash.erase_all();
        await flash.lock();
    }

    async flash_erase_all() {
        this._dbg.debug("Stm32FP.flash_erase_all()");
        await this._flash_erase_all();
    }

    async _flash_write(addr, data, { erase = false, verify = false, erase_sizes = null, bank = 0 }) {
        // align data
        if (data.length % 4) {
            let padded_data = new Uint8Array(data.length + (4 - (data.length % 4)));
            data.forEach((b, i) => padded_data[i] = b);
            padded_data.fill(0xff, data.length);
            data = padded_data;
        }
        let flash = new Flash(this, this._stlink, this._dbg, bank);
        await flash.init();
        if (erase) {
            if (erase_sizes) {
                await flash.erase_pages(this.FLASH_START, erase_sizes, addr, data.length);
            } else {
                await flash.erase_all();
            }
        }
        this._dbg.bargraph_start("Writing FLASH", {"value_min": addr, "value_max": (addr + data.length)});
        await flash.init_write(Stm32FP.SRAM_START);
        while (data.length > 0) {
            this._dbg.bargraph_update({"value": addr});
            let block = data.slice(0, this._stlink.maximum_transfer_size);
            data = data.slice(this._stlink.maximum_transfer_size);
            await flash.write(addr, block);
            if (verify) {
                this._dbg.debug(`Verifying ${block.length} bytes`);
                let flashed_data = await this._stlink.get_mem32(addr, block.length);
                let flashed_block = new Uint8Array(flashed_data.buffer);
                let verified = false;
                if (flashed_block.length == block.length) {
                    verified = flashed_block.every((octet, index) => octet == block[index]);
                }
                if (!verified) {
                    throw new Exception("Verify error at block address: 0x" + H32(addr));
                }
            }
            addr += block.length;
        }
        await flash.lock();
        this._dbg.bargraph_done();
    }
    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let addr_str = (addr !== null) ? `0x{H32(addr)}` : 'None';
        this._dbg.debug(`Stm32FP.flash_write(${addr_str}, [data:${data.length}Bytes], erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`);
        if (addr === null) {
            addr = this.FLASH_START;
        } else {
            if (addr % 2) {
                throw new Exception("Start address is not aligned to half-word");
            }
        }
        await this._flash_write(addr, data, arguments[2]);
    }
}

// support STM32F MCUs with page access to FLASH and two banks
// (STM32F1xxxF and STM32F1xxxG) (XL devices)
const STM32FPXL_BANK_SIZE = 512 * 1024;
class Stm32FPXL extends Stm32FP {
    async flash_erase_all() {
        this._dbg.debug("Stm32F1.flash_erase_all()");
        await this._flash_erase_all(0);
        await this._flash_erase_all(1);
    }

    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let options = arguments[2];
        let addr_str = (addr !== null) ? `0x${H32(addr)}` : 'None';
        this._dbg.debug(`Stm32F1.flash_write(${addr_str}, [data:${data.length}Bytes], erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`);
        var addr_bank1, addr_bank2, data_bank1, data_bank2;
        if (addr === null) {
            addr = this.FLASH_START;
        } else {
            if (addr % 2) {
                throw new Exception("Start address is not aligned to half-word");
            }
        }
        if ((addr - this.FLASH_START + data.length) <= STM32FPXL_BANK_SIZE) {
            await this._flash_write(addr, data, { ...options, bank: 0 });
        } else {
            if ((addr - this.FLASH_START) > STM32FPXL_BANK_SIZE) {
                await this._flash_write(addr, data, { ...options, bank: 1 });
            } else {
                addr_bank1 = addr;
                addr_bank2 = (this.FLASH_START + STM32FPXL_BANK_SIZE);
                data_bank1 = data.slice(0, (STM32FPXL_BANK_SIZE - (addr - this.FLASH_START)));
                data_bank2 = data.slice(STM32FPXL_BANK_SIZE - (addr - this.FLASH_START));
                await this._flash_write(addr_bank1, data_bank1, { ...options, bank: 1 });
                await this._flash_write(addr_bank2, data_bank2, { ...options, bank: 1 });
            }
        }
    }
}

export { Stm32FP, Stm32FPXL };
