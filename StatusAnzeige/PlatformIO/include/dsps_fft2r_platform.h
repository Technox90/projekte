#pragma once

// PNGdec detects this optional ESP-DSP header on ESP32-S3.
// StatusAnzeige does not need the SIMD RGB565 path, so disable it.
// This keeps PNGdec portable without pulling the full ESP-DSP component.
#define dsps_fft2r_sc16_aes3_enabled 0
