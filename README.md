# DTMF PIN Detection Plugin for GStreamer

A GStreamer plugin for detecting DTMF (Dual-Tone Multi-Frequency) PIN codes in audio streams with configurable actions and timeout handling.

## Features

-   ✅ **DTMF Detection**: Detects all 16 DTMF digits (0-9, \*, #, A, B, C, D)
-   ✅ **PIN Validation**: Validates PIN codes against configuration file
-   ✅ **Timeout Handling**: Configurable inter-digit and entry timeouts
-   ✅ **Bus Messages**: Emits clean GStreamer bus messages for detected PINs
-   ✅ **Pass-through Mode**: Optional audio pass-through for monitoring
-   ✅ **Sample Rate Support**: Optimized for 8000 Hz input (required for spandsp)

## How It Works

### Architecture

The plugin is a GStreamer Base Transform element that:

1.  **Receives Audio Input**: Accepts raw audio (S16LE, 8000 Hz recommended)
2.  **Detects DTMF Tones**: Uses spandsp library for reliable DTMF detection
3.  **Accumulates Digits**: Builds PIN code from detected digits
4.  **Validates PINs**: Matches against configuration file
5.  **Emits Messages**: Sends `pin-detected` bus messages with results
6.  **Handles Timeouts**: Resets buffer on inter-digit or entry timeout
7.  **Passes Audio**: Optionally passes audio through unchanged

### Pipeline Example

```
filesrc location=audio.wav
    ↓
decodebin
    ↓
audioconvert
    ↓
audioresample
    ↓
audio/x-raw,rate=8000
    ↓
dtmfpinsrc config-file=codes.pin
    ↓
audioconvert
    ↓
autoaudiosink
```

### Detection Process

```
Audio Stream → DTMF Detection → Digit Accumulation → PIN Validation → Bus Message
     ↓              ↓                  ↓                    ↓                  ↓
  S16LE 8000Hz    spandsp        PIN Buffer         codes.pin       Application
                                      ↓
                               Timeout Check
                                      ↓
                            Inter-digit (3000ms)
                            Entry (10000ms)
```

### Bus Messages

When a PIN is detected, the plugin emits a `pin-detected` bus message:

```c
// Valid PIN
{
  "message-name": "pin-detected",
  "pin": "1234",
  "function": "open_door",
  "valid": TRUE
}

// Invalid PIN
{
  "message-name": "pin-detected",
  "pin": "1111",
  "function": "",
  "valid": FALSE
}
```

## Building

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt-get install \
    build-essential \
    make \
    meson \
    ninja-build \
    pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libspandsp-dev

# Build dependencies are auto-managed by Makefile
```

### Using Make (Recommended)

```bash
# Clone or navigate to the plugin directory
git clone https://github.com/TVforME/gstreamer/gstdtmfpinsrc
cd gstdtmfpinsrc

# Build the plugin
make clean && make

# Install system-wide
sudo make install

# Uninstall
sudo make uninstall
```

### Using Meson

```bash
# Setup build
meson setup builddir

# Build
meson compile -C builddir

# Install
meson install -C builddir
```

## Configuration

### PIN Configuration File

**Format**: `PIN=function_name`

**Comments**: Lines starting with `;` or `#`

**Valid DTMF Characters**: 0-9, \*, #, A, B, C, D

**Example** (`codes.pin`):

```
; DTMF PIN Configuration File
; Format: PIN=function_name

; Door access PINs
1234=open_door
5678=unlock_garage
0000=admin_mode

; Emergency PINs
9999=emergency_shutdown
*911=emergency_call

; Special access codes
*A1B=special_code
C23D=maintenance_mode
ABC#=hello_world

; Test PINs
1111=test_mode
2222=guest_access

; This is a comment
#123=hash_pin  ; # is now a valid DTMF digit, not a comment
```

### Plugin Properties

| Property | Type | Default | Description |
| --- | --- | --- | --- |
| `config-file` | string | "codes.pin" | Path to PIN configuration file |
| `inter-digit-timeout` | uint | 3000 | Timeout between digits (ms) |
| `entry-timeout` | uint | 10000 | Timeout for complete entry (ms) |
| `pass-through` | boolean | FALSE | Allow audio pass-through |

### Usage Examples

#### gst-launch-1.0

```bash
# With audio monitoring (pass-through=true)
gst-launch-1.0 filesrc location=audio.wav ! \
  decodebin ! audioconvert ! audioresample ! \
  audio/x-raw,rate=8000 ! \
  dtmfpinsrc config-file=codes.pin pass-through=true ! \
  audioconvert ! autoaudiosink

# Background detection only (pass-through=false)
gst-launch-1.0 filesrc location=audio.wav ! \
  decodebin ! audioconvert ! audioresample ! \
  audio/x-raw,rate=8000 ! \
  dtmfpinsrc config-file=codes.pin pass-through=false ! \
  audioconvert ! autoaudiosink
```

#### C Application

```c
#include <gst/gst.h>

// Setup pipeline
GstElement *pipeline = gst_parse_launch(
    "filesrc location=audio.wav ! decodebin ! "
    "audioconvert ! audioresample ! audio/x-raw,rate=8000 ! "
    "dtmfpinsrc config-file=codes.pin ! "
    "audioconvert ! autoaudiosink", NULL);

// Add bus watch to handle pin-detected messages
GstBus *bus = gst_element_get_bus(pipeline);
gst_bus_add_watch(bus, bus_call, loop);

// Start pipeline
gst_element_set_state(pipeline, GST_STATE_PLAYING);

// Main loop
g_main_loop_run(loop);

// Bus callback
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    if (gst_message_has_name(msg, "pin-detected")) {
        const GstStructure *s = gst_message_get_structure(msg);
        
        const gchar *pin = gst_structure_get_string(s, "pin");
        const gchar *function = gst_structure_get_string(s, "function");
        gboolean valid;
        gst_structure_get_boolean(s, "valid", &valid);
        
        if (valid) {
            g_print("Valid PIN: %s -> %s\n", pin, function);
            // Execute function
        } else {
            g_print("Invalid PIN: %s\n", pin);
        }
    }
    return TRUE;
}
```

## Testing

### Test Program

```bash
cd dtmfpinsrc/test

# Build test program
make install

# Run comprehensive test
./test_dtmfpinsrc ../test_dtmf.wav ../codes.pin
```

### Test File Coverage

The `test_dtmf.wav` file (91.40 seconds) includes:

-   **7 Valid PINs**: 1234, 5678, 9999, 0000, \*A1B, C23D, ABC#
-   **5 Invalid PINs**: 1111, 2222, 123 (too short), 12345 (too long), ABCD
-   **2 Timeout Scenarios**: Inter-digit timeout (4s), Entry timeout (12s)

**Note**: 4-second gaps between tests ensure proper buffer reset.

### Expected Test Results

```
✅ VALID PIN DETECTED: 1234 -> open_door
✅ VALID PIN DETECTED: 5678 -> unlock_garage
✅ VALID PIN DETECTED: 9999 -> emergency_shutdown
✅ VALID PIN DETECTED: 0000 -> admin_mode
✅ VALID PIN DETECTED: *A1B -> special_code
✅ VALID PIN DETECTED: C23D -> commented_example
✅ VALID PIN DETECTED: ABC# -> hello_world

❌ INVALID PIN: 1111
❌ INVALID PIN: 2222
...
```

### Generating Custom Test Files

```bash
cd dtmfpinsrc

# Generate test file with default configuration
python3 generate_dtmf_test.py

# Edit generate_dtmf_test.py to customize:
# - Test sequences
# - Timing parameters
# - Reset gaps between tests
```

## Technical Details

### Sample Rate Requirements

**Required**: 8000 Hz input for optimal DTMF detection

**Reason**: The spandsp library requires 8000 Hz for accurate DTMF tone detection. Other sample rates may result in unreliable detection.

**Solution**: Use `audioresample` element in your pipeline:

```
audioresample ! audio/x-raw,rate=8000 ! dtmfpinsrc
```

### DTMF Frequency Pairs

| Digit | Low (Hz) | High (Hz) |
| --- | --- | --- |
| 1 | 697 | 1209 |
| 2 | 697 | 1336 |
| 3 | 697 | 1477 |
| A | 697 | 1633 |
| 4 | 770 | 1209 |
| 5 | 770 | 1336 |
| 6 | 770 | 1477 |
| B | 770 | 1633 |
| 7 | 852 | 1209 |
| 8 | 852 | 1336 |
| 9 | 852 | 1477 |
| C | 852 | 1633 |
| \* | 941 | 1209 |
| 0 | 941 | 1336 |
| # | 941 | 1477 |
| D | 941 | 1633 |

### Timeout Behavior

**Inter-Digit Timeout** (default: 3000ms)

-   Triggers when no new digit arrives within timeout period
-   Resets PIN buffer
-   Emits timeout message (if PIN was partially entered)

**Entry Timeout** (default: 10000ms)

-   Triggers when total entry time exceeds timeout
-   Resets PIN buffer
-   Emits timeout message (if PIN was partially entered)

**Buffer Reset**: Both timeouts clear the PIN buffer and return to initial state

## Troubleshooting

### Issue: No DTMF detection

**Solution**: Ensure sample rate is 8000 Hz:

```bash
# Add audioresample to pipeline
audioresample ! audio/x-raw,rate=8000 ! dtmfpinsrc
```

### Issue: PIN not detected

**Solution**: Check that:

1.  PIN is in `codes.pin` configuration file
2.  Comment character is `;` (not `#`)
3.  PIN uses valid DTMF characters only
4.  Sample rate is 8000 Hz

### Issue: Timeouts not triggering

**Solution**: Verify timeout settings:

```bash
# Check timeout values
gst-inspect-1.0 dtmfpinsrc

# Set custom timeouts
dtmfpinsrc inter-digit-timeout=3000 entry-timeout=10000
```
## Documentation

| File | Description |
| --- | --- |
| `README.md` | This file - main documentation |
| `test/README.md` | Test program documentation |

## Project Structure

```
standalone_dtmfpinsrc/
├── src/
│   ├── gstdtmfpinsrc.c       # Plugin source code
│   ├── gstdtmfpinsrc.h       # Plugin header
│   └── config.h.in           # Build configuration
├── test/
│   ├── test_dtmfpinsrc.c     # Test program
│   ├── codes.pin             # PIN configuration
│   ├── test_dtmf.wav         # Test audio file (91.40s)
│   ├── generate_dtmf_test.py # test_dtmf.wav generator
│   ├── Makefile              # Make build system
│   ├── meson.build           # Meson build system
│   └── README.md             # Test documentation
├── Makefile                  # Main Makefile
├── meson.build               # Main meson.build
└── README.md                 # This file
```

## Summary

The DTMF PIN Detection Plugin provides:

-   ✅ Reliable DTMF detection using spandsp
-   ✅ Configurable PIN validation
-   ✅ Timeout handling for security
-   ✅ Clean bus message interface
-   ✅ Optional audio pass-through
-   ✅ Comprehensive testing
-   ✅ Full documentation

**Status**: Production Ready ✅

The plugin is fully functional, tested, and ready for deployment in production environments.
