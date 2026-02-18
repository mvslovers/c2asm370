# c2asm370

A C cross-compiler that produces IBM System/370 assembler source from C code. Built on a stripped-down GCC 3.2.3, it runs on Linux (32-bit) and macOS (including Apple Silicon) and generates mainframe-ready assembler output.

## Background

c2asm370 is a fork of [gccmvs](https://gccmvs.sourceforge.net) by Paul Edwards. It was created by Michael Dean Rayborn (Mike Rayborn), who simplified the GCC 3.2.3 source tree to focus solely on generating assembler files from C source code.

Mike Rayborn has since retired from active development. With his agreement, the [mvslovers](https://github.com/mvslovers) community has taken over the project to preserve and continue its development.

## What It Does

c2asm370 compiles C source code into IBM System/370 assembler source files (`.s`). It does **not** produce object files — the generated assembler must be uploaded to a platform with an IBM-compatible assembler (e.g., MVS 3.8, z/OS, VM/CMS).

The generated output uses:
- **EBCDIC** character encoding
- **S/370 assembler** syntax with MVS calling conventions
- **PDPCLIB** macros for function prologue/epilogue

## Building

### Requirements

- **Linux:** GCC with 32-bit support (`gcc-multilib`)
- **macOS:** Xcode Command Line Tools (Apple Silicon and Intel supported)
- GNU Make

### Build

```bash
make                # Build the c2asm370 executable
make clean          # Remove build artifacts
make install        # Install to /usr/local/bin (set PREFIX to override)
```

> **Note:** The build is limited to `-O1` optimization due to memory management characteristics inherited from the GCC 3.2.3 source.

## Usage

```bash
./c2asm370 -I crent370/include -I <your include> -S myprogram.c
```

This produces `myprogram.s` containing the assembler source ready for upload to a mainframe assembler.

## Project Structure

| Directory | Contents |
|-----------|----------|
| `i370/` | IBM 370 target-specific code (machine definition, assembler output). **Check here first for bugs.** |
| `gcc/` | Core GCC 3.2.3 compiler (C frontend, RTL, optimization passes) |
| `libiberty/` | GNU support library |
| `include/` | Shared headers for libiberty and GCC internals |
| `macro/` | assembler macro definitions used by generated output |

## Credits

- **Paul Edwards** — Original [gccmvs](https://gccmvs.sourceforge.net) project
- **Mike Rayborn** — Created c2asm370 as a streamlined fork of gccmvs
- **MVSLOVERS** — Current maintainer

## License

This project is based on GCC 3.2.3 and is licensed under the GNU General Public License. See [COPYING](COPYING) for details.
