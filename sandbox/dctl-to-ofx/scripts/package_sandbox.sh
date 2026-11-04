#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RUNNER_PATH="$BASE_DIR/runners/macos/NexDCTLRunner.ofx"

if [ ! -f "$RUNNER_PATH" ]; then
  echo "Error: macOS runner binary not found at $RUNNER_PATH"
  exit 1
fi

# Locate nexkeyctl CLI
NEXKEYCTL=""
if command -v nexkeyctl >/dev/null 2>&1; then
  NEXKEYCTL="nexkeyctl"
elif [ -x "$BASE_DIR/bin/nexkeyctl" ]; then
  NEXKEYCTL="$BASE_DIR/bin/nexkeyctl"
elif [ -x "$BASE_DIR/../../tools/nexkeyctl/build/nexkeyctl" ]; then
  NEXKEYCTL="$BASE_DIR/../../tools/nexkeyctl/build/nexkeyctl"
elif [ -x "$BASE_DIR/../../../build-universal/tools/nexkeyctl/nexkeyctl" ]; then
  NEXKEYCTL="$BASE_DIR/../../../build-universal/tools/nexkeyctl/nexkeyctl"
elif [ -x "$BASE_DIR/../../../build/tools/nexkeyctl/nexkeyctl" ]; then
  NEXKEYCTL="$BASE_DIR/../../../build/tools/nexkeyctl/nexkeyctl"
else
  echo "Error: nexkeyctl executable not found. Please install it or place in PATH / bin."
  exit 1
fi

INPUT_FILE="$1"
if [ -z "$INPUT_FILE" ]; then
  echo "NexKeyRuntime DCTL-to-OFX Sandbox Packager (macOS)"
  echo "------------------------------------------------"
  read -r -p "Enter path to .dctl file (or drag and drop here): " INPUT_FILE
  # Remove enclosing quotes if dragged from Finder
  INPUT_FILE="${INPUT_FILE%\"}"
  INPUT_FILE="${INPUT_FILE#\"}"
  INPUT_FILE="${INPUT_FILE%\'}"
  INPUT_FILE="${INPUT_FILE#\'}"
fi

if [ ! -f "$INPUT_FILE" ]; then
  echo "Error: input file '$INPUT_FILE' not found."
  exit 1
fi

OUTPUT_DIR="${2:-$BASE_DIR/dist}"
mkdir -p "$OUTPUT_DIR"

echo "Packaging $INPUT_FILE into $OUTPUT_DIR in Sandbox mode..."
"$NEXKEYCTL" package-dctl \
  --input="$INPUT_FILE" \
  --runner="$RUNNER_PATH" \
  --output="$OUTPUT_DIR" \
  --sandbox

echo ""
echo "Success! Plugin bundle created in: $OUTPUT_DIR"
echo "To test in DaVinci Resolve, copy the .ofx.bundle to /Library/OFX/Plugins/:"
echo "  sudo cp -R \"$OUTPUT_DIR\"/*.ofx.bundle /Library/OFX/Plugins/"

