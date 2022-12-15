# Test Roms

This folder contains all of the test roms that were used during development.
Mainly a subest of what can be found at https://github.com/christopherpow/nes-test-roms
or on the wiki: https://www.nesdev.org/wiki/Emulator_tests

## A quick note about Nestest

Nestest is the first major hurdle to cross. Once you move beyond calling cpu functions directly and start running full roms, this is the first "whole system" challenge.

Read the txt file completely as it explains how to run it. I would add some notes about the format of the log. It uses fixed width columns up until the last field:

| Section | Width | Notes |
| ------- | ----- | ----- |
| Program Counter | 6 | |
| OP Code and args | 10 | * at the end is an illegal instruction I think |
| Assembly | 32 | Shows values so have a way to read memory without side effects |
| Registers | 21 | |
| PPU info | 12 | Format is Scanline,Dot (Do the counters early to match the log) |
| CPU Cycles | NA | PPU runs 3x so should be a third of Dot for the first 40-ish lines |

**Example Line**
<pre>EBB9  E3 45    *ISB ($45,X) @ 47 = 0647 = FF    A:FF X:02 Y:AB P:A5 SP:FB PPU:161,209 CYC:18370</pre>

## Test Matrix

This is the current state of what has been tested:

### CPU Tests

| Name   | Pass / Fail | Notes
| ----          | ---- | -----
| nestest       | Pass | Success beep doesn't go away?
| instr_test-v5 | In Progress |
| - 03-immediate | Fail | Not handling some illegal opcodes properly
| - 07-abs_xy   | Fail | Not handling some illegal opcodes properly
| cpu_reset     | Pass | 

### APU Tests

| Name           | Pass / Fail | Notes
| ----           | ---- | -----
| blargg_apu     | In Progress | Some pass but DMC unimplemented and cpu runs too fast
| - 4-jitter     | Fail | "Frame IRQ is set too late" but PPU timing is fine on checking in debugger. Some instructions don't wait long enough
| - 5-len_timing | Fail | "First length of mode 0 too late" See above probably
| - 6-irq_flag_timing | Fail | Too soon now
| - 7-dmc_basics | Fail | Unimplemented
| - 8-dmc_rates  | Fail | Unimplemented

### PPU Tests

| Name           | Pass / Fail | Notes
| ----           | ---- | -----
| blargg_ppu_tests | In Progress |
| - power_up_palette | Fail | Each NES will have a random palette ram state on boot cause quanum mechaincs so won't fix. Kept in the table to mentione that.
| - vbl_clear_time | unstable | Error #3 Too late. Now inconsistent. Likely because of the way cycles/frame is mapped back to the emulator, I'm gaining extra ppu cycles on occasion
| - vram_access | Fail | Error #2. VRAM reads should be buffered. Not implemented
















<!--- Our little secret... -->
<style>
table td:nth-of-type(1) {
    white-space: nowrap;
}
</style>