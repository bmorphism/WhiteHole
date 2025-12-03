![WhiteHole: Audio Loopback Driver](Images/whitehole-banner-830px.png)

# WhiteHole: Audio Loopback Driver

![Platform: macOS](https://img.shields.io/badge/platform-macOS-lightgrey)
[![Release](https://img.shields.io/github/v/release/ExistentialAudio/WhiteHole)](https://github.com/ExistentialAudio/WhiteHole/releases)
[![License](https://img.shields.io/github/license/ExistentialAudio/WhiteHole)](LICENSE)
[![Twitter](https://img.shields.io/badge/Follow%20on%20Twitter-1da1f2)](https://twitter.com/ExistentialAI)
[![Facebook](https://img.shields.io/badge/Like%20on%20Facebook-4267B2)](https://www.facebook.com/Existential-Audio-103423234434751)

WhiteHole is a modern macOS virtual audio loopback driver that allows applications to pass audio to other applications with zero additional latency.

### [Download Installer](https://existential.audio/whitehole) 

### [Join the Discord Server](https://discord.gg/y8BWfnWRnn)

## Sponsors

### Recall.ai - API for desktop recording

If you’re looking for a desktop recording API, consider checking out [Recall.ai](https://www.recall.ai/product/desktop-recording-sdk?utm_source=github&utm_medium=sponsorship&utm_campaign=existentialaudio-whitehole), an API that records video, audio, and transcripts from Zoom, Google Meet, Microsoft Teams, in-person meetings, and more.

To sponsor this project visit https://github.com/sponsors/ExistentialAudio

## Table of Contents

- [Features](#features)
- [Installation Instructions](#installation-instructions)
- [Uninstallation Instructions](#uninstallation-instructions)
- [User Guides](#user-guides)
- [Developer Guides](#developer-guides)
- [Feature Requests](#feature-requests)
- [FAQ](#faq)
- [Wiki](https://github.com/ExistentialAudio/WhiteHole/wiki)

## Features

- Builds 2, 16, 64, 128, and 256 audio channels versions
- Customizable channel count, latency, hidden devices
- Customizable mirror device to allow for a hidden input or output
- Supports 8kHz, 16kHz, 44.1kHz, 48kHz, 88.2kHz, 96kHz, 176.4kHz, 192kHz, 352.8kHz, 384kHz, 705.6kHz and 768kHz sample rates
- Zero additional driver latency
- Compatible with macOS 10.10 Yosemite and newer
- Builds for Intel and Apple Silicon
- No kernel extensions or modifications to system security necessary

![Audio MIDI Setup](Images/audio-midi-setup.png)

## Installation Instructions

### Option 1: Download Installer

1. [Download the latest installer](https://existential.audio/whitehole)
2. Close all running audio applications
3. Open and install package

### Option 2: Install via Homebrew

- 2ch: `brew install whitehole-2ch`
- 16ch: `brew install whitehole-16ch`
- 64ch: `brew install whitehole-64ch`

## Uninstallation Instructions

### Option 1: Use Uninstaller

- [Download WhiteHole 2ch Uninstaller](https://existential.audio/downloads/WhiteHole2chUninstaller.pkg)
- [Download WhiteHole 16ch Uninstaller](https://existential.audio/downloads/WhiteHole16chUninstaller.pkg)
- [Download WhiteHole 64ch Uninstaller](https://existential.audio/downloads/WhiteHole64chUninstaller.pkg)

### Option 2: Manually Uninstall

1. Delete the WhiteHole driver with the terminal command:
   
    `rm -R /Library/Audio/Plug-Ins/HAL/WhiteHoleXch.driver` 
   
   Be sure to replace `X` with either `2`, `16`, or `64`.
   
   Note that the directory is the root `/Library` not `/Users/user/Library`.

2. Restart CoreAudio with the terminal command:

    `sudo killall -9 coreaudiod`

For more specific details [visit the Wiki](https://github.com/ExistentialAudio/WhiteHole/wiki/Uninstallation).

## User Guides

### Logic Pro X

- [Logic Pro X to FaceTime](https://existential.audio/howto/StreamFromLogicProXtoFaceTime.php)
- [Logic Pro X to Google Meet](https://existential.audio/howto/StreamFromLogicProXtoGoogleMeet.php)
- [Logic Pro X to Skype](https://existential.audio/howto/StreamFromLogicProXtoSkype.php)
- [Logic Pro X to Zoom](https://existential.audio/howto/StreamFromLogicProXtoZoom.php)

### GarageBand

- [GarageBand to FaceTime](https://existential.audio/howto/StreamFromGarageBandToFaceTime.php)
- [GarageBand to Google Meet](https://existential.audio/howto/StreamFromGarageBandToGoogleMeet.php)
- [GarageBand to Skype](https://existential.audio/howto/StreamFromGarageBandToSkype.php)
- [GarageBand to Zoom](https://existential.audio/howto/StreamFromGarageBandToZoom.php)

### Audacity

- [Audacity Setup](https://github.com/ExistentialAudio/WhiteHole/wiki/Audacity)

### Reaper

- [Reaper to Zoom](https://noahliebman.net/2020/12/telephone-colophon-or-how-i-overengineered-my-call-audio/) by Noah Liebman

### Record System Audio

1. [Setup Multi-Output Device](https://github.com/ExistentialAudio/WhiteHole/wiki/Multi-Output-Device)
2. In `Audio MIDI Setup` → `Audio Devices` right-click on the newly created Multi-Output and select "Use This Device For Sound Output"
3. Open digital audio workstation (DAW) such as GarageBand and set input device to "WhiteHole" 
4. Set track to input from channel 1-2
5. Play audio from another application and monitor or record in your DAW

### Route Audio Between Applications

1. Set output driver to "WhiteHole" in sending application
2. Output audio to any channel
3. Open receiving application and set input device to "WhiteHole" 
4. Input audio from the corresponding output channels

## Developer Guides

### A license is required for all non-GPLv3 projects
Please support our hard work and continued development. To request a license [contact Existential Audio](mailto:devinroth@existential.audio).

### Build & Install
After building, to install WhiteHole:

1. Copy or move the built `WhiteHoleXch.driver` bundle to `/Library/Audio/Plug-Ins/HAL`
2. Restart CoreAudio using `sudo killall -9 coreaudiod`

### Customizing WhiteHole

The following pre-compiler constants may be used to easily customize a build of WhiteHole.

```
kDriver_Name
kPlugIn_BundleID
kPlugIn_Icon

kDevice_Name
kDevice_IsHidden
kDevice_HasInput
kDevice_HasOutput

kDevice2_Name
kDevice2_IsHidden
kDevice2_HasInput
kDevice2_HasOutput

kLatency_Frame_Size
kNumber_Of_Channels
kSampleRates
```

They can be specified at build time with `xcodebuild` using `GCC_PREPROCESSOR_DEFINITIONS`. 

Example:

```bash
xcodebuild \
  -project WhiteHole.xcodeproj \
  GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS kSomeConstant=value'
```

Be sure to escape any quotation marks when using strings. 

### Renaming WhiteHole

To customize WhiteHole it is required to change the following constants. 
- `kDriver_Name`
- `kPlugIn_BundleID` (note that this must match the target bundleID)
- `kPlugIn_Icon`

These can specified as pre-compiler constants using ```xcodebuild```.

```bash
driverName="WhiteHole"
bundleID="audio.existential.WhiteHole"
icon="WhiteHole.icns"

xcodebuild \
  -project WhiteHole.xcodeproj \
  -configuration Release \
  PRODUCT_BUNDLE_IDENTIFIER=$bundleID \
  GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS
  kDriver_Name=\"'$driverName'\"
  kPlugIn_BundleID=\"'$bundleID'\"
  kPlugIn_Icon=\"'$icon'\"'
```

### Customizing Channels, Latency, and Sample Rates

`kNumber_Of_Channels` is used to set the number of channels. Be careful when specifying high channel counts. Although WhiteHole is designed to be extremely efficient at higher channel counts it's possible that your computer might not be able to keep up. Sample rates play a role as well. Don't use high sample rates with a high number of channels. Some applications don't know how to handle high channel counts. Proceed with caution.

`kLatency_Frame_Size` is how much time in frames that the driver has to process incoming and outgoing audio. It can be used to delay the audio inside of WhiteHole up to a maximum of 65536 frames. This may be helpful if using WhiteHole with a high channel count. 

`kSampleRates` set the sample rate or sample rates of the audio device. If using multiple sample rates separate each with a comma (`,`). For example: `kSampleRates='44100,48000'`.

### Mirror Device

By default WhiteHole has a hidden mirrored audio device. The devices may be customized using the following constants. 

```
// Original Device
kDevice_IsHidden
kDevice_HasInput
kDevice_HasOutput

// Mirrored Device
kDevice2_IsHidden
kDevice2_HasInput
kDevice2_HasOutput
```

When all are set to true a 2nd WhiteHole will show up that works exactly the same. The inputs and outputs are mirrored so the outputs from both devices go to the inputs of both devices.

This is useful if you need a separate device for input and output.

Example

```
// Original Device
kDevice_IsHidden=false
kDevice_HasInput=true
kDevice_HasOutput=false

// Mirrored Device
kDevice2_IsHidden=false
kDevice2_HasInput=false
kDevice2_HasOutput=true
```

In this situation we have two WhiteHole devices. One will have inputs only and the other will have outputs only.

One way to use this in projects is to hide the mirrored device and use it behind the scenes. That way the user will see an input only device while routing audio through to the output behind them scenes. 

Hidden audio devices can be accessed using `kAudioHardwarePropertyTranslateUIDToDevice`.

### Continuous Integration / Continuous Deployment

WhiteHole can be integrated into your CI/CD. Take a look at the [create_installer.sh](https://github.com/ExistentialAudio/WhiteHole/blob/master/Installer/create_installer.sh) shell script to see how the installer is built, signed and notarized.

## Feature Requests

If you are interested in any of the following features please leave a comment in the linked issue. To request a features not listed please create a new issue.

- [Sync Clock with other Audio Devices](https://github.com/ExistentialAudio/WhiteHole/issues/27) in development see v0.3.0
- [Output Whitehole to other Audio Device](https://github.com/ExistentialAudio/WhiteHole/issues/40)
- [Add Support for AU Plug-ins](https://github.com/ExistentialAudio/WhiteHole/issues/18)
- [Inter-channel routing](https://github.com/ExistentialAudio/WhiteHole/issues/13)
- [Record Directly to File](https://github.com/ExistentialAudio/WhiteHole/issues/8)
- [Configuration Options Menu](https://github.com/ExistentialAudio/WhiteHole/issues/7)
- [Support for Additional Bit Depths](https://github.com/ExistentialAudio/WhiteHole/issues/42)

## FAQ

### Why isn't WhiteHole showing up in the Applications folder?

WhiteHole is a virtual audio loopback driver. It only shows up in `Audio MIDI Setup`, `Sound Preferences`, or other audio applications.

### How can I listen to the audio and use WhiteHole at the same time?

See [Setup a Multi-Output Device](https://github.com/ExistentialAudio/WhiteHole/wiki/Multi-Output-Device).

### What bit depth does WhiteHole use, and can I change it?

WhiteHole uses 32-bit float bit depth since macOS Core Audio natively uses 32-bit at the system level. This provides the broadest compatibility and greatest audio headroom.

This format is lossless for up to 24-bit integer. All applications should be able to playback and record audio, and do not require adjusting bit depth at the WhiteHole driver level.

### How can I change the volume of a Multi-Output device?

Unfortunately macOS does not support changing the volume of a Multi-Output device but you can set the volume of individual devices in Audio MIDI Setup. 

### Why is nothing playing through WhiteHole? 

- Check `System Preferences` → `Security & Privacy` → `Privacy` → `Microphone` to make sure your digital audio workstation (DAW) application has microphone access. 

- Check that the volume is all the way up on WhiteHole input and output in ``Audio MIDI Setup``.

- If you are using a multi-output device, due to issues with macOS the Built-in Output must be enabled and listed as the top device in the Multi-Output. [See here for details](https://github.com/ExistentialAudio/WhiteHole/wiki/Multi-Output-Device#4-select-output-devices).

### Why is audio glitching after X minutes when using a multi-output or an aggregate?

- You need to enable drift correction for all devices except the Clock Source also known as Master Device or Primary Device.

### Why is the Installer failing?

- Certain versions of macOS have a known issue where install packages may fail to install when the install package is located in certain folders. If you downloaded the .pkg file to your Downloads folder, try moving it to the Desktop and open the .pkg again (or vice-versa).

### What Apps Don't Work with Multi-Outputs?

Unfortunately multi-outputs can be buggy and some apps won't work with them at all. Here is a list of known ones. If additional incompatible applications are found, please report them by opening an [issue](https://github.com/ExistentialAudio/WhiteHole/issues).

- Apple Podcasts
- Apple Messages
- HDHomeRun

### AirPods with an Aggregate/Multi-Output is not working.

The microphone from AirPods runs at a lower sample rate which means it should not be used as the primary/clock device in an Aggregate or Multi-Output device. The solution is to use your built-in speakers (and just mute them) or WhiteHole 2ch as the primary/clock device. WhiteHole 16ch will not work as the primary since the primary needs to have 2ch. 

Read [this discussion](https://github.com/ExistentialAudio/WhiteHole/issues/146) for more details.

### Can I integrate WhiteHole into my app?

WhiteHole is licensed under GPL-3.0. You can use WhiteHole as long as your app is also licensed as GPL-3.0. For all other applications please [contact Existential Audio directly](mailto:devinroth@existential.audio).

## Roadmap: Colored Stream Routing

### bmorphism Fork Extensions

This fork extends WhiteHole with categorical audio routing based on stream "colors" (types). Streams only mix when their colors are compatible, enabling type-safe audio composition.

#### Phase 1: Color-Tagged Streams

```
┌─────────────────────────────────────────────────────────────────┐
│  Stream Color Algebra                                           │
│                                                                 │
│  Each audio stream carries a "color" tag:                       │
│  • Color = SHA3(source_app || channel_config || sample_rate)    │
│  • O(1) membership test via Bloom filter or Cuckoo filter       │
│                                                                 │
│  Mixing Rule:                                                   │
│  mix : Color × Color → Maybe Color                              │
│  mix(c1, c2) = Just (c1 ⊕ c2)  if compatible(c1, c2)            │
│              = Nothing          otherwise                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Implementation:**
- `kStreamColor` preprocessor constant for static color assignment
- Runtime color negotiation via `kAudioDevicePropertyStreamConfiguration`
- Bloom filter with k=7 hash functions (seed 1069) for O(1) compatibility check

#### Phase 2: Maybe Monad Rejection

```c
// Stream mixing with Maybe semantics
typedef struct {
    bool is_just;           // Nothing = incompatible, Just = mixed result
    float* buffer;          // Audio data (NULL if Nothing)
    uint32_t color_hash;    // Combined color on success
} MaybeAudioBuffer;

MaybeAudioBuffer mix_streams(AudioBuffer* a, AudioBuffer* b) {
    if (!colors_compatible(a->color, b->color)) {
        return (MaybeAudioBuffer){ .is_just = false, .buffer = NULL };
    }
    // Mix and return Just(combined)
    return (MaybeAudioBuffer){
        .is_just = true,
        .buffer = mix_buffers(a, b),
        .color_hash = a->color ^ b->color  // XOR for balanced ternary compat
    };
}
```

**Rejection Modes:**
- `kRejectMode_Silent` - Incompatible streams produce silence
- `kRejectMode_Passthrough` - First stream wins, second rejected
- `kRejectMode_Error` - Return error to application

#### Phase 3: InterruptOperad

The `InterruptOperad` enables compositional interrupt handling for audio streams:

```
┌─────────────────────────────────────────────────────────────────┐
│  InterruptOperad : Stream Composition with Priorities           │
│                                                                 │
│  Operations:                                                    │
│  • interrupt : Priority → Stream → Stream → Stream              │
│    High-priority stream interrupts low-priority                 │
│                                                                 │
│  • duck : Float → Stream → Stream → Stream                      │
│    Reduce volume of background stream when foreground active    │
│                                                                 │
│  • gate : Predicate → Stream → Maybe Stream                     │
│    Pass stream only when predicate satisfied                    │
│                                                                 │
│  Composition Laws (Operad Axioms):                              │
│  • Associativity: (f ∘ g) ∘ h = f ∘ (g ∘ h)                     │
│  • Identity: id ∘ f = f = f ∘ id                                │
│  • Equivariance: σ(f ∘ g) = σ(f) ∘ σ(g)                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Use Cases:**
- Voice detection interrupts music playback (Aqua Voice integration)
- Notification sounds duck background audio
- Conference call audio gates through only when unmuted

#### Phase 4: Balanced Ternary Stream Tagging

Stream colors encoded in balanced ternary (trits: -1, 0, +1):

```
Color Tag: 7 trits (seed 1069 = [+1, -1, -1, +1, +1, +1, +1])

Position meanings:
  [0]: Source type     (-1=system, 0=app, +1=hardware)
  [1]: Channel count   (-1=mono, 0=stereo, +1=multichannel)
  [2]: Sample rate     (-1=low, 0=standard, +1=high)
  [3]: Bit depth       (-1=16bit, 0=24bit, +1=32bit)
  [4]: Latency class   (-1=realtime, 0=normal, +1=buffered)
  [5]: Priority        (-1=background, 0=normal, +1=foreground)
  [6]: Mix permission  (-1=reject, 0=conditional, +1=allow)

Compatibility: Streams mix iff Hamming distance ≤ 2
```

### Information-Theoretic Foundation

WhiteHole as a cognitive time-binding device:

```
┌─────────────────────────────────────────────────────────────────┐
│  Audio Information Flow                                         │
│                                                                 │
│  Physical: Sound waves → microphone → ADC → bits → DAC → waves  │
│  Logical:  Pattern captured, stored, routed, replayed           │
│                                                                 │
│  The "white hole" metaphor:                                     │
│  • Black hole: Information falls IN, entropy increases          │
│  • White hole: Information flows OUT, patterns persist          │
│                                                                 │
│  WhiteHole driver = Information channel that preserves          │
│  patterns across the entropic arrow of time                     │
│                                                                 │
│  Entropic Gravity Connection (Verlinde 2010, Carney 2025):      │
│  • Gravity emerges from entropy maximization                    │
│  • Audio routing = information geodesic through device space    │
│  • Zero latency = minimal action principle                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Build with Color Extensions

```bash
xcodebuild \
  -project WhiteHole.xcodeproj \
  -configuration Release \
  CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO \
  GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS
    kEnableColorRouting=1
    kBloomFilterSeed=1069
    kDefaultRejectMode=kRejectMode_Silent'
```

### Version Control

This fork uses both git and [pijul](https://pijul.org/) for parallel versioning:
- **git**: Linear history, GitHub integration
- **pijul**: Patch-based, commutative merges, category-theoretic foundations

```bash
# View pijul patches
nix-shell -p pijul --run "pijul log"

# Record changes
nix-shell -p pijul --run "pijul record -m 'description'"
```

## Links and Resources
### [MultiSoundChanger](https://github.com/rlxone/MultiSoundChanger)
A small tool for changing sound volume even for aggregate devices cause native sound volume controller can't change volume of aggregate devices
### [BackgroundMusic](https://github.com/kyleneideck/BackgroundMusic)
Background Music, a macOS audio utility: automatically pause your music, set individual apps' volumes and record system audio.

