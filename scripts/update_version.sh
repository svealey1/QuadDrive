#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION=$(cat "$PROJECT_ROOT/VERSION.txt" | tr -d '\n' | tr -d ' ')
echo "📝 Updating to: $VERSION"
BASE_VERSION=$(echo "$VERSION" | cut -d'-' -f1)

if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    sed -i.bak -E "s/(project\([^)]*VERSION )[0-9]+\.[0-9]+\.[0-9]+/\1$BASE_VERSION/" "$PROJECT_ROOT/CMakeLists.txt"
    rm -f "$PROJECT_ROOT/CMakeLists.txt.bak"
    echo "✓ CMakeLists.txt → $BASE_VERSION"
fi

if [ -f "$PROJECT_ROOT/Source/PluginEditor.cpp" ]; then
    sed -i.bak -E 's/(versionLabel\.setText\(")[vV][^"]*"/\1v'"$VERSION"'"/' "$PROJECT_ROOT/Source/PluginEditor.cpp"
    rm -f "$PROJECT_ROOT/Source/PluginEditor.cpp.bak"
    echo "✓ PluginEditor.cpp → v$VERSION"
fi

echo "✅ Done!"
