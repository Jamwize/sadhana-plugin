#!/bin/bash
# Sadhana — build a universal (Apple Silicon + Intel) AU/VST3 and a .pkg installer.
# Runs on GitHub's macOS machines, or on any Mac with Xcode Command Line Tools:  bash build_mac.sh
set -e
cd "$(dirname "$0")"
VERSION=1.0.0

command -v cmake >/dev/null 2>&1 || { command -v brew >/dev/null && brew install cmake || { python3 -m pip install --user --quiet cmake; export PATH="$(python3 -m site --user-base)/bin:$PATH"; }; }
[ -d JUCE ] || git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j"$(sysctl -n hw.ncpu)"

ART=build/Sadhana_artefacts/Release
rm -rf dist pkgroot && mkdir -p dist "pkgroot/Library/Audio/Plug-Ins/Components" "pkgroot/Library/Audio/Plug-Ins/VST3"
cp -R "$ART/AU/Sadhana.component" "pkgroot/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/VST3/Sadhana.vst3"     "pkgroot/Library/Audio/Plug-Ins/VST3/"

# ad-hoc signatures (required on Apple Silicon; no paid developer account involved)
codesign --force --deep -s - "pkgroot/Library/Audio/Plug-Ins/Components/Sadhana.component"
codesign --force --deep -s - "pkgroot/Library/Audio/Plug-Ins/VST3/Sadhana.vst3"
lipo -info "pkgroot/Library/Audio/Plug-Ins/Components/Sadhana.component/Contents/MacOS/Sadhana"

chmod +x packaging/scripts/postinstall
pkgbuild --root pkgroot --identifier au.com.samgreengardencare.sadhana --version "$VERSION" \
         --install-location / --scripts packaging/scripts "dist/Sadhana-component.pkg"
productbuild --distribution packaging/distribution.xml --resources packaging --package-path dist \
             "dist/Sadhana Installer.pkg"
rm dist/Sadhana-component.pkg

# also a plain zip of the plug-ins for manual installs
( cd pkgroot/Library/Audio/Plug-Ins && zip -qry "../../../../dist/Sadhana-plugins.zip" Components/Sadhana.component VST3/Sadhana.vst3 )
ls -la dist
