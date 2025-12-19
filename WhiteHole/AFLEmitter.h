/*
    File: AFLEmitter.h
    
    WhiteHole → AFL Emission Extension (PLUS-PLUS stream)
    Enables WhiteHole to emit AFL-format output for downstream consumers
    
    SplitMixTernary SEED = 1069
    Copyright (C) 2025 bmorphism
*/

#ifndef AFLEmitter_h
#define AFLEmitter_h

#include "WhiteHoleColor.h"
#include "AFLBridge.h"
#include <stdbool.h>

//==================================================================================================
#pragma mark -
#pragma mark Emission Configuration
//==================================================================================================

typedef enum {
    AFL_EMIT_JSON,        // Emit to JSON files
    AFL_EMIT_UNIX_SOCKET, // Emit via Unix domain socket
    AFL_EMIT_SHARED_MEM,  // Emit via shared memory
    AFL_EMIT_NATS         // Emit via NATS message queue
} AFLEmitMode;

typedef struct {
    AFLEmitMode mode;
    char output_path[256];       // For JSON mode
    char socket_path[256];       // For Unix socket mode
    char shm_name[64];           // For shared memory mode
    char nats_subject[128];      // For NATS mode
    float emit_interval_ms;      // Minimum interval between emissions
    bool emit_on_change;         // Only emit when parameters change significantly
    float change_threshold;      // Threshold for "significant" change (0.01 = 1%)
} AFLEmitConfig;

//==================================================================================================
#pragma mark -
#pragma mark Audio Analysis → AFL Parameter Derivation
//==================================================================================================

typedef struct {
    // Spectral analysis results
    float fundamental_freq;     // Estimated pitch (Hz)
    float spectral_centroid;    // Brightness measure
    float spectral_flux;        // Rate of spectral change
    float rms_energy;           // Loudness
    float zero_crossing_rate;   // Noisiness indicator
    
    // Derived AFL parameters
    AFLSpeechParams derived_speech;
    AFLVisualParams derived_visual;
    ColorHash derived_color;
    StreamTag derived_tag;
} AudioAnalysisResult;

// Analyze audio buffer and derive AFL parameters
AudioAnalysisResult analyze_audio_for_afl(
    const float* audio_buffer,
    size_t frame_count,
    uint32_t channel_count,
    float sample_rate
);

//==================================================================================================
#pragma mark -
#pragma mark Emitter Lifecycle
//==================================================================================================

typedef struct AFLEmitterState AFLEmitterState;

// Create emitter with configuration
AFLEmitterState* afl_emitter_create(const AFLEmitConfig* config);

// Destroy emitter and clean up resources
void afl_emitter_destroy(AFLEmitterState* state);

// Process audio through WhiteHole and emit AFL parameters
bool afl_emitter_process(
    AFLEmitterState* state,
    const MaybeAudioBuffer* audio,
    const StreamColor* stream_color
);

// Force immediate emission of current state
bool afl_emitter_flush(AFLEmitterState* state);

// Get current derived parameters (for inspection)
bool afl_emitter_get_current(
    const AFLEmitterState* state,
    AFLSpeechParams* out_speech,
    AFLVisualParams* out_visual
);

//==================================================================================================
#pragma mark -
#pragma mark Color-to-Visual Mapping
//==================================================================================================

// Map a StreamColor to visual parameters
// Uses balanced ternary trits to influence HSB values
static inline AFLVisualParams color_to_visual(const StreamColor* color) {
    AFLVisualParams visual;
    
    // Map first 3 trits to hue (120° per trit sign)
    int hue_sum = 0;
    for (int i = 0; i < 3; i++) {
        hue_sum += color->tag.trits[i];
    }
    visual.hue = 180.0f + (hue_sum * 40.0f);  // Center at 180°, ±120°
    if (visual.hue < 0) visual.hue += 360.0f;
    if (visual.hue >= 360.0f) visual.hue -= 360.0f;
    
    // Map trits 3-4 to saturation
    int sat_sum = color->tag.trits[3] + color->tag.trits[4];
    visual.saturation = 0.5f + (sat_sum * 0.2f);  // 0.1 - 0.9 range
    
    // Map trits 5-6 to brightness
    int bright_sum = color->tag.trits[5] + color->tag.trits[6];
    visual.brightness = 0.5f + (bright_sum * 0.2f);  // 0.1 - 0.9 range
    
    // Hash-derived secondary parameters
    visual.size = 1.0f + ((color->hash % 100) / 100.0f);  // 1.0 - 2.0
    visual.speed = 0.5f + ((color->hash >> 8) % 100) / 66.0f;  // 0.5 - 2.0
    visual.complexity = 1.0f + ((color->hash >> 16) % 20);  // 1 - 20
    visual.opacity = 0.7f + ((color->hash >> 24) % 30) / 100.0f;  // 0.7 - 1.0
    
    return visual;
}

// Map audio energy to speech parameters
static inline AFLSpeechParams energy_to_speech(
    float fundamental_freq,
    float rms_energy,
    float spectral_centroid,
    float spectral_flux
) {
    AFLSpeechParams speech;
    
    speech.pitch = fundamental_freq;
    speech.volume = rms_energy;
    speech.rate = 0.5f + spectral_flux;  // Faster change = faster rate
    speech.richness = spectral_centroid / 2000.0f;  // Normalize to typical range
    speech.stress = rms_energy * spectral_flux * 2.0f;  // High energy + flux = stress
    
    // Clamp to valid ranges
    if (speech.pitch < 50.0f) speech.pitch = 50.0f;
    if (speech.pitch > 1000.0f) speech.pitch = 1000.0f;
    if (speech.volume > 1.0f) speech.volume = 1.0f;
    if (speech.rate > 2.0f) speech.rate = 2.0f;
    if (speech.richness > 2.0f) speech.richness = 2.0f;
    if (speech.stress > 1.0f) speech.stress = 1.0f;
    
    return speech;
}

//==================================================================================================
#pragma mark -
#pragma mark Seed 1069 Emission Preset
//==================================================================================================

// Pre-configured emitter for Seed 1069 compatible streams
static inline AFLEmitConfig seed_1069_emit_config(void) {
    AFLEmitConfig config;
    
    config.mode = AFL_EMIT_JSON;
    strncpy(config.output_path, "/tmp/whitehole_afl_output", sizeof(config.output_path));
    config.emit_interval_ms = 16.67f;  // ~60fps
    config.emit_on_change = true;
    config.change_threshold = 0.02f;  // 2% change threshold
    
    // Set NATS subject for distributed streaming
    strncpy(config.nats_subject, "whitehole.afl.seed1069", sizeof(config.nats_subject));
    
    return config;
}

//==================================================================================================
#pragma mark -
#pragma mark Trifurcation Metadata
//==================================================================================================

// Embed trifurcation identity in emission
typedef struct {
    int8_t stream_sign;        // -1, 0, or +1
    char stream_role[32];      // "MINUS", "ERGODIC", or "PLUS"
    uint32_t seed;             // SplitMixTernary seed (1069)
    uint64_t timestamp_ns;     // Emission timestamp
} TrifurcationMeta;

static inline TrifurcationMeta create_plus_plus_meta(void) {
    TrifurcationMeta meta;
    meta.stream_sign = +1;
    strncpy(meta.stream_role, "PLUS-PLUS", sizeof(meta.stream_role));
    meta.seed = 1069;
    meta.timestamp_ns = 0;  // Set at emission time
    return meta;
}

#endif /* AFLEmitter_h */
