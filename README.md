# Ember: 3-band distortion (VST3 / AU / Standalone)

An original multiband distortion built with JUCE. It is in the same category
as plugins like Output's Thermal, but the DSP, UI and names are all new.

## Signal flow
Input gain -> 2x oversampling -> Linkwitz-Riley split into Low / Mid / High
(two crossovers, phase-compensated) -> per band: Drive -> Shape (Soft, Hard,
Fold, Crush) -> Level / Mute -> sum -> downsample -> dry/wet mix -> output gain.

## Build
Requires CMake 3.22+ and a C++17 compiler (Xcode, Visual Studio 2022, or gcc/clang).
The first configure downloads JUCE (needs internet).

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

The VST3 is copied to your system plugin folder after the build.
Artefacts are also under build/Ember_artefacts/. Run the Standalone build
first to check it makes sound, then rescan plugins in your DAW.

## Notes
- VST3 is targeted (VST2 is discontinued by Steinberg).
- JUCE is AGPLv3 or commercial; check the licence before you distribute.
- Change COMPANY_NAME and the plugin codes in CMakeLists.txt.
- This code has not been compiled or run by its author; expect to fix small
  compile errors on the first build.
