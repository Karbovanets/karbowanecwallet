#!/usr/bin/env bash
set -euo pipefail

artifact_kind="${1:?usage: $0 appimage|deb version}"
krb_version="${2:?usage: $0 appimage|deb version}"
qt_version="${QT_VERSION:-6.8.3}"
build_parallel_level="${BUILD_PARALLEL_LEVEL:-2}"

export DEBIAN_FRONTEND=noninteractive
export APPIMAGE_EXTRACT_AND_RUN=1

apt-get update -y
apt-get install -y --no-install-recommends \
  ca-certificates \
  build-essential \
  cmake \
  dpkg-dev \
  file \
  git \
  libboost-all-dev \
  libdbus-1-3 \
  libfontconfig1 \
  libfuse2 \
  libglib2.0-0 \
  libgl1-mesa-dev \
  libxext6 \
  libxrender1 \
  libxcb-cursor0 \
  libxcb-xinerama0 \
  libxkbcommon-x11-0 \
  patchelf \
  python3 \
  python3-pip \
  wget \
  xvfb \
  xz-utils

python3 -m pip install --no-cache-dir "aqtinstall==3.1.20"
python3 -m aqt install-qt linux desktop "$qt_version" linux_gcc_64 \
  -O /opt/Qt

git config --global --add safe.directory "$(pwd)"

qt_root="/opt/Qt/$qt_version/gcc_64"
export CMAKE_PREFIX_PATH="$qt_root"
export PATH="$qt_root/bin:$PATH"
export LD_LIBRARY_PATH="$qt_root/lib:${LD_LIBRARY_PATH:-}"

for required_file in \
  "$qt_root/lib/cmake/Qt6Svg/Qt6SvgConfig.cmake" \
  "$qt_root/lib/cmake/Qt6LinguistTools/Qt6LinguistToolsConfig.cmake"; do
  if [[ ! -f "$required_file" ]]; then
    echo "Required Qt component is missing: $required_file" >&2
    exit 1
  fi
done

for required_tool in qmake lrelease; do
  if ! command -v "$required_tool" >/dev/null 2>&1; then
    echo "Required Qt tool is missing: $required_tool" >&2
    exit 1
  fi
done

build_folder="build/release"
mkdir -p "$build_folder"
cmake -S . -B "$build_folder" \
  -DARCH=default \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
  -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true \
  -DBOOST_IGNORE_SYSTEM_PATHS_DEFAULT=ON \
  -DBOOST_ROOT=/usr
cmake --build "$build_folder" --parallel "$build_parallel_level"
xvfb-run -a "$build_folder/KarbowanecWallet" --version

cd appimage
chmod +x ./create-appimage.sh
./create-appimage.sh
appimage_name="Karbo-wallet-$krb_version.AppImage"
rm -f "$appimage_name"
mv -f Karbo*.AppImage "$appimage_name"
ls -l "$appimage_name"
xvfb-run -a "./$appimage_name" --version
cd ..

if [[ "$artifact_kind" == "appimage" ]]; then
  exit 0
fi

if [[ "$artifact_kind" != "deb" ]]; then
  echo "Unknown artifact kind: $artifact_kind" >&2
  exit 1
fi

deb_version="${krb_version#v}"
release_name="Karbo-wallet-linux-amd64-$krb_version"
pkgroot="build/debroot"
rm -rf "$pkgroot"
mkdir -p "$pkgroot/opt/karbo-wallet" \
  "$pkgroot/usr/share/applications" \
  "$pkgroot/usr/share/icons/hicolor/256x256/apps" \
  "$pkgroot/DEBIAN"

cp -a appimage/AppDir/. "$pkgroot/opt/karbo-wallet/"
cp appimage/AppDir/usr/share/applications/karbowanecwallet.desktop \
  "$pkgroot/usr/share/applications/karbowanecwallet.desktop"
sed -i 's|^Exec=.*|Exec=/opt/karbo-wallet/AppRun %U|' \
  "$pkgroot/usr/share/applications/karbowanecwallet.desktop"
sed -i 's|^Icon=.*|Icon=karbowanec|' \
  "$pkgroot/usr/share/applications/karbowanecwallet.desktop"
cp appimage/AppDir/usr/share/icons/hicolor/256x256/apps/karbowanec.png \
  "$pkgroot/usr/share/icons/hicolor/256x256/apps/karbowanec.png"

installed_size="$(du -ks "$pkgroot" | cut -f1)"
cat > "$pkgroot/DEBIAN/control" <<EOF
Package: karbowanecwallet
Version: $deb_version
Section: utils
Priority: optional
Architecture: amd64
Maintainer: Karbowanec-project <krbcoin@ukr.net>
Installed-Size: $installed_size
Depends: libc6 (>= 2.35), libgcc-s1, libstdc++6, libgl1, libx11-6, libxcb1, libxkbcommon-x11-0
Description: Karbowanec KRB wallet
 Karbowanec is Ukrainian decentralized, privacy oriented peer-to-peer
 cryptocurrency.
EOF

dpkg-deb --build "$pkgroot" "./appimage/$release_name.deb"
xvfb-run -a "$pkgroot/opt/karbo-wallet/AppRun" --version
