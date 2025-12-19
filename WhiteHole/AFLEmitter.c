/*
    File: AFLEmitter.c
    
    WhiteHole → AFL Emission Implementation (PLUS-PLUS stream)
    
    SplitMixTernary SEED = 1069
    Copyright (C) 2025 bmorphism
*/

#include "AFLEmitter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <Accelerate/Accelerate.h>

//==================================================================================================
#pragma mark -
#pragma mark Internal State
//==================================================================================================

struct AFLEmitterState {
    AFLEmitConfig config;
    AFLSpeechParams last_speech;
    AFLVisualParams last_visual;
    ColorHash last_color;
    uint64_t last_emit_time_ns;
    bool initialized;
};

//==================================================================================================
#pragma mark -
#pragma mark Timing Utilities
//==================================================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

//==================================================================================================
#pragma mark -
#pragma mark Audio Analysis
//==================================================================================================

AudioAnalysisResult analyze_audio_for_afl(
    const float* audio_buffer,
    size_t frame_count,
    uint32_t channel_count,
    float sample_rate
) {
    AudioAnalysisResult result = {0};
    
    if (!audio_buffer || frame_count == 0 || channel_count == 0) {
        return result;
    }
    
    size_t sample_count = frame_count * channel_count;
    
    // Mix to mono for analysis
    float* mono = malloc(frame_count * sizeof(float));
    if (!mono) return result;
    
    for (size_t i = 0; i < frame_count; i++) {
        float sum = 0.0f;
        for (uint32_t ch = 0; ch < channel_count; ch++) {
            sum += audio_buffer[i * channel_count + ch];
        }
        mono[i] = sum / channel_count;
    }
    
    // RMS energy using Accelerate
    float rms = 0.0f;
    vDSP_rmsqv(mono, 1, &rms, frame_count);
    result.rms_energy = rms;
    
    // Zero crossing rate
    size_t zero_crossings = 0;
    for (size_t i = 1; i < frame_count; i++) {
        if ((mono[i-1] >= 0 && mono[i] < 0) || (mono[i-1] < 0 && mono[i] >= 0)) {
            zero_crossings++;
        }
    }
    result.zero_crossing_rate = (float)zero_crossings / frame_count;
    
    // Simple pitch estimation via autocorrelation
    // Look for first peak after initial falloff
    size_t max_lag = (size_t)(sample_rate / 50.0f);  // Min 50 Hz
    size_t min_lag = (size_t)(sample_rate / 1000.0f);  // Max 1000 Hz
    if (max_lag > frame_count / 2) max_lag = frame_count / 2;
    
    float max_corr = 0.0f;
    size_t best_lag = min_lag;
    
    for (size_t lag = min_lag; lag < max_lag; lag++) {
        float corr = 0.0f;
        for (size_t i = 0; i < frame_count - lag; i++) {
            corr += mono[i] * mono[i + lag];
        }
        corr /= (frame_count - lag);
        
        if (corr > max_corr) {
            max_corr = corr;
            best_lag = lag;
        }
    }
    
    result.fundamental_freq = sample_rate / best_lag;
    
    // Spectral centroid estimation (simplified)
    // Use zero crossing rate as proxy for high-frequency content
    result.spectral_centroid = result.zero_crossing_rate * sample_rate / 2.0f;
    
    // Spectral flux (approximate via frame-to-frame energy change)
    size_t hop_size = 256;
    size_t num_hops = frame_count / hop_size;
    float flux_sum = 0.0f;
    float prev_energy = 0.0f;
    
    for (size_t hop = 0; hop < num_hops; hop++) {
        float energy = 0.0f;
        for (size_t i = 0; i < hop_size; i++) {
            size_t idx = hop * hop_size + i;
            if (idx < frame_count) {
                energy += mono[idx] * mono[idx];
            }
        }
        energy = sqrtf(energy / hop_size);
        
        if (hop > 0) {
            flux_sum += fabsf(energy - prev_energy);
        }
        prev_energy = energy;
    }
    
    result.spectral_flux = num_hops > 1 ? flux_sum / (num_hops - 1) : 0.0f;
    
    // Derive AFL parameters
    result.derived_speech = energy_to_speech(
        result.fundamental_freq,
        result.rms_energy,
        result.spectral_centroid,
        result.spectral_flux
    );
    
    // Create temporary StreamColor for visual derivation
    AFLBridgeState temp_bridge;
    temp_bridge.speech = result.derived_speech;
    temp_bridge.visual = (AFLVisualParams){0};  // Will be overwritten
    StreamColor derived = afl_to_stream_color(&temp_bridge);
    
    result.derived_visual = color_to_visual(&derived);
    result.derived_color = derived.hash;
    result.derived_tag = derived.tag;
    
    free(mono);
    return result;
}

