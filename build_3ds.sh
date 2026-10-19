#!/usr/bin/env bash
# Nintendo 3DS build (devkitARM + libctru + citro3d).
#
# Run it from the devkitPro MSYS2 shell on Windows, or from any shell on
# Linux/macOS with devkitPro installed. Required packages:
#   3ds-dev 3ds-zlib 3ds-libpng 3ds-libjpeg-turbo
#
# Usage:
#   ./build_3ds.sh               Old 3DS profile (default)
#   ./build_3ds.sh new3ds        New 3DS profile
#   ./build_3ds.sh clean new3ds  wipe that profile's build dir first
#
# Output: build/3ds-<profile>/pvz_<profile>.3dsx, plus pvz_<profile>.elf with
# debug info (not part of the .3dsx) to turn [CRASH] addresses in log.txt into
# source lines: arm-none-eabi-addr2line -f -C -e pvz_<profile>.elf <pc> <lr>
set -e
cd "$(dirname "$0")"

PROFILE=old3ds
CLEAN=
for ARG in "$@"; do
	case "$ARG" in
		old3ds|OLD3DS) PROFILE=old3ds ;;
		new3ds|NEW3DS) PROFILE=new3ds ;;
		clean)         CLEAN=1 ;;
		*) echo "Unknown argument: $ARG (expected old3ds, new3ds or clean)"; exit 1 ;;
	esac
done

if [ -z "$DEVKITPRO" ]; then
	echo "DEVKITPRO is not set: install devkitPro and run this from its shell."
	exit 1
fi

BUILD_DIR="build/3ds-$PROFILE"
if [ -n "$CLEAN" ]; then
	rm -rf "$BUILD_DIR"
fi

"$DEVKITPRO/portlibs/3ds/bin/arm-none-eabi-cmake" -S . -B "$BUILD_DIR" \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DPVZ_3DS_PROFILE=$(echo "$PROFILE" | tr a-z A-Z)
cmake --build "$BUILD_DIR" --parallel

echo
echo "Done: $BUILD_DIR/pvz_$PROFILE.3dsx"
