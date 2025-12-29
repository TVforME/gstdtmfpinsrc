# DTMF PIN Detection Test Program

This directory contains a comprehensive test program for the `dtmfpinsrc` GStreamer plugin.

## Overview

The test program monitors bus messages from the plugin and executes corresponding functions when PIN codes are detected. It provides a complete testing framework with all 11 function mappings from the default `codes.pin` configuration file.

## Building

### Using Makefile

```bash
make clean
make all
```

### Using Meson

```bash
meson setup builddir
cd builddir
ninja
```

## Usage

### Basic Test

```bash
./test_dtmfpinsrc ../dtmf_test_complete.wav ../codes.pin
```

### With Custom Files

```bash
./test_dtmfpinsrc /path/to/audio.wav /path/to/config.pin
```

## Function Mappings

The test program implements the following function mappings:

| PIN | Function Name | Description |
| --- | --- | --- |
| 1234 | unlock\_front\_door | Unlocks the front door |
| 5678 | activate\_alarm | Activates the security alarm |
| 9999 | emergency\_shutdown | Performs emergency shutdown |
| 1111 | test\_mode | Enters test mode |
| 2468 | guest\_access | Grants guest access |
| 1357 | admin\_mode | Enters admin mode |
| \*123 | reset\_system | Resets the system |
| #123 | hash\_test\_pin | Tests hash PIN functionality |
| ABCD | test\_abcd\_mode | Tests ABCD DTMF digits |
| 9ABC | mixed\_digit\_test | Tests mixed digit PINs |
| B123 | extended\_pin\_test | Tests extended PIN codes |

## Expected Output

When a valid PIN is detected, you'll see:

```
═════════════════════════════════════════════════════════════
✅ VALID PIN DETECTED: 1234 -> unlock_front_door
═════════════════════════════════════════════════════════════
🔍 Looking for function: 'unlock_front_door'
📋 Description: Unlocks the front door
⚡ Executing function...
  🚪 UNLOCKING FRONT DOOR...
  → Access granted
  → Door unlocked
✓ Function executed successfully
```

When an invalid PIN is detected:

```
❌ INVALID PIN: 9999
```

## Pipeline Configuration

The test program uses the following GStreamer pipeline:

```
filesrc → decodebin → audioconvert → audioresample → 
dtmfpinsrc → autoaudiosink
```

Key features:

-   **Audio Resampling**: Ensures 8000Hz sample rate for DTMF detection
-   **Pass-through Mode**: Audio is played through speakers (can hear DTMF tones)
-   **Bus Monitoring**: Captures PIN detection messages
-   **Function Execution**: Executes corresponding functions for valid PINs

## Adding New Functions

To add a new function mapping:

1.  Add the function declaration:

```c
static gboolean your_function_name(void);
```

2.  Add the function implementation:

```c
static gboolean your_function_name(void)
{
    g_print("  🎯 YOUR FUNCTION...\n");
    g_print("  → Implementation here\n");
    return TRUE;
}
```

3.  Add to the function\_map array:

```c
{"your_pin", your_function_name, "Your function description"},
```

4.  Update `codes.pin` with the new PIN mapping.

## Troubleshooting

### Plugin Not Found

Ensure the plugin is installed:

```bash
# From parent directory
cd ..
make install
```

### No Audio Output

Check that `autoaudiosink` can access your audio device.

### DTMF Not Detected

-   Verify audio file contains DTMF tones
-   Ensure sample rate is 8000Hz
-   Check configuration file format

## Clean Build

```bash
make clean
```

## Install (Optional)

```bash
sudo make install
```

This installs `test_dtmfpinsrc` to `/usr/local/bin/`.