#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$(sed -n \
  's/^project(Mach3SurfacingGenerator VERSION \([0-9.]*\).*/\1/p' \
  "$ROOT/CMakeLists.txt")"

if [[ -z "$VERSION" ]]; then
  echo "Version du projet introuvable." >&2
  exit 1
fi

BUILD_DIR="${MACH3_LINUX_BUILD_DIR:-$HOME/.cache/Mach3SurfacingGenerator/build-$VERSION}"
DIST_DIR="$ROOT/dist"

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON
cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure

mkdir -p "$DIST_DIR"
rm -f \
  "$DIST_DIR/Mach3SurfacingGenerator-${VERSION}-linux-x86_64.tar.gz" \
  "$DIST_DIR/mach3surfacinggenerator_${VERSION}-1_amd64.deb"

(
  cd "$BUILD_DIR"
  cpack -G TGZ
  cpack -G DEB
)

cp "$BUILD_DIR/Mach3SurfacingGenerator-${VERSION}-linux-x86_64.tar.gz" \
  "$DIST_DIR/"

DEB_FILE="$(find "$BUILD_DIR" -maxdepth 1 -type f -name '*.deb' | head -n 1)"
if [[ -z "$DEB_FILE" ]]; then
  echo "Paquet DEB introuvable." >&2
  exit 1
fi
cp "$DEB_FILE" "$DIST_DIR/"

echo "Releases Linux disponibles dans $DIST_DIR"
