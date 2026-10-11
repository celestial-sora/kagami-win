#!/usr/bin/env bash
# Run from the UCRT64 MSYS2 environment installed with winget/MSYS2 or CI setup.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
sha=ec203121355220dd90278a369eeabb0b8ba4f14a
source_dir="$root/.local/airplay-source-$sha"
mkdir -p "$root/.local" "$root/dist/airplay/bin" "$root/dist/airplay/lib/gstreamer-1.0" "$root/dist/airplay/libexec/gstreamer-1.0" "$root/dist/airplay/share"
if [[ ! -e "$source_dir" ]]; then
    git init -q "$source_dir"
    git -C "$source_dir" remote add origin https://github.com/FDH2/UxPlay.git
    git -C "$source_dir" fetch --depth=1 origin "$sha"
    git -C "$source_dir" checkout -q --detach FETCH_HEAD
    python "$root/tools/patch-uxplay.py" "$source_dir"
    printf '%s\n' "$sha" > "$source_dir/KAGAMI-PATCHED"
fi
[[ $(cat "$source_dir/KAGAMI-PATCHED") == "$sha" ]] || exit 2
[[ $(git -C "$source_dir" rev-parse HEAD) == "$sha" ]] || exit 2
cmake -S "$source_dir" -B "$source_dir/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNO_X11_DEPS=ON
cmake --build "$source_dir/build" --parallel 2
cp "$source_dir/build/uxplay.exe" "$root/dist/airplay/bin/kagami-uxplay.exe"
cp /ucrt64/bin/gst-inspect-1.0.exe "$root/dist/airplay/bin/"
# Include only the renderer/decoder/audio plugins used by this receiver.
plugins=(coreelements playback app typefindfunctions videoconvertscale videoparsersbad libav autodetect d3d11 wasapi audioconvert audioresample volume)
for plugin in "${plugins[@]}"; do
    file="/ucrt64/lib/gstreamer-1.0/libgst$plugin.dll"
    [[ -f "$file" ]] || { printf 'Missing GStreamer plugin: %s\n' "$plugin" >&2; exit 2; }
    cp "$file" "$root/dist/airplay/lib/gstreamer-1.0/"
done
cp /ucrt64/libexec/gstreamer-1.0/gst-plugin-scanner.exe "$root/dist/airplay/libexec/gstreamer-1.0/"
# Resolve recursively from actual PE imports rather than copying unrelated runtimes.
python "$root/tools/bundle-airplay.py" "$root/dist/airplay" /ucrt64/bin
cp -R /ucrt64/share/licenses "$root/dist/airplay/share/"
cp "$source_dir/LICENSE" "$root/dist/airplay/LICENSE-UxPlay.txt"
printf '%s\n' "$sha" > "$root/dist/airplay/UXPLAY-COMMIT"
pacman -Q > "$root/dist/airplay/BUILD-PACKAGES.txt"
# Ship the modified backend's corresponding source in every development artifact.
tar --exclude=.git --exclude=build -czf "$root/dist/airplay/UxPlay-corresponding-source.tar.gz" -C "$source_dir" .
"$root/dist/airplay/bin/kagami-uxplay.exe" -rc NUL -h > "$root/build/airplay-help.txt"
# Probe the bundled runtime with system plugin discovery disabled. This validates
# relocation and codec/sink availability, without listening or claiming phone support.
export GST_PLUGIN_SYSTEM_PATH_1_0=
export GST_PLUGIN_PATH_1_0="$(cygpath -w "$root/dist/airplay/lib/gstreamer-1.0")"
export GST_PLUGIN_SCANNER_1_0="$(cygpath -w "$root/dist/airplay/libexec/gstreamer-1.0/gst-plugin-scanner.exe")"
export GST_REGISTRY_1_0="$(cygpath -w "$root/build/gst-registry.bin")"
for element in appsrc h264parse avdec_h264 decodebin videoconvert videoscale d3d11videosink wasapisink audioconvert audioresample volume; do
    "$root/dist/airplay/bin/gst-inspect-1.0.exe" "$element" > "$root/build/airplay-element-$element.txt"
done
printf 'Pinned Windows AirPlay backend built; phone/audio acceptance still requires devices.\n'
