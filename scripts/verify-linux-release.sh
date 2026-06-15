#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$(sed -n \
  's/^project(Mach3SurfacingGenerator VERSION \([0-9.]*\).*/\1/p' \
  "$ROOT/CMakeLists.txt")"
PACKAGE="$ROOT/dist/mach3surfacinggenerator_${VERSION}_amd64.deb"
TEST_ROOT="/tmp/mach3-surfacing-release-test"

if [[ ! -f "$PACKAGE" ]]; then
  echo "Paquet introuvable: $PACKAGE" >&2
  exit 1
fi

echo "--- package metadata ---"
dpkg-deb -f "$PACKAGE" Package Version Architecture Depends

rm -rf "$TEST_ROOT"
mkdir -p "$TEST_ROOT"
dpkg-deb -x "$PACKAGE" "$TEST_ROOT"

BINARY="$TEST_ROOT/usr/bin/Mach3SurfacingGenerator"
DESKTOP="$TEST_ROOT/usr/share/applications/Mach3SurfacingGenerator.desktop"

[[ "$(stat -c '%a' "$BINARY")" == "755" ]]
[[ "$(stat -c '%a' "$DESKTOP")" == "644" ]]

echo "--- smoke test ---"
set +e
QT_QPA_PLATFORM=offscreen timeout 4s "$BINARY" \
  >"$TEST_ROOT/smoke.log" 2>&1
result=$?
set -e
cat "$TEST_ROOT/smoke.log"

if [[ $result -ne 124 ]]; then
  echo "Le programme s'est arrêté avec le code $result." >&2
  exit 1
fi

echo "Linux package verification: OK"