//==================================================================================================
#pragma mark -
#pragma mark Emitter Lifecycle
//==================================================================================================

AFLEmitterState* afl_emitter_create(const AFLEmitConfig* config) {
    AFLEmitterState* state = calloc(1, sizeof(AFLEmitterState));
    if (!state) return NULL;
    
    if (config) {
        state->config = *config;
    } else {
        state->config = seed_1069_emit_config();
    }
    
    // Create output directory if needed
    if (state->config.mode == AFL_EMIT_JSON) {
        mkdir(state->config.output_path, 0755);
    }
    
    state->initialized = true;
    state->last_emit_time_ns = 0;
    
    return state;
}

void afl_emitter_destroy(AFLEmitterState* state) {
    if (state) {
        free(state);
    }
}

//==================================================================================================
#pragma mark -
#pragma mark JSON Emission
//==================================================================================================

static bool emit_json(
    const AFLEmitConfig* config,
    const AFLSpeechParams* speech,
    const AFLVisualParams* visual,
    ColorHash color_hash,
    const TrifurcationMeta* meta
) {
    char path[512];
    
    // Emit AFL speech params
    snprintf(path, sizeof(path), "%s/afl.json", config->output_path);
    FILE* f = fopen(path, "w");
    if (!f) return false;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"pitch\" : %.1f,\n", speech->pitch);
    fprintf(f, "  \"rate\" : %.2f,\n", speech->rate);
    fprintf(f, "  \"volume\" : %.2f,\n", speech->volume);
    fprintf(f, "  \"richness\" : %.2f,\n", speech->richness);
    fprintf(f, "  \"stress\" : %.2f\n", speech->stress);
    fprintf(f, "}\n");
    fclose(f);
    
    // Emit visual params
    snprintf(path, sizeof(path), "%s/visual.json", config->output_path);
    f = fopen(path, "w");
    if (!f) return false;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"visual\" : {\n");
    fprintf(f, "    \"hue\" : %.1f,\n", visual->hue);
    fprintf(f, "    \"saturation\" : %.2f,\n", visual->saturation);
    fprintf(f, "    \"brightness\" : %.2f,\n", visual->brightness);
    fprintf(f, "    \"size\" : %.2f,\n", visual->size);
    fprintf(f, "    \"speed\" : %.2f,\n", visual->speed);
    fprintf(f, "    \"complexity\" : %.1f,\n", visual->complexity);
    fprintf(f, "    \"opacity\" : %.2f\n", visual->opacity);
    fprintf(f, "  },\n");
    fprintf(f, "  \"color_hash\" : \"0x%08X\",\n", color_hash);
    fprintf(f, "  \"trifurcation\" : {\n");
    fprintf(f, "    \"stream_sign\" : %d,\n", meta->stream_sign);
    fprintf(f, "    \"stream_role\" : \"%s\",\n", meta->stream_role);
    fprintf(f, "    \"seed\" : %u,\n", meta->seed);
    fprintf(f, "    \"timestamp_ns\" : %llu\n", (unsigned long long)meta->timestamp_ns);
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    fclose(f);
    
    return true;
}

//==================================================================================================
#pragma mark -
#pragma mark Change Detection
//==================================================================================================

