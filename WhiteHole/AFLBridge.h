/*
    File: AFLBridge.h
    
    AFL → WhiteHole Bridge (PLUS-MINUS stream)
    Connects AFL speech synthesis parameters to WhiteHole audio routing
    
    SplitMixTernary SEED = 1069
    Copyright (C) 2025 bmorphism
*/

#ifndef AFLBridge_h
#define AFLBridge_h

#include "WhiteHoleColor.h"
#include <stdbool.h>

//==================================================================================================
#pragma mark -
#pragma mark AFL Parameter Types
//==================================================================================================

typedef struct {
    float pitch;      // 0-1000 Hz
    float rate;       // 0.0-2.0 speech rate multiplier
    float volume;     // 0.0-1.0 normalized volume
    float richness;   // 0.0-2.0 harmonic richness
    float stress;     // 0.0-1.0 stress emphasis
} AFLSpeechParams;

typedef struct {
    float hue;        // 0-360 degrees
    float saturation; // 0.0-1.0
    float brightness; // 0.0-1.0
    float size;       // 0.5-3.0 visual element size
    float speed;      // 0.5-2.0 animation speed
    float complexity; // 1-20 visual complexity
    float opacity;    // 0.0-1.0
} AFLVisualParams;

typedef struct {
    AFLSpeechParams speech;
    AFLVisualParams visual;
    ColorHash derived_color;
    StreamTag derived_tag;
} AFLBridgeState;

//==================================================================================================
#pragma mark -
#pragma mark AFL → WhiteHole Color Derivation
//==================================================================================================

// Derive StreamColor from AFL parameters
// Maps audio characteristics to color-tagged routing
static inline StreamColor afl_to_stream_color(const AFLBridgeState* state) {
    StreamColor color;
    
    // Hash combining speech and visual params
    uint32_t hash = 2166136261u;
    hash ^= (uint32_t)(state->speech.pitch * 100);
    hash *= 16777619u;
    hash ^= (uint32_t)(state->visual.hue);
    hash *= 16777619u;
    hash ^= (uint32_t)(state->speech.volume * 255);
    hash *= 16777619u;
    color.hash = hash;
    
    // Derive balanced ternary tag from speech characteristics
    // Map pitch to first 3 trits, rate/volume to next 4
    color.tag.trits[0] = state->speech.pitch < 300 ? TRIT_NEG : 
                         state->speech.pitch > 700 ? TRIT_POS : TRIT_ZERO;
    color.tag.trits[1] = state->speech.rate < 0.7 ? TRIT_NEG :
                         state->speech.rate > 1.3 ? TRIT_POS : TRIT_ZERO;
    color.tag.trits[2] = state->speech.volume < 0.3 ? TRIT_NEG :
                         state->speech.volume > 0.7 ? TRIT_POS : TRIT_ZERO;
    color.tag.trits[3] = state->speech.richness < 0.5 ? TRIT_NEG :
                         state->speech.richness > 1.5 ? TRIT_POS : TRIT_ZERO;
    color.tag.trits[4] = state->speech.stress < 0.3 ? TRIT_NEG :
                         state->speech.stress > 0.7 ? TRIT_POS : TRIT_ZERO;
    // Visual params influence final 2 trits
    color.tag.trits[5] = state->visual.brightness < 0.3 ? TRIT_NEG :
                         state->visual.brightness > 0.7 ? TRIT_POS : TRIT_ZERO;
    color.tag.trits[6] = state->visual.saturation < 0.3 ? TRIT_NEG :
                         state->visual.saturation > 0.7 ? TRIT_POS : TRIT_ZERO;
    
    snprintf(color.name, sizeof(color.name), "AFL-P%.0f-R%.1f", 
             state->speech.pitch, state->speech.rate);
    
    return color;
}

//==================================================================================================
#pragma mark -
#pragma mark Bridge Operations
//==================================================================================================

// Initialize bridge from JSON file paths
bool afl_bridge_init_from_json(AFLBridgeState* state, 
                                const char* afl_json_path,
                                const char* visual_json_path);

// Update bridge state from JSON (hot reload)
bool afl_bridge_update(AFLBridgeState* state,
                       const char* afl_json_path,
                       const char* visual_json_path);

// Check if AFL stream is compatible with WhiteHole stream
bool afl_stream_compatible(const AFLBridgeState* afl_state,
                           const StreamColor* whitehole_stream);

// Create audio buffer from AFL parameters (synthesize)
MaybeAudioBuffer afl_synthesize_buffer(const AFLBridgeState* state,
                                        float* output_buffer,
                                        size_t frame_count,
                                        uint32_t channel_count,
                                        float sample_rate);

// Route AFL output through WhiteHole with color validation
MaybeAudioBuffer afl_route_through_whitehole(
    const AFLBridgeState* afl_state,
    const MaybeAudioBuffer* afl_audio,
    const StreamColor* destination_color,
    float* output_buffer,
    size_t max_frames
);

//==================================================================================================
#pragma mark -
#pragma mark Priority Integration
//==================================================================================================

// AFL priority levels for interrupt operations
typedef enum {
    AFL_PRIORITY_BACKGROUND = 64,    // Background ambient
    AFL_PRIORITY_NORMAL = 128,       // Normal speech
    AFL_PRIORITY_URGENT = 192,       // Urgent notifications
    AFL_PRIORITY_CRITICAL = 255      // System critical
} AFLPriority;

// Create interrupt context for AFL stream
static inline InterruptContext afl_create_interrupt_context(
    const AFLBridgeState* state,
    AFLPriority priority
) {
    InterruptContext ctx;
    ctx.priority = (Priority)priority;
    ctx.color = afl_to_stream_color(state);
    ctx.duck_amount = 1.0f - (state->speech.stress * 0.5f);  // High stress = more ducking
    return ctx;
}

#endif /* AFLBridge_h */
