/*
    File: WhiteHoleColor.c

    Color-Tagged Stream Routing Implementation for WhiteHole
    bmorphism fork - Seed 1069 balanced ternary

    Copyright (C) 2025 bmorphism
    Based on WhiteHole by Existential Audio Inc.
*/

#include "WhiteHoleColor.h"
#include <string.h>
#include <Accelerate/Accelerate.h>

//==================================================================================================
#pragma mark -
#pragma mark Stream Mixing Implementation
//==================================================================================================

MaybeAudioBuffer color_validated_mix(
    const StreamColor* color_a, const MaybeAudioBuffer* stream_a,
    const StreamColor* color_b, const MaybeAudioBuffer* stream_b,
    float* output_buffer, size_t max_frames
) {
    // Validate inputs
    if (!color_a || !color_b || !stream_a || !stream_b || !output_buffer) {
        return Nothing();
    }

    // Check color compatibility using balanced ternary Hamming distance
    if (!colors_compatible(color_a, color_b)) {
        // Colors incompatible - return Nothing (monadic failure)
        return Nothing();
    }

    // Check both streams have data
    if (!stream_a->is_just || !stream_b->is_just) {
        return Nothing();
    }

    // Ensure matching channel counts
    if (stream_a->channel_count != stream_b->channel_count) {
        return Nothing();
    }

    // Calculate output size
    size_t frames = stream_a->frame_count < stream_b->frame_count
                  ? stream_a->frame_count : stream_b->frame_count;
    if (frames > max_frames) {
        frames = max_frames;
    }

    size_t sample_count = frames * stream_a->channel_count;

    // Use Accelerate framework for SIMD-optimized mixing
    // vDSP_vadd: vector add (simple mix)
    vDSP_vadd(stream_a->buffer, 1, stream_b->buffer, 1, output_buffer, 1, sample_count);

    // Scale by 0.5 to prevent clipping
    float scale = 0.5f;
    vDSP_vsmul(output_buffer, 1, &scale, output_buffer, 1, sample_count);

    // Compute mixed color hash
    ColorHash mixed_color = mix_colors(color_a, color_b);

    return Just(output_buffer, frames, stream_a->channel_count, mixed_color);
}

//==================================================================================================
#pragma mark -
#pragma mark Bloom Filter Operations
//==================================================================================================

void register_stream_color(BloomFilter* filter, const StreamColor* color) {
    if (!filter || !color) return;
    bloom_add(filter, color->hash);
}

bool check_color_compatibility(const BloomFilter* filter, ColorHash color) {
    if (!filter) return false;
    return bloom_maybe_contains(filter, color);
}

//==================================================================================================
#pragma mark -
#pragma mark InterruptOperad Default Implementations
//==================================================================================================

// Default interrupt: higher priority completely replaces lower
static MaybeAudioBuffer default_interrupt(
    const InterruptContext* ctx,
    const MaybeAudioBuffer* high_priority,
    const MaybeAudioBuffer* low_priority
) {
    (void)ctx;
    (void)low_priority;

    if (!high_priority || !high_priority->is_just) {
        // Fall back to low priority if high is Nothing
        if (low_priority && low_priority->is_just) {
            return *low_priority;
        }
        return Nothing();
    }

    return *high_priority;
}

// Default duck: attenuate secondary by amount
static MaybeAudioBuffer default_duck(
    float amount,
    const MaybeAudioBuffer* primary,
    const MaybeAudioBuffer* secondary
) {
    if (!primary || !primary->is_just) {
        return Nothing();
    }

    if (!secondary || !secondary->is_just || amount >= 1.0f) {
        return *primary;
    }

    // In a full implementation, we'd allocate and mix here
    // For now, return primary with ducking metadata noted
    return *primary;
}

// Default gate: pass through if predicate is true
static MaybeAudioBuffer default_gate(
    bool (*predicate)(const MaybeAudioBuffer*),
    const MaybeAudioBuffer* stream
) {
    if (!stream || !predicate) {
        return Nothing();
    }

    if (predicate(stream)) {
        return *stream;
    }

    return Nothing();
}

//==================================================================================================
#pragma mark -
#pragma mark Balanced Ternary Utilities
//==================================================================================================

// Convert integer to balanced ternary StreamTag
StreamTag int_to_stream_tag(int32_t value) {
    StreamTag tag;
    int32_t remaining = value;

    for (int i = 0; i < STREAM_TAG_TRITS; i++) {
        int32_t rem = remaining % 3;
        if (rem == 0) {
            tag.trits[i] = TRIT_ZERO;
        } else if (rem == 1) {
            tag.trits[i] = TRIT_POS;
            remaining -= 1;
        } else {  // rem == 2, which is -1 in balanced ternary
            tag.trits[i] = TRIT_NEG;
            remaining += 1;
        }
        remaining /= 3;
    }

    return tag;
}

// Convert StreamTag back to integer
int32_t stream_tag_to_int(const StreamTag* tag) {
    int32_t value = 0;
    int32_t power = 1;

    for (int i = 0; i < STREAM_TAG_TRITS; i++) {
        value += tag->trits[i] * power;
        power *= 3;
    }

    return value;
}

// Verify seed 1069 encoding is correct
// 1069 in balanced ternary: 3^6 = 729, 3^5 = 243, 3^4 = 81, 3^3 = 27, 3^2 = 9, 3^1 = 3, 3^0 = 1
// 1069 = 729 + 243 + 81 + 27 - 9 - 3 + 1 = 1069 CHECK
// [+1, -1, -1, +1, +1, +1, +1] reading right-to-left for powers
bool verify_seed_1069(void) {
    StreamTag computed = int_to_stream_tag(1069);
    for (int i = 0; i < STREAM_TAG_TRITS; i++) {
        if (computed.trits[i] != kSeed1069.trits[i]) {
            return false;
        }
    }
    return true;
}

//==================================================================================================
#pragma mark -
#pragma mark Color Hash Generation
//==================================================================================================

// Simple FNV-1a hash for color generation from name
ColorHash compute_color_hash(const char* name) {
    if (!name) return 0;

    uint32_t hash = 2166136261u;  // FNV offset basis
    const uint32_t prime = 16777619u;  // FNV prime

    while (*name) {
        hash ^= (uint8_t)*name++;
        hash *= prime;
    }

    return hash;
}

// Create a StreamColor from name and optional explicit tag
StreamColor create_stream_color(const char* name, const StreamTag* tag) {
    StreamColor color;

    color.hash = compute_color_hash(name);

    if (tag) {
        color.tag = *tag;
    } else {
        // Derive tag from hash
        color.tag = int_to_stream_tag((int32_t)(color.hash % 2187));  // 3^7 = 2187
    }

    // Copy name (truncate if necessary)
    if (name) {
        strncpy(color.name, name, sizeof(color.name) - 1);
        color.name[sizeof(color.name) - 1] = '\0';
    } else {
        color.name[0] = '\0';
    }

    return color;
}
