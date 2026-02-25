/*
    File: WhiteHoleAudioMonad.h

    Audio Monad: Reader(StreamColor) + Writer(EffectLog) + Maybe
    Composable DSP pipeline with color-validated bind.

    The Audio Monad encapsulates:
      - A possibly-failed audio buffer (Maybe)
      - Stream color context propagated through bind (Reader)
      - An effect log accumulated across transformations (Writer)

    Monad laws are verified in WhiteHoleColorRouting.dfy.

    SplitMixTernary SEED = 1069
    Copyright (C) 2025 bmorphism
*/

#ifndef WhiteHoleAudioMonad_h
#define WhiteHoleAudioMonad_h

#include "WhiteHoleColor.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//==================================================================================================
#pragma mark -
#pragma mark Effect Log (Writer component)
//==================================================================================================

#define AUDIO_MONAD_MAX_EFFECTS 16
#define EFFECT_NAME_LEN 32

typedef enum {
    EFFECT_MIX,         // Two streams mixed
    EFFECT_GAIN,        // Gain/attenuation applied
    EFFECT_GATE,        // Predicate-gated pass
    EFFECT_DUCK,        // Priority ducking
    EFFECT_INTERRUPT,   // Priority interrupt
    EFFECT_MAP,         // Sample-wise transform
    EFFECT_FILTER,      // Frequency-domain filter
    EFFECT_DELAY,       // Temporal offset
    EFFECT_RESAMPLE,    // Sample rate conversion
    EFFECT_BIND         // Monadic bind (color-checked)
} EffectKind;

typedef struct {
    EffectKind kind;
    char name[EFFECT_NAME_LEN];
    ColorHash input_color;
    ColorHash output_color;
    float parameter;         // Effect-specific scalar (gain dB, delay ms, etc.)
} EffectEntry;

typedef struct {
    EffectEntry entries[AUDIO_MONAD_MAX_EFFECTS];
    uint32_t count;
} EffectLog;

static inline EffectLog effect_log_empty(void) {
    EffectLog log;
    log.count = 0;
    return log;
}

static inline bool effect_log_append(EffectLog* log, EffectEntry entry) {
    if (log->count >= AUDIO_MONAD_MAX_EFFECTS) return false;
    log->entries[log->count++] = entry;
    return true;
}

// Writer monoidal append: concatenate two logs
static inline EffectLog effect_log_concat(const EffectLog* a, const EffectLog* b) {
    EffectLog result = *a;
    for (uint32_t i = 0; i < b->count && result.count < AUDIO_MONAD_MAX_EFFECTS; i++) {
        result.entries[result.count++] = b->entries[i];
    }
    return result;
}

//==================================================================================================
#pragma mark -
#pragma mark AudioM  —  the Audio Monad type
//==================================================================================================

// AudioM a = StreamColor -> (Maybe (a, EffectLog))
//
// Unpacked into a concrete struct for C:
//   - is_just: false => computation failed (color mismatch, gate reject, etc.)
//   - When is_just:
//       buffer, frame_count, channel_count  = the audio payload
//       color   = propagated/transformed stream color
//       log     = accumulated effect trace
typedef struct {
    // Maybe layer
    bool is_just;

    // Audio payload (valid when is_just)
    float* buffer;
    size_t frame_count;
    uint32_t channel_count;
    float sample_rate;

    // Reader layer — propagated color context
    StreamColor color;

    // Writer layer — accumulated effect log
    EffectLog log;
} AudioM;

//==================================================================================================
#pragma mark -
#pragma mark Monad primitives: return, bind, map, join
//==================================================================================================

// return : a -> AudioM a
// Lift a raw buffer into the monad with an initial color tag.
static inline AudioM audio_return(float* buf, size_t frames, uint32_t channels,
                                   float sr, const StreamColor* color) {
    AudioM m;
    m.is_just = true;
    m.buffer = buf;
    m.frame_count = frames;
    m.channel_count = channels;
    m.sample_rate = sr;
    m.color = *color;
    m.log = effect_log_empty();
    return m;
}

// mzero : AudioM a   (MonadPlus zero / failure)
static inline AudioM audio_nothing(void) {
    AudioM m;
    m.is_just = false;
    m.buffer = NULL;
    m.frame_count = 0;
    m.channel_count = 0;
    m.sample_rate = 0;
    m.color = (StreamColor){0};
    m.log = effect_log_empty();
    return m;
}

// The Kleisli arrow type:  a -> AudioM b
// In C, an audio transform takes an AudioM and returns an AudioM.
typedef AudioM (*AudioKleisli)(const AudioM* input);

