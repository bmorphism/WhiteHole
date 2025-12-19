/*
    File: AFLBridge.c
    
    AFL → WhiteHole Bridge Implementation (PLUS-MINUS stream)
    
    SplitMixTernary SEED = 1069
    Copyright (C) 2025 bmorphism
*/

#include "AFLBridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Accelerate/Accelerate.h>

//==================================================================================================
#pragma mark -
#pragma mark JSON Parsing (Minimal)
//==================================================================================================

static float parse_json_float(const char* json, const char* key, float default_val) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char* pos = strstr(json, search);
    if (!pos) return default_val;
    
    pos = strchr(pos, ':');
    if (!pos) return default_val;
    
    return (float)atof(pos + 1);
}

static bool load_json_file(const char* path, char* buffer, size_t buffer_size) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    
    size_t bytes_read = fread(buffer, 1, buffer_size - 1, f);
    buffer[bytes_read] = '\0';
    fclose(f);
    
    return bytes_read > 0;
}

//==================================================================================================
#pragma mark -
#pragma mark Bridge Initialization
//==================================================================================================

bool afl_bridge_init_from_json(AFLBridgeState* state,
                                const char* afl_json_path,
                                const char* visual_json_path) {
    if (!state) return false;
    
    char json_buffer[4096];
    
    // Parse AFL speech params
    if (afl_json_path && load_json_file(afl_json_path, json_buffer, sizeof(json_buffer))) {
        state->speech.pitch = parse_json_float(json_buffer, "pitch", 600.0f);
        state->speech.rate = parse_json_float(json_buffer, "rate", 1.0f);
        state->speech.volume = parse_json_float(json_buffer, "volume", 0.75f);
        state->speech.richness = parse_json_float(json_buffer, "richness", 1.0f);
        state->speech.stress = parse_json_float(json_buffer, "stress", 0.5f);
    } else {
        // Defaults
        state->speech = (AFLSpeechParams){ 600.0f, 1.0f, 0.75f, 1.0f, 0.5f };
    }
    
    // Parse visual params
    if (visual_json_path && load_json_file(visual_json_path, json_buffer, sizeof(json_buffer))) {
        state->visual.hue = parse_json_float(json_buffer, "hue", 180.0f);
        state->visual.saturation = parse_json_float(json_buffer, "saturation", 0.7f);
        state->visual.brightness = parse_json_float(json_buffer, "brightness", 0.75f);
        state->visual.size = parse_json_float(json_buffer, "size", 1.0f);
        state->visual.speed = parse_json_float(json_buffer, "speed", 1.0f);
        state->visual.complexity = parse_json_float(json_buffer, "complexity", 5.0f);
        state->visual.opacity = parse_json_float(json_buffer, "opacity", 0.9f);
    } else {
        // Defaults
        state->visual = (AFLVisualParams){ 180.0f, 0.7f, 0.75f, 1.0f, 1.0f, 5.0f, 0.9f };
    }
    
    // Derive color from combined state
    StreamColor derived = afl_to_stream_color(state);
    state->derived_color = derived.hash;
    state->derived_tag = derived.tag;
    
    return true;
}

bool afl_bridge_update(AFLBridgeState* state,
                       const char* afl_json_path,
                       const char* visual_json_path) {
    return afl_bridge_init_from_json(state, afl_json_path, visual_json_path);
}

//==================================================================================================
#pragma mark -
#pragma mark Compatibility Check
//==================================================================================================

bool afl_stream_compatible(const AFLBridgeState* afl_state,
                           const StreamColor* whitehole_stream) {
    if (!afl_state || !whitehole_stream) return false;
    
    StreamColor afl_color = afl_to_stream_color(afl_state);
    return colors_compatible(&afl_color, whitehole_stream);
}

//==================================================================================================
#pragma mark -
#pragma mark Audio Synthesis
//==================================================================================================

MaybeAudioBuffer afl_synthesize_buffer(const AFLBridgeState* state,
                                        float* output_buffer,
                                        size_t frame_count,
                                        uint32_t channel_count,
                                        float sample_rate) {
    if (!state || !output_buffer || frame_count == 0) {
        return Nothing();
    }
    
    // Generate a simple tone modulated by AFL parameters
    // In production, this would interface with actual TTS synthesis
    float freq = state->speech.pitch;
    float amplitude = state->speech.volume;
    float phase = 0.0f;
    float phase_increment = (2.0f * M_PI * freq) / sample_rate;
    
    // Apply rate as time-stretch factor to phase
    phase_increment *= state->speech.rate;
    
    // Generate samples
    for (size_t frame = 0; frame < frame_count; frame++) {
        float sample = amplitude * sinf(phase);
        
        // Add harmonics based on richness
        if (state->speech.richness > 1.0f) {
            float harmonic_amp = amplitude * (state->speech.richness - 1.0f) * 0.3f;
            sample += harmonic_amp * sinf(phase * 2.0f);  // 2nd harmonic
            sample += harmonic_amp * 0.5f * sinf(phase * 3.0f);  // 3rd harmonic
        }
        
        // Apply stress as amplitude modulation
        if (state->speech.stress > 0.5f) {
            float mod = 1.0f + (state->speech.stress - 0.5f) * 0.3f * sinf(phase * 0.1f);
            sample *= mod;
        }
        
        // Write to all channels
        for (uint32_t ch = 0; ch < channel_count; ch++) {
            output_buffer[frame * channel_count + ch] = sample;
        }
        
        phase += phase_increment;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
    
    StreamColor color = afl_to_stream_color(state);
    return Just(output_buffer, frame_count, channel_count, color.hash);
}

//==================================================================================================
#pragma mark -
#pragma mark WhiteHole Routing
//==================================================================================================

MaybeAudioBuffer afl_route_through_whitehole(
    const AFLBridgeState* afl_state,
    const MaybeAudioBuffer* afl_audio,
    const StreamColor* destination_color,
    float* output_buffer,
    size_t max_frames
) {
    if (!afl_state || !afl_audio || !destination_color || !output_buffer) {
        return Nothing();
    }
    
    StreamColor afl_color = afl_to_stream_color(afl_state);
    
    // Use WhiteHole's color-validated mixing
    return color_validated_mix(
        &afl_color, afl_audio,
        destination_color, afl_audio,  // Self-mix with destination validation
        output_buffer, max_frames
    );
}

//==================================================================================================
#pragma mark -
#pragma mark Seed 1069 Integration
//==================================================================================================

// Check if AFL state is compatible with Seed 1069 stream
bool afl_compatible_with_seed_1069(const AFLBridgeState* state) {
    if (!state) return false;
    
    StreamColor afl_color = afl_to_stream_color(state);
    StreamColor seed_color;
    seed_color.hash = compute_color_hash("seed-1069");
    seed_color.tag = kSeed1069;
    strncpy(seed_color.name, "seed-1069", sizeof(seed_color.name));
    
    return colors_compatible(&afl_color, &seed_color);
}

// Compute Hamming distance from Seed 1069
uint32_t afl_distance_from_seed_1069(const AFLBridgeState* state) {
    if (!state) return UINT32_MAX;
    
    StreamColor afl_color = afl_to_stream_color(state);
    return hamming_distance(&afl_color.tag, &kSeed1069);
}
