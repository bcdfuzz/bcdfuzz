# BCDFuzz

This repository contains the proof-of-concept implementation for our paper(#0871), submitted to the NDSS '26 Fall Cycle. BCDFuzz is a framework for discovering vulnerabilities in the BCD parsing logic of the closed-source Windows Boot Manager.

## Generating the Harness Shellcode

The Target-Embedded Harness used by BCDFuzz is implemented as a set of shellcodes patched into the `bootmgfw.efi` binary. The source code for these shellcodes is provided in `main.c`. To generate the final binary shellcodes, follow these steps:

1.  **Compile the Source:** Compile `main.c` within a standard EDK2 development environment. This will produce a UEFI application (`.efi` file).
2.  **Extract Functions:** Load the compiled `.efi` file into a disassembler such as IDA Pro.
3.  **Dump Shellcodes:** Locate the `First()` and `Second()` functions and dump their raw machine code. These binary dumps are the shellcodes used for patching.

### Critical Post-Patching Step: Updating the `gBS` Pointer

**Note:** This is a critical manual step required for the harness to function correctly.

The EDK2 compilation process hardcodes the address of the global Boot Services table pointer (`gBS`) within the generated shellcode. When this shellcode is relocated to a new base address inside the patched `bootmgfw.efi`, this hardcoded pointer becomes invalid, and the harness will fail.

Therefore, after patching the shellcodes into `bootmgfw.efi`, you **must manually update** the `gBS` pointer address within the shellcode to match the correct address used by the target `bootmgfw.efi` binary.

## Example Fuzzing Command

The following command provides a template for starting a fuzzing campaign with kAFL, using the patched bootloader and a generated seed corpus.

```bash
kafl fuzz \
  --bios /path/to/OVMF.fd \
  --qemu-base "-enable-kvm -machine kAFL64-v1 -net nic -net tap,ifname=tap0,script=no -cpu kAFL64-Hypervisor-v1,+vmx,+rdrand,+rdseed -no-reboot" \
  --qemu-extra="-drive file=/path/to/empty.img,index=0,media=disk -debugcon file:debug.log -global isa-debugcon.iobase=0x402" \
  --work-dir /dev/shm/kafl_uefi \
  --seed-dir /path/to/corpus \
  -m 4096 \
  --redqueen \
  --grimoire \
  --trace-cb \
  -v