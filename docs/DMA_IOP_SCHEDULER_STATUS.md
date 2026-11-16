# DMA, devices, scheduler and IOP experimental milestone

This milestone adds executable host-side scaffolding; it is not a complete PS2 DMAC or IOP implementation.

## DMA chain engine

- 128-bit tag decoder exposes QWC, PCE, ID, IRQ, ADDR, SPR and upper data.
- Incremental chain state supports REFE, CNT, NEXT, REF, REFS, CALL, RET and END.
- CALL/RET uses a bounded two-entry return stack; malformed RET, stack overflow, unmapped tag fetches and address overflow are rejected.
- One QWC is transferred per step through the EE physical bus and a device callback.
- Regression covers CNT/END, CALL/RET, REF/REFS/REFE, NEXT and invalid RET/address.
- Not yet wired into each channel's CHCR.MOD state machine; TTE/TIE, tag IRQ timing, SPR address translation and exact DMAC corner cases remain open.

## DPCR / DICR and CP0

- Experimental DPCR channel priority/enable nibble accessors.
- DICR completion flags, masks, master enable and write-one-to-clear behavior for two register banks.
- Completed DMA channels can assert EE Cause.IP2 (bit 10).
- Scheduler consumes fabric completion events and routes them through the interrupt helper.
- Register bit timing, channel/bank mapping, master-flag behavior and real EE interrupt controller routing still require differential verification.

## VIF / GIF / SIF FIFO primitives

- Separate bounded 64-QWC ring FIFOs for VIF0, VIF1, GIF, SIF0, SIF1 and SIF2.
- DMA callback adapter can push/pull qwords from an individual FIFO.
- This is queue storage and DMA plumbing only: VIF command decoding, GIF packet parsing, SIF protocol, backpressure, device timing and actual hardware FIFO depths/semantics are not implemented.

## IOP and scheduler

- Initial IOP R3000A integer subset: ADDIU, ANDI, ORI, LUI, ADDU, SUBU, AND, OR, SLT, SLL, JR, BEQ, BNE, J, JAL, LW and SW.
- IOP memory accesses use a caller-supplied bus callback. Exceptions, COP0, DMA channels, timers, interrupts, cache behavior and most opcodes are not implemented.
- Deterministic scheduler increments EE cycles, steps IOP at an experimental 1:8 ratio, services one DMA QWC per EE tick and forwards DMA completion to EE Cause.IP2.
- Clock ratio and event ordering are provisional pending hardware validation.

## Validation

The Makefile's `make check` includes regression executables for DMA tags/chains, DPCR/DICR, device FIFO, IOP and scheduler, in addition to existing EE, bus and library tests. GitHub Actions is configured to run `make check` on pushes to main and pull requests. Do not call the milestone CI-approved until a run for the current commit is visible and successful.

The Xenon XEX job remains manual and requires a provisioned self-hosted runner labelled `linux,xenon`; no `build/default.xex` is claimed by this milestone.


## CHCR TIE and tag IRQ follow-up

- CHCR TIE is defined as bit 7 and passed into the chain executor.
- A tag IRQ stops chain execution at that tag boundary only when TIE is set; the fabric reports the channel completion to the existing DICR/EE interrupt path.
- With TIE clear, the tag IRQ is ignored and chain execution proceeds.
- Regression tests cover both enabled and disabled TIE behavior.
- Chain execution errors now clear CHCR.STR and release the per-channel chain state rather than leaving a channel permanently started.
- These changes are source-level changes only until the GitHub Actions host test job succeeds; exact DMAC IRQ timing still needs hardware/differential validation.


## DPCR arbitration follow-up

- The DMA fabric now selects among runnable, enabled channels using the DPCR priority field instead of round-robin arbitration.
- Lower numeric priority values are selected first; equal-priority channels are resolved in favor of the higher channel number.
- The DMA bus regression now covers distinct priorities and equal-priority tie-breaking.
- This is source-level behavior and a regression specification; it is not yet CI-validated or hardware-verified.