static bool params_changed_significantly(
    const AFLSpeechParams* a, const AFLSpeechParams* b,
    const AFLVisualParams* va, const AFLVisualParams* vb,
    float threshold
) {
    // Check speech params
    if (fabsf(a->pitch - b->pitch) / 1000.0f > threshold) return true;
    if (fabsf(a->rate - b->rate) / 2.0f > threshold) return true;
    if (fabsf(a->volume - b->volume) > threshold) return true;
    if (fabsf(a->richness - b->richness) / 2.0f > threshold) return true;
    if (fabsf(a->stress - b->stress) > threshold) return true;
    
    // Check visual params
    if (fabsf(va->hue - vb->hue) / 360.0f > threshold) return true;
    if (fabsf(va->saturation - vb->saturation) > threshold) return true;
    if (fabsf(va->brightness - vb->brightness) > threshold) return true;
    
    return false;
}

//==================================================================================================
#pragma mark -
#pragma mark Process & Emit
//==================================================================================================

bool afl_emitter_process(
    AFLEmitterState* state,
    const MaybeAudioBuffer* audio,
    const StreamColor* stream_color
) {
    if (!state || !state->initialized || !audio || !audio->is_just) {
        return false;
    }
    
    // Analyze audio
    AudioAnalysisResult analysis = analyze_audio_for_afl(
        audio->buffer,
        audio->frame_count,
        audio->channel_count,
        48000.0f  // Assume 48kHz, could be parameterized
    );
    
    // Override visual from stream color if provided
    if (stream_color) {
        analysis.derived_visual = color_to_visual(stream_color);
        analysis.derived_color = stream_color->hash;
        analysis.derived_tag = stream_color->tag;
    }
    
    // Check timing
    uint64_t now = get_time_ns();
    uint64_t interval_ns = (uint64_t)(state->config.emit_interval_ms * 1000000.0f);
    
    if (now - state->last_emit_time_ns < interval_ns) {
        return true;  // Too soon, skip but not an error
    }
    
    // Check change threshold if configured
    if (state->config.emit_on_change && state->last_emit_time_ns > 0) {
        if (!params_changed_significantly(
            &analysis.derived_speech, &state->last_speech,
            &analysis.derived_visual, &state->last_visual,
            state->config.change_threshold
        )) {
            return true;  // No significant change, skip
        }
    }
    
    // Emit based on mode
    bool success = false;
    TrifurcationMeta meta = create_plus_plus_meta();
    meta.timestamp_ns = now;
    
    switch (state->config.mode) {
        case AFL_EMIT_JSON:
            success = emit_json(
                &state->config,
                &analysis.derived_speech,
                &analysis.derived_visual,
                analysis.derived_color,
                &meta
            );
            break;
            
        case AFL_EMIT_UNIX_SOCKET:
        case AFL_EMIT_SHARED_MEM:
        case AFL_EMIT_NATS:
            // TODO: Implement other emission modes
            success = false;
            break;
    }
    
    if (success) {
        state->last_speech = analysis.derived_speech;
        state->last_visual = analysis.derived_visual;
        state->last_color = analysis.derived_color;
        state->last_emit_time_ns = now;
    }
    
    return success;
}

bool afl_emitter_flush(AFLEmitterState* state) {
    if (!state || !state->initialized) {
        return false;
    }
    
    TrifurcationMeta meta = create_plus_plus_meta();
    meta.timestamp_ns = get_time_ns();
    
    if (state->config.mode == AFL_EMIT_JSON) {
        return emit_json(
            &state->config,
            &state->last_speech,
            &state->last_visual,
            state->last_color,
            &meta
        );
    }
    
    return false;
}

bool afl_emitter_get_current(
    const AFLEmitterState* state,
    AFLSpeechParams* out_speech,
    AFLVisualParams* out_visual
) {
    if (!state || !state->initialized) {
        return false;
    }
    
    if (out_speech) *out_speech = state->last_speech;
    if (out_visual) *out_visual = state->last_visual;
    
    return true;
}
