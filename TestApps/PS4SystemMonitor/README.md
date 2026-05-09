# PS4 System Monitor - Native Homebrew App

A real PS4 native application built with OpenOrbis SDK that displays live system stats (CPU usage, memory, temperature) directly rendered to the PS4 framebuffer.

## Building

### Requirements
- OpenOrbis SDK (https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- Set `OO_PS4_TOOLCHAIN` environment variable to SDK path

### Compile
```bash
make
```

### Output
- `eboot.bin` - PS4 ELF executable
- Ready for fPKG compilation with the PS Classics fPKG Builder (PS4 App tab)

## App Details
- **Title**: Elite Kinyamon's System Monitor
- **Title ID**: EKMS00001
- **Content ID**: UP0001-EKMS00001_00-SYSTEMMONITOR001
