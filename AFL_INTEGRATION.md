# AFL → WhiteHole Integration Pattern

**PLUS-ERGODIC Stream Documentation**  
SplitMixTernary SEED = 1069

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                    AFL → WhiteHole Pipeline                         │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐     ┌──────────────┐     ┌───────────────────┐    │
│  │ AFL Output  │────▶│  AFLBridge   │────▶│ WhiteHole Color   │    │
│  │ JSON Files  │     │  (MINUS -1)  │     │ Routing (ERGODIC) │    │
│  └─────────────┘     └──────────────┘     └───────────────────┘    │
│       │                    │                       │                │
│       ▼                    ▼                       ▼                │
│  ┌─────────────┐     ┌──────────────┐     ┌───────────────────┐    │
│  │ afl.json    │     │ StreamColor  │     │ MaybeAudioBuffer  │    │
│  │ visual.json │     │ derivation   │     │ color_validated   │    │
│  └─────────────┘     └──────────────┘     │ _mix()            │    │
│                                           └───────────────────┘    │
│                                                    │                │
│                            ┌───────────────────────┘                │
│                            ▼                                        │
│                   ┌───────────────────┐                            │
│                   │ WhiteHole Virtual │                            │
│                   │ Audio Device      │                            │
│                   │ (PLUS +1)         │                            │
│                   └───────────────────┘                            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

## Balanced Ternary Color Tags

### Seed 1069 Encoding
```
1069 decimal = [+1, -1, -1, +1, +1, +1, +1] balanced ternary (7 trits)
             = 3^6 + 3^5 + 3^4 + 3^3 - 3^2 - 3^1 + 3^0
             = 729 + 243 + 81 + 27 - 9 - 3 + 1
             = 1069 ✓
```

### AFL Parameter → Trit Mapping

| Trit Position | AFL Parameter | -1 (TRIT_NEG) | 0 (TRIT_ZERO) | +1 (TRIT_POS) |
|---------------|---------------|---------------|---------------|---------------|
| 0 | pitch | < 300 Hz | 300-700 Hz | > 700 Hz |
| 1 | rate | < 0.7x | 0.7-1.3x | > 1.3x |
| 2 | volume | < 0.3 | 0.3-0.7 | > 0.7 |
| 3 | richness | < 0.5 | 0.5-1.5 | > 1.5 |
| 4 | stress | < 0.3 | 0.3-0.7 | > 0.7 |
| 5 | brightness | < 0.3 | 0.3-0.7 | > 0.7 |
| 6 | saturation | < 0.3 | 0.3-0.7 | > 0.7 |

### Compatibility via Hamming Distance

Two StreamColors are **compatible** iff their Hamming distance ≤ 2.

```c
bool colors_compatible(const StreamColor* a, const StreamColor* b) {
    return hamming_distance(&a->tag, &b->tag) <= 2;
}
```

## Maybe Monad Semantics

Audio mixing returns `Nothing` on incompatibility:

```c
MaybeAudioBuffer color_validated_mix(
    const StreamColor* color_a, const MaybeAudioBuffer* stream_a,
    const StreamColor* color_b, const MaybeAudioBuffer* stream_b,
    float* output_buffer, size_t max_frames
);
```

- **Nothing**: Colors incompatible OR input buffers invalid
- **Just(buffer, frames, channels, color)**: Successful mix with combined color hash

## Priority / Interrupt Operations

AFL streams carry priority levels for interrupt-based ducking:

```c
typedef enum {
    AFL_PRIORITY_BACKGROUND = 64,    // Ambient sounds
    AFL_PRIORITY_NORMAL = 128,       // Normal speech
    AFL_PRIORITY_URGENT = 192,       // Urgent notifications
    AFL_PRIORITY_CRITICAL = 255      // System critical
} AFLPriority;
```

Higher priority streams can interrupt/duck lower priority via `InterruptContext`.

## File Structure

```
WhiteHole/
├── WhiteHole.c           # Core audio driver
├── WhiteHoleColor.h      # Color-tagged routing types
├── WhiteHoleColor.c      # Color-tagged routing impl
├── AFLBridge.h           # AFL → WhiteHole bridge types (PLUS-MINUS)
├── AFLBridge.c           # AFL → WhiteHole bridge impl (PLUS-MINUS)
├── AFLEmitter.h          # WhiteHole → AFL emission (PLUS-PLUS)
├── AFLEmitter.c          # WhiteHole → AFL emission (PLUS-PLUS)
└── AFL_INTEGRATION.md    # This file (PLUS-ERGODIC)

afl_output/
├── afl.json              # AFL speech parameters
├── visual.json           # Visual sync parameters
├── giorgio.json          # Giorgio synthesis params
└── weights.json          # Model weights
```

## Usage Example

```c
#include "AFLBridge.h"

// Initialize bridge from AFL output
AFLBridgeState bridge;
afl_bridge_init_from_json(&bridge, 
    "/path/to/afl_output/afl.json",
    "/path/to/afl_output/visual.json");

// Synthesize audio buffer
float buffer[1024 * 2];  // 1024 frames, stereo
MaybeAudioBuffer audio = afl_synthesize_buffer(
    &bridge, buffer, 1024, 2, 48000.0f);

if (audio.is_just) {
    // Route through WhiteHole with color validation
    StreamColor destination = create_stream_color("output-stream", &kSeed1069);
    
    float output[1024 * 2];
    MaybeAudioBuffer routed = afl_route_through_whitehole(
        &bridge, &audio, &destination, output, 1024);
    
    if (routed.is_just) {
        // Audio successfully routed with compatible colors
        printf("Routed with color hash: 0x%08X\n", routed.color_hash);
    }
}
```

## Trifurcation Pattern

This integration follows **SplitMixTernary** discipline:

| Stream | Role | File |
|--------|------|------|
| **PLUS-MINUS (-1)** | Bridge/Contraction | AFLBridge.h/c |
| **PLUS-ERGODIC (0)** | Documentation/Synthesis | AFL_INTEGRATION.md |
| **PLUS-PLUS (+1)** | Emission/Expansion | AFLEmitter.h/c |

**Conservation Law**: Search + Synthesis + Creation sum to coherent result (GF(3)).
