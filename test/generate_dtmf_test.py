#!/usr/bin/env python3
"""
Generate comprehensive DTMF test WAV file matching codes.pin configuration.

This script creates a test file with:
1. Valid PINs (should trigger success)
2. Invalid PINs (should trigger error)
3. Timeout scenarios (inter-digit and entry timeout)
4. All DTMF digits (0-9, *, #, A, B, C, D)
"""

import numpy as np
import wave
import struct

class DTMFGenerator:
    """Generate DTMF tones according to ITU-T Q.24 specification."""
    
    # DTMF frequency pairs (Hz)
    # Low frequencies: 697, 770, 852, 941
    # High frequencies: 1209, 1336, 1477, 1633
    FREQUENCIES = {
        '1': (697, 1209),
        '2': (697, 1336),
        '3': (697, 1477),
        'A': (697, 1633),
        '4': (770, 1209),
        '5': (770, 1336),
        '6': (770, 1477),
        'B': (770, 1633),
        '7': (852, 1209),
        '8': (852, 1336),
        '9': (852, 1477),
        'C': (852, 1633),
        '*': (941, 1209),
        '0': (941, 1336),
        '#': (941, 1477),
        'D': (941, 1633),
    }
    
    def __init__(self, sample_rate=8000, sample_width=2, channels=1):
        self.sample_rate = sample_rate
        self.sample_width = sample_width
        self.channels = channels
    
    def generate_tone(self, digit, duration):
        """Generate DTMF tone for a specific digit."""
        low_freq, high_freq = self.FREQUENCIES[digit]
        
        # Generate time array
        t = np.linspace(0, duration, int(self.sample_rate * duration), False)
        
        # Generate sine waves for both frequencies
        tone = (0.5 * np.sin(2 * np.pi * low_freq * t) + 
                0.5 * np.sin(2 * np.pi * high_freq * t))
        
        # Convert to 16-bit PCM
        audio = (tone * 32767).astype(np.int16)
        
        # Stereo if needed
        if self.channels == 2:
            audio = np.column_stack((audio, audio))
        
        return audio
    
    def generate_silence(self, duration):
        """Generate silence."""
        samples = int(self.sample_rate * duration)
        if self.channels == 2:
            silence = np.zeros((samples, 2), dtype=np.int16)
        else:
            silence = np.zeros(samples, dtype=np.int16)
        return silence
    
    def generate_pin_sequence(self, pin, digit_duration=0.1, gap_duration=0.15, pause_after=0.5):
        """Generate a complete PIN code sequence."""
        audio_segments = []
        
        for digit in pin:
            audio_segments.append(self.generate_tone(digit, digit_duration))
            audio_segments.append(self.generate_silence(gap_duration))
        
        # Add pause after PIN entry (allows for PIN validation/timeout)
        audio_segments.append(self.generate_silence(pause_after))
        
        # Concatenate all segments
        return np.concatenate(audio_segments)
    
    def write_wav(self, filename, audio_data):
        """Write audio data to WAV file."""
        with wave.open(filename, 'wb') as wav_file:
            wav_file.setnchannels(self.channels)
            wav_file.setsampwidth(self.sample_width)
            wav_file.setframerate(self.sample_rate)
            wav_file.writeframes(audio_data.tobytes())

def main():
    """Generate comprehensive DTMF test file."""
    
    print("🎵 DTMF Test File Generator")
    print("=" * 50)
    
    # Initialize generator for 8000Hz (required by spandsp)
    gen = DTMFGenerator(sample_rate=8000, sample_width=2, channels=1)
    
    # Test sequence configuration
    # Format: (PIN, description, expected_result)
    test_sequences = [
        # Valid PINs from codes.pin
        ('1234', 'Valid: open_door', 'valid'),
        ('5678', 'Valid: unlock_garage', 'valid'),
        ('9999', 'Valid: emergency_shutdown', 'valid'),
        ('0000', 'Valid: admin_mode', 'valid'),
        ('*A1B', 'Valid: special_code', 'valid'),
        ('C23D', 'Valid: commented_example', 'valid'),
        ('ABC#', 'Valid: hello_world', 'valid'),
        
        # Invalid PINs
        ('1111', 'Invalid: not in config', 'invalid'),
        ('2222', 'Invalid: not in config', 'invalid'),
        ('123', 'Invalid: too short', 'invalid'),
        ('12345', 'Invalid: too long', 'invalid'),
        ('ABCD', 'Invalid: not in config', 'invalid'),
        
        # Timeout tests
        ('1', 'Timeout: inter-digit pause after 1', 'timeout_interdigit'),
        ('2', 'Timeout: entry timeout after 2', 'timeout_entry'),
    ]
    
    all_audio = []
    
    print("\n📝 Generating test sequences:")
    print("-" * 50)
    
    for pin, description, expected in test_sequences:
        print(f"  {description} - PIN: {pin}")
        
        if expected == 'timeout_interdigit':
            # Generate first digit, then long pause for inter-digit timeout
            all_audio.append(gen.generate_pin_sequence('1', digit_duration=0.1, gap_duration=0.1, pause_after=0.5))
            # Add 4-second pause (longer than 3000ms timeout)
            all_audio.append(gen.generate_silence(4.0))
            
        elif expected == 'timeout_entry':
            # Generate second digit, then long pause for entry timeout
            all_audio.append(gen.generate_pin_sequence('2', digit_duration=0.1, gap_duration=0.1, pause_after=0.5))
            # Add 12-second pause (longer than 10000ms timeout)
            all_audio.append(gen.generate_silence(12.0))
            
        else:
            # Normal PIN sequences
            all_audio.append(gen.generate_pin_sequence(pin, digit_duration=0.1, gap_duration=0.15, pause_after=0.5))
        
        # IMPORTANT: Add 4-second silence between test sequences to allow proper buffer reset
        # This ensures the PIN buffer clears before the next test
        # 4 seconds > 3000ms inter-digit timeout to trigger buffer reset
        all_audio.append(gen.generate_silence(4.0))
    
    # Concatenate all audio
    final_audio = np.concatenate(all_audio)
    
    # Write to WAV file
    output_file = 'test_dtmf.wav'
    gen.write_wav(output_file, final_audio)
    
    # Print statistics
    duration = len(final_audio) / gen.sample_rate
    file_size = len(final_audio) * gen.sample_width * gen.channels
    
    print("\n" + "=" * 50)
    print(f"✅ Generated: {output_file}")
    print(f"   Duration: {duration:.2f} seconds")
    print(f"   File size: {file_size / 1024:.1f} KB")
    print(f"   Sample rate: {gen.sample_rate} Hz")
    print(f"   Channels: {gen.channels}")
    print(f"   Test sequences: {len(test_sequences)}")
    print("\n📋 Test coverage:")
    print(f"   - Valid PINs: {sum(1 for _, _, e in test_sequences if e == 'valid')}")
    print(f"   - Invalid PINs: {sum(1 for _, _, e in test_sequences if e == 'invalid')}")
    print(f"   - Timeout tests: {sum(1 for _, _, e in test_sequences if 'timeout' in e)}")
    print("\n🎯 Expected results:")
    print("   Valid PINs → pin-detected message with valid=TRUE")
    print("   Invalid PINs → pin-detected message with valid=FALSE")
    print("   Timeout → pin-detected message with valid=FALSE")
    print("\n⚠️  IMPORTANT: 4-second gaps between tests ensure proper buffer reset")
    print("=" * 50)

if __name__ == '__main__':
    main()