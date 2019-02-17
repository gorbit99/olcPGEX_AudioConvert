# olcPGEX_AudioConvert
Audio converter for the OLC PGE

# Usage:
In your main file:
```cpp
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"
#define OLC_PGEX_AUDIOCONVERT
#include "olcPGEX_AudioConverter.h"
```

When loading samples:
```cpp
int sample = olc::AudioConvert::LoadAudioSample("someWavFile.wav");
```
Function protoype:
```cpp
int LoadAudioSample(std::string sWavFile, olc::ResourcePack *pack = nullptr);
```

The sound extension is required for this PGEX. <br>
Supported encodings: `PCM, IEEE floating point, A-law, μ-law` at any sampling rate.