// bind : AudioM a -> (a -> AudioM b) -> AudioM b
//
// If m is Nothing, short-circuit.
// Otherwise apply f, merge effect logs (Writer), and propagate color (Reader).
AudioM audio_bind(const AudioM* m, AudioKleisli f);

// map : (float -> float) -> AudioM a -> AudioM a
// Apply a sample-wise function to every sample in the buffer.
// Pure lift — does not check colors, just transforms data.
typedef float (*SampleFn)(float sample);
AudioM audio_map(const AudioM* m, SampleFn f, const char* effect_name);

// join : AudioM (AudioM a) -> AudioM a
// Flatten nested monad (used internally by bind).
// In our representation, join on a Just(Just(x)) = Just(x) with merged logs.
AudioM audio_join(const AudioM* outer, const AudioM* inner);

//==================================================================================================
#pragma mark -
#pragma mark Kleisli combinators (sugar for building DSP chains)
//==================================================================================================

// Kleisli composition: (f >=> g) x  =  f(x) >>= g
// Compose two audio transforms into one.
AudioM audio_kleisli_compose(AudioKleisli f, AudioKleisli g, const AudioM* input);

// andThen : AudioM a -> AudioKleisli -> AudioM a
// Alias for bind, reads left-to-right.
static inline AudioM audio_and_then(const AudioM* m, AudioKleisli f) {
    return audio_bind(m, f);
}

//==================================================================================================
#pragma mark -
#pragma mark Color-validated operations (lift InterruptOperad into monad)
//==================================================================================================

// Monadic mix: combine two AudioM streams.
// Checks color compatibility; returns Nothing if incompatible.
// Merges both effect logs.
AudioM audio_mix(const AudioM* a, const AudioM* b,
                  float* output_buffer, size_t max_frames);

// Monadic gain: scale all samples by a linear factor.
AudioM audio_gain(const AudioM* m, float gain_linear,
                   float* output_buffer);

// Monadic gate: pass through only if predicate holds on the audio content.
typedef bool (*AudioPredicate)(const AudioM*);
AudioM audio_gate(const AudioM* m, AudioPredicate pred);

// Monadic duck: attenuate m by duck_factor when trigger is active (Just).
AudioM audio_duck(const AudioM* m, const AudioM* trigger, float duck_factor,
                   float* output_buffer);

// Monadic interrupt: if high is Just and has higher priority, replace low.
AudioM audio_interrupt(const AudioM* high, const AudioM* low,
                        Priority high_pri, Priority low_pri);

// Monadic delay: offset the buffer by delay_frames (zero-fill prefix).
AudioM audio_delay(const AudioM* m, size_t delay_frames,
                    float* output_buffer, size_t output_capacity);

//==================================================================================================
#pragma mark -
#pragma mark Trifurcated pipeline (MINUS / ERGODIC / PLUS parallel streams)
//==================================================================================================

typedef struct {
    AudioM minus;    // MINUS (-1): Validator/constrainer
    AudioM ergodic;  // ERGODIC (0): Coordinator/synthesizer
    AudioM plus;     // PLUS (+1): Generator/executor
} TrifurcatedAudioM;

// Split a single AudioM into three trifurcated streams.
// Each stream gets a derived color with the trit at position 0 set accordingly.
TrifurcatedAudioM audio_trifurcate(const AudioM* source,
                                     float* buf_minus,
                                     float* buf_ergodic,
                                     float* buf_plus);

// Merge trifurcated streams back into one, validating GF(3) conservation.
// Returns Nothing if trit sum != 0 (mod 3).
AudioM audio_merge_trifurcated(const TrifurcatedAudioM* tri,
                                 float* output_buffer, size_t max_frames);

// Apply a Kleisli arrow independently to each trifurcated stream.
TrifurcatedAudioM audio_trifurcate_map(const TrifurcatedAudioM* tri,
                                         AudioKleisli f_minus,
                                         AudioKleisli f_ergodic,
                                         AudioKleisli f_plus);

//==================================================================================================
#pragma mark -
#pragma mark Monad law verification helpers
//==================================================================================================

// Check left identity:  return a >>= f  ==  f a
bool audio_monad_check_left_identity(float* buf, size_t frames, uint32_t channels,
                                      float sr, const StreamColor* color,
                                      AudioKleisli f);

// Check right identity:  m >>= return  ==  m
bool audio_monad_check_right_identity(const AudioM* m);

// Check associativity:  (m >>= f) >>= g  ==  m >>= (\x -> f x >>= g)
bool audio_monad_check_associativity(const AudioM* m,
                                      AudioKleisli f, AudioKleisli g);

#endif /* WhiteHoleAudioMonad_h */
