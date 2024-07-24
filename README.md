# riscv-emulator
A RISC-V emulator capable of running Linux, written in C++.
![The emulator running Linux with a buildroot userspace](screenshots/linux_screenshot.png)

## Supported Extensions
* RV64I
* A (atomics)
* C (compressed instructions)
* M (multiplication)
* Zicsr (CSRs)

Sv39 paging is also supported.

## Supported Peripherals
* CLINT
* PLIC
* UART (simple-uart driver for the above Linux)

## Test Coverage
You can find the official RISC-V test suite [here](https://github.com/riscv-software-src/riscv-tests).
Currently, the emulator is capable of passing all tests for the base instruction set and supported extensions.

## Running Tests
For running the RISC-V test suite, you may wish to build a freestanding toolchain.
```
export RISCV=/path/to/my/new/toolchain
git clone https://github.com/riscv/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=$RISCV --with-arch=rv64gc
make -j 16
```
Alternatively, download a pre-built binary from the releases page (riscv64-elf-ubuntu-*-gcc-nightly) or, on Arch Linux, install the AUR package [riscv64-gnu-toolchain-elf-bin](https://aur.archlinux.org/packages/riscv64-gnu-toolchain-elf-bin).

Then build the tests as so:
```
# clone and configure
cd external
git clone https://github.com/riscv/riscv-tests
cd riscv-tests
git submodule update --init --recursive
autoconf
./configure

# build relevant tests
cd isa
make -j 16 rv64ui rv64ua rv64uc rv64um rv64si rv64mi
rm *-v-*

# convert ELF to bin
rm -f *.bin
for file in *; do if [[ -x "$file" && ! -d "$file" ]]; then riscv64-unknown-elf-objcopy -O binary "$file" "$file.bin"; fi; done
```

Then run the tests:
```
# in project's root directory
python run_tests.py
```

## Running Linux
Install the Linux toolchain (as opposed to the elf toolchain) by following the instructions above but running `make linux` instead of `make`. If you wish to download a pre-built version instead, follow the same steps as previously described but swap `-elf-` with `-glibc-`. Then:
```
# Build
cd external/linux
chmod +x build.sh
./build.sh

# Run
cd -
cd build
cmake .. -G Ninja && ninja
./riscv-emulator ../external/linux/build/opensbi/build/platform/generic/firmware/fw_payload.bin
```