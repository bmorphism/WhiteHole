/*
    File: WhiteHoleColor.h

    Color-Tagged Stream Routing Extensions for WhiteHole
    bmorphism fork - Seed 1069 balanced ternary

    Copyright (C) 2025 bmorphism
    Based on WhiteHole by Existential Audio Inc.
*/

#ifndef WhiteHoleColor_h
#define WhiteHoleColor_h

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//==================================================================================================
#pragma mark -
#pragma mark Balanced Ternary Types (Seed 1069)
//==================================================================================================

// Seed 1069 = [+1, -1, -1, +1, +1, +1, +1] in balanced ternary
// 7 trits per stream tag
typedef int8_t Trit;  // -1, 0, +1

#define TRIT_NEG  ((Trit)-1)
#define TRIT_ZERO ((Trit)0)
#define TRIT_POS  ((Trit)1)

#define STREAM_TAG_TRITS 7

typedef struct {
    Trit trits[STREAM_TAG_TRITS];
} StreamTag;

// Seed 1069 constant
static const StreamTag kSeed1069 = {
    .trits = { TRIT_POS, TRIT_NEG, TRIT_NEG, TRIT_POS, TRIT_POS, TRIT_POS, TRIT_POS }
};

//==================================================================================================
#pragma mark -
#pragma mark Color Hash Types
//==================================================================================================

// SHA3-derived color hash (truncated to 32 bits for O(1) ops)
typedef uint32_t ColorHash;

// Bloom filter for O(1) membership test
#define BLOOM_FILTER_BITS 256
#define BLOOM_FILTER_WORDS (BLOOM_FILTER_BITS / 64)

typedef struct {
    uint64_t bits[BLOOM_FILTER_WORDS];
} BloomFilter;

//==================================================================================================
#pragma mark -
#pragma mark Maybe Monad for Audio Buffers
//==================================================================================================

typedef struct {
    bool is_just;           // Nothing = incompatible, Just = mixed result
    float* buffer;          // Audio data (NULL if Nothing)
    size_t frame_count;     // Number of frames
    uint32_t channel_count; // Channels per frame
    ColorHash color_hash;   // Combined color on success (0 if Nothing)
} MaybeAudioBuffer;

// Maybe constructors
static inline MaybeAudioBuffer Nothing(void) {
    return (MaybeAudioBuffer){ .is_just = false, .buffer = NULL, .frame_count = 0, .channel_count = 0, .color_hash = 0 };
}

static inline MaybeAudioBuffer Just(float* buf, size_t frames, uint32_t channels, ColorHash color) {
    return (MaybeAudioBuffer){ .is_just = true, .buffer = buf, .frame_count = frames, .channel_count = channels, .color_hash = color };
}

//==================================================================================================
#pragma mark -
#pragma mark Stream Color Operations
//==================================================================================================

typedef struct {
    ColorHash hash;
    StreamTag tag;
    char name[64];
} StreamColor;

// Color compatibility check (Hamming distance <= 2 is compatible)
static inline uint32_t hamming_distance(const StreamTag* a, const StreamTag* b) {
    uint32_t dist = 0;
    for (int i = 0; i < STREAM_TAG_TRITS; i++) {
        if (a->trits[i] != b->trits[i]) dist++;
    }
    return dist;
}

static inline bool colors_compatible(const StreamColor* a, const StreamColor* b) {
    return hamming_distance(&a->tag, &b->tag) <= 2;
}

// Color mixing: combine two compatible colors
// Returns 0 (invalid) if incompatible
static inline ColorHash mix_colors(const StreamColor* a, const StreamColor* b) {
    if (!colors_compatible(a, b)) return 0;
    // XOR combination for mixed color identity
    return a->hash ^ b->hash;
}

//==================================================================================================
#pragma mark -
#pragma mark Bloom Filter Operations
//==================================================================================================

static inline void bloom_init(BloomFilter* filter) {
    for (int i = 0; i < BLOOM_FILTER_WORDS; i++) {
        filter->bits[i] = 0;
    }
}

// MurmurHash3-style bit mixing for hash distribution
static inline uint32_t bloom_hash1(ColorHash h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    return h % BLOOM_FILTER_BITS;
}

static inline uint32_t bloom_hash2(ColorHash h) {
    h ^= h >> 16;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h % BLOOM_FILTER_BITS;
}

static inline void bloom_add(BloomFilter* filter, ColorHash color) {
    uint32_t b1 = bloom_hash1(color);
    uint32_t b2 = bloom_hash2(color);
    filter->bits[b1 / 64] |= (1ULL << (b1 % 64));
    filter->bits[b2 / 64] |= (1ULL << (b2 % 64));
}

// O(1) membership test - may have false positives, never false negatives
static inline bool bloom_maybe_contains(const BloomFilter* filter, ColorHash color) {
    uint32_t b1 = bloom_hash1(color);
    uint32_t b2 = bloom_hash2(color);
    return (filter->bits[b1 / 64] & (1ULL << (b1 % 64))) &&
           (filter->bits[b2 / 64] & (1ULL << (b2 % 64)));
}

//==================================================================================================
#pragma mark -
#pragma mark InterruptOperad Operations
//==================================================================================================

typedef uint8_t Priority;

#define PRIORITY_MIN     0
#define PRIORITY_DEFAULT 128
#define PRIORITY_MAX     255

typedef struct {
    Priority priority;
    StreamColor color;
    float duck_amount;  // 0.0 = full duck, 1.0 = no duck
} InterruptContext;

// Interrupt operation: higher priority stream takes precedence
// interrupt : Priority -> Stream -> Stream -> Stream
typedef MaybeAudioBuffer (*InterruptOp)(
    const InterruptContext* ctx,
    const MaybeAudioBuffer* high_priority,
    const MaybeAudioBuffer* low_priority
);

// Duck operation: attenuate low priority by amount
// duck : Float -> Stream -> Stream -> Stream
typedef MaybeAudioBuffer (*DuckOp)(
    float amount,  // 0.0 = silence low, 1.0 = full mix
    const MaybeAudioBuffer* primary,
    const MaybeAudioBuffer* secondary
);

// Gate operation: predicate-based stream filtering
// gate : Predicate -> Stream -> Maybe Stream
typedef MaybeAudioBuffer (*GateOp)(
    bool (*predicate)(const MaybeAudioBuffer*),
    const MaybeAudioBuffer* stream
);

//==================================================================================================
#pragma mark -
#pragma mark Stream Mixing with Color Validation
//==================================================================================================

// Mix two audio streams only if colors are compatible
// Returns Nothing if incompatible, Just(mixed) otherwise
MaybeAudioBuffer color_validated_mix(
    const StreamColor* color_a, const MaybeAudioBuffer* stream_a,
    const StreamColor* color_b, const MaybeAudioBuffer* stream_b,
    float* output_buffer, size_t max_frames
);

// Register a color in the compatibility filter
void register_stream_color(BloomFilter* filter, const StreamColor* color);

// Check if a color might be compatible with registered colors
bool check_color_compatibility(const BloomFilter* filter, ColorHash color);

#endif /* WhiteHoleColor_h */
