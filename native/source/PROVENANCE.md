# Source provenance

Derived from https://github.com/microsoft/Windows-Camera/tree/626f8b19c5f367602f2e89c6b314573d3776c9df/Samples/VirtualCamera/VirtualCameraMediaSource

Files retain Microsoft copyright notices and `LICENSE.microsoft`. Only the SimpleMediaSource lifecycle is retained. Physical-camera HW/Augmented wrappers and association requests are excluded from activation and compilation. Kagami uses a private CLSID, fixed 720p/30 types, a restricted per-user frame pipe, black waiting/disconnect output, and explicit synthetic output through its own desktop. Sample RGB coloring no longer modifies pixels.

Artwork is the unmodified approved Kagami RGBA PNG from celestial-sora/kagami commit 372b911e700ae454e81a1ba38d27a2bf9c43557a. The ICO pads this source to a square and resizes with alpha preserved.
