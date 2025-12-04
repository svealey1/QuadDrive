#!/bin/bash
#═══════════════════════════════════════════════════════════════════════════════
#  QUADDRIVE PROJECT HARNESS
#  Single command center for build, test, validate, and release workflows
#═══════════════════════════════════════════════════════════════════════════════
#
#  Usage:
#    ./harness.sh [command] [options]
#
#  Commands:
#    status      Show project status dashboard
#    build       Build plugin (auto-updates version)
#    test        Run validation checklist
#    check       Quick health check (no interaction)
#    phase       Show current phase progress
#    null        Run null test diagnostics
#    release     Prepare release package
#    setup       First-time setup
#    help        Show this help
#
#═══════════════════════════════════════════════════════════════════════════════

set -e

# Project configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_NAME="QuadDrive"
GITHUB_URL="https://github.com/svealey1/QuadDrive"

# Paths
VERSION_FILE="$PROJECT_ROOT/VERSION.txt"
CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"
PROCESSOR_FILE="$PROJECT_ROOT/Source/PluginProcessor.cpp"
EDITOR_FILE="$PROJECT_ROOT/Source/PluginEditor.cpp"

# Plugin output paths (macOS)
VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3/Quad-Blend Drive.vst3"
AU_PATH="$HOME/Library/Audio/Plug-Ins/Components/Quad-Blend Drive.component"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

#═══════════════════════════════════════════════════════════════════════════════
# HELPER FUNCTIONS
#═══════════════════════════════════════════════════════════════════════════════

get_version() {
    if [ -f "$VERSION_FILE" ]; then
        cat "$VERSION_FILE" | tr -d '\n' | tr -d ' '
    else
        echo "0.0.0"
    fi
}

get_cmake_version() {
    if [ -f "$CMAKE_FILE" ]; then
        grep -oE 'VERSION [0-9]+\.[0-9]+\.[0-9]+' "$CMAKE_FILE" | head -1 | cut -d' ' -f2
    else
        echo "?"
    fi
}

get_ui_version() {
    if [ -f "$EDITOR_FILE" ]; then
        grep -oE 'versionLabel\.setText\("[vV][^"]*"' "$EDITOR_FILE" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+[^"]*' | head -1
    else
        echo "?"
    fi
}

print_header() {
    echo ""
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}  ${BOLD}$PROJECT_NAME PROJECT HARNESS${NC}                                      ${BLUE}║${NC}"
    echo -e "${BLUE}║${NC}  ${DIM}$GITHUB_URL${NC}          ${BLUE}║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_section() {
    echo ""
    echo -e "${CYAN}━━━ $1 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

check_mark() {
    if [ "$1" = "pass" ]; then
        echo -e "${GREEN}✓${NC}"
    elif [ "$1" = "fail" ]; then
        echo -e "${RED}✗${NC}"
    elif [ "$1" = "warn" ]; then
        echo -e "${YELLOW}⚠${NC}"
    else
        echo -e "${DIM}?${NC}"
    fi
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: status
#═══════════════════════════════════════════════════════════════════════════════

cmd_status() {
    print_header
    
    VERSION=$(get_version)
    CMAKE_VER=$(get_cmake_version)
    UI_VER=$(get_ui_version)
    
    print_section "VERSION STATUS"
    
    echo -e "  VERSION.txt:     ${BOLD}$VERSION${NC}"
    
    BASE_VER="${VERSION%%-*}"
    if [ "$CMAKE_VER" = "$BASE_VER" ]; then
        echo -e "  CMakeLists.txt:  ${GREEN}$CMAKE_VER${NC} $(check_mark pass)"
    else
        echo -e "  CMakeLists.txt:  ${RED}$CMAKE_VER${NC} (expected $BASE_VER) $(check_mark fail)"
    fi
    
    if [ "$UI_VER" = "$VERSION" ]; then
        echo -e "  PluginEditor:    ${GREEN}$UI_VER${NC} $(check_mark pass)"
    else
        echo -e "  PluginEditor:    ${RED}$UI_VER${NC} (expected $VERSION) $(check_mark fail)"
    fi
    
    print_section "BUILD ARTIFACTS"
    
    if [ -d "$VST3_PATH" ]; then
        VST3_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$VST3_PATH" 2>/dev/null || echo "?")
        echo -e "  VST3:  ${GREEN}Found${NC} (modified: $VST3_DATE)"
    else
        echo -e "  VST3:  ${RED}Not found${NC}"
    fi
    
    if [ -d "$AU_PATH" ]; then
        AU_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$AU_PATH" 2>/dev/null || echo "?")
        echo -e "  AU:    ${GREEN}Found${NC} (modified: $AU_DATE)"
    else
        echo -e "  AU:    ${RED}Not found${NC}"
    fi
    
    print_section "CODE HEALTH"
    
    if [ -f "$PROCESSOR_FILE" ]; then
        DEBUG_COUNT=$(grep -c "std::cout\|printf\|DBG(" "$PROCESSOR_FILE" 2>/dev/null || echo "0")
        if [ "$DEBUG_COUNT" -gt 0 ]; then
            echo -e "  Debug output in DSP:    ${YELLOW}$DEBUG_COUNT occurrences${NC} $(check_mark warn)"
        else
            echo -e "  Debug output in DSP:    ${GREEN}Clean${NC} $(check_mark pass)"
        fi
        
        if grep -q "PROTECTION_ENABLE" "$PROCESSOR_FILE"; then
            echo -e "  Output limiter:         ${YELLOW}Old params present${NC} $(check_mark warn)"
        elif grep -q "OUTPUT_CEILING" "$PROCESSOR_FILE"; then
            echo -e "  Output limiter:         ${GREEN}Simplified${NC} $(check_mark pass)"
        else
            echo -e "  Output limiter:         ${DIM}Cannot determine${NC}"
        fi
    fi
    
    print_section "GIT STATUS"
    
    if [ -d "$PROJECT_ROOT/.git" ]; then
        BRANCH=$(git -C "$PROJECT_ROOT" branch --show-current 2>/dev/null || echo "?")
        DIRTY=$(git -C "$PROJECT_ROOT" status --porcelain 2>/dev/null | wc -l | tr -d ' ')
        
        echo -e "  Branch:          ${BOLD}$BRANCH${NC}"
        if [ "$DIRTY" -gt 0 ]; then
            echo -e "  Working tree:    ${YELLOW}$DIRTY uncommitted changes${NC}"
        else
            echo -e "  Working tree:    ${GREEN}Clean${NC}"
        fi
    else
        echo -e "  ${DIM}Not a git repository${NC}"
    fi
    
    echo ""
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: build
#═══════════════════════════════════════════════════════════════════════════════

cmd_build() {
    print_header
    
    VERSION=$(get_version)
    echo -e "${BOLD}Building QuadDrive v$VERSION${NC}"
    echo ""
    
    # Step 1: Update versions
    echo -e "${CYAN}[1/4]${NC} Updating version labels..."
    if [ -f "$PROJECT_ROOT/scripts/update_version.sh" ]; then
        bash "$PROJECT_ROOT/scripts/update_version.sh"
    else
        echo -e "${YELLOW}  Warning: scripts/update_version.sh not found${NC}"
        echo "  Run: ./harness.sh setup"
    fi
    
    # Step 2: Configure
    echo ""
    echo -e "${CYAN}[2/4]${NC} Configuring CMake..."
    cmake -B "$PROJECT_ROOT/build" -DCMAKE_BUILD_TYPE=Release -S "$PROJECT_ROOT"
    
    # Step 3: Build
    echo ""
    echo -e "${CYAN}[3/4]${NC} Compiling..."
    NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
    cmake --build "$PROJECT_ROOT/build" --config Release -j"$NUM_CORES"
    
    # Step 4: Verify
    echo ""
    echo -e "${CYAN}[4/4]${NC} Verifying outputs..."
    
    BUILD_SUCCESS=true
    if [ -d "$VST3_PATH" ]; then
        echo -e "  ${GREEN}✓${NC} VST3: $VST3_PATH"
    else
        echo -e "  ${RED}✗${NC} VST3 not found"
        BUILD_SUCCESS=false
    fi
    
    if [ -d "$AU_PATH" ]; then
        echo -e "  ${GREEN}✓${NC} AU: $AU_PATH"
    else
        echo -e "  ${RED}✗${NC} AU not found"
        BUILD_SUCCESS=false
    fi
    
    echo ""
    if [ "$BUILD_SUCCESS" = true ]; then
        echo -e "${GREEN}═══════════════════════════════════════════════════════════════════${NC}"
        echo -e "${GREEN}  BUILD SUCCESSFUL - v$VERSION${NC}"
        echo -e "${GREEN}═══════════════════════════════════════════════════════════════════${NC}"
        echo ""
        echo "Next: ./harness.sh test"
    else
        echo -e "${RED}  BUILD FAILED${NC}"
        exit 1
    fi
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: test
#═══════════════════════════════════════════════════════════════════════════════

cmd_test() {
    print_header
    
    VERSION=$(get_version)
    echo -e "${BOLD}QuadDrive Validation Suite v$VERSION${NC}"
    echo ""
    
    passed=0
    failed=0
    skipped=0
    
    ask_test() {
        local test_id="$1"
        local desc="$2"
        
        echo -e "${YELLOW}[$test_id]${NC} $desc"
        read -p "  Result? (p=pass/f=fail/s=skip) > " result
        
        case $result in
            [Pp]* ) 
                echo -e "  ${GREEN}✓ PASS${NC}"
                ((passed++))
                ;;
            [Ss]* )
                echo -e "  ${DIM}⊘ SKIPPED${NC}"
                ((skipped++))
                ;;
            * )
                echo -e "  ${RED}✗ FAIL${NC}"
                ((failed++))
                ;;
        esac
        echo ""
    }
    
    print_section "VERSION VERIFICATION"
    ask_test "VER-01" "VERSION.txt exists and shows $VERSION"
    ask_test "VER-02" "CMakeLists.txt VERSION matches"
    ask_test "VER-03" "UI label in DAW shows v$VERSION"
    
    print_section "NULL TESTS (Plugin Doctor)"
    ask_test "NULL-01" "BYPASS ON → Flat response (perfect null)"
    ask_test "NULL-02" "MIX 0% → Flat response (dry path pristine)"
    ask_test "NULL-03" "All processors muted → Clean passthrough"
    
    print_section "SIGNAL INTEGRITY"
    ask_test "SIG-01" "Dry/Wet sweep → NO comb filtering"
    ask_test "SIG-02" "TRUE PEAK ON → Output never exceeds ceiling"
    ask_test "SIG-03" "Extreme settings → No artifacts"
    
    print_section "METERING"
    ask_test "MTR-01" "Waveform updates in real-time"
    ask_test "MTR-02" "GR trace aligns with waveform"
    
    print_section "DAW COMPATIBILITY"
    ask_test "DAW-01" "Ableton Live → Loads correctly"
    ask_test "DAW-02" "Logic Pro (AU) → Loads correctly"
    ask_test "DAW-03" "Parameter automation → No zipper noise"
    
    print_section "PROCESSING MODES"
    ask_test "MODE-01" "Zero Latency → Works"
    ask_test "MODE-02" "8× Oversampling → Works"
    ask_test "MODE-03" "16× Oversampling → Works"
    
    print_section "RESULTS"
    total=$((passed + failed))
    
    echo -e "  Passed:  ${GREEN}$passed${NC}"
    echo -e "  Failed:  ${RED}$failed${NC}"
    echo -e "  Skipped: ${DIM}$skipped${NC}"
    echo ""
    
    if [ $failed -eq 0 ] && [ $passed -gt 0 ]; then
        echo -e "${GREEN}  ALL TESTS PASSED${NC}"
        return 0
    else
        echo -e "${RED}  $failed TESTS FAILED${NC}"
        return 1
    fi
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: check
#═══════════════════════════════════════════════════════════════════════════════

cmd_check() {
    print_header
    echo -e "${BOLD}Quick Health Check${NC}"
    echo ""
    
    issues=0
    
    VERSION=$(get_version)
    CMAKE_VER=$(get_cmake_version)
    UI_VER=$(get_ui_version)
    BASE_VER="${VERSION%%-*}"
    
    if [ ! -f "$VERSION_FILE" ]; then
        echo -e "$(check_mark fail) VERSION.txt missing"
        ((issues++))
    else
        echo -e "$(check_mark pass) VERSION.txt: $VERSION"
    fi
    
    if [ "$CMAKE_VER" != "$BASE_VER" ]; then
        echo -e "$(check_mark fail) CMakeLists.txt mismatch: $CMAKE_VER != $BASE_VER"
        ((issues++))
    else
        echo -e "$(check_mark pass) CMakeLists.txt synced"
    fi
    
    if [ "$UI_VER" != "$VERSION" ]; then
        echo -e "$(check_mark fail) UI version mismatch: $UI_VER != $VERSION"
        ((issues++))
    else
        echo -e "$(check_mark pass) UI version synced"
    fi
    
    if [ ! -f "$PROJECT_ROOT/scripts/update_version.sh" ]; then
        echo -e "$(check_mark fail) scripts/update_version.sh missing"
        ((issues++))
    else
        echo -e "$(check_mark pass) Version script present"
    fi
    
    echo ""
    if [ $issues -eq 0 ]; then
        echo -e "${GREEN}All checks passed${NC}"
        return 0
    else
        echo -e "${RED}$issues issues found${NC}"
        return 1
    fi
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: phase
#═══════════════════════════════════════════════════════════════════════════════

cmd_phase() {
    print_header
    echo -e "${BOLD}Phase Progress${NC}"
    
    print_section "PHASE 0: HOUSEKEEPING"
    
    VERSION=$(get_version)
    CMAKE_VER=$(get_cmake_version)
    UI_VER=$(get_ui_version)
    BASE_VER="${VERSION%%-*}"
    
    if [ -f "$VERSION_FILE" ]; then
        echo -e "  ${GREEN}[✓]${NC} VERSION.txt exists ($VERSION)"
    else
        echo -e "  ${RED}[ ]${NC} VERSION.txt missing"
    fi
    
    if [ "$CMAKE_VER" = "$BASE_VER" ] && [ "$UI_VER" = "$VERSION" ]; then
        echo -e "  ${GREEN}[✓]${NC} All versions synced"
    else
        echo -e "  ${RED}[ ]${NC} Version sync broken"
    fi
    
    if [ -f "$PROJECT_ROOT/scripts/update_version.sh" ]; then
        echo -e "  ${GREEN}[✓]${NC} Update script installed"
    else
        echo -e "  ${RED}[ ]${NC} Update script missing"
    fi
    
    echo -e "  ${YELLOW}[?]${NC} Null tests passing (verify in Plugin Doctor)"
    echo -e "  ${YELLOW}[?]${NC} Output limiter simplified"
    echo -e "  ${YELLOW}[?]${NC} Slew rate bug fixed"
    
    print_section "UPCOMING"
    echo -e "  ${DIM}Phase 1: DSP Lock${NC}"
    echo -e "  ${DIM}Phase 2: UI Polish${NC}"
    echo -e "  ${DIM}Phase 3: Platform Expansion${NC}"
    echo -e "  ${DIM}Phase 4: Beta Testing${NC}"
    echo -e "  ${DIM}Phase 5: Polish & Presets${NC}"
    echo -e "  ${DIM}Phase 6: Code Signing${NC}"
    echo -e "  ${DIM}Phase 7: Licensing${NC}"
    echo -e "  ${DIM}Phase 8: Launch${NC}"
    echo ""
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: null
#═══════════════════════════════════════════════════════════════════════════════

cmd_null() {
    print_header
    echo -e "${BOLD}Null Test Guide${NC}"
    
    print_section "TEST PROCEDURE"
    echo ""
    echo "Open Plugin Doctor and load Quad-Blend Drive"
    echo ""
    echo -e "${CYAN}Test 1: BYPASS NULL${NC}"
    echo "  1. Enable BYPASS"
    echo "  2. Run frequency sweep"
    echo "  3. Expected: Perfectly flat"
    echo ""
    echo -e "${CYAN}Test 2: MIX 0% NULL${NC}"
    echo "  1. Disable BYPASS, Set MIX to 0%"
    echo "  2. Run frequency sweep"
    echo "  3. Expected: Perfectly flat"
    echo ""
    echo -e "${CYAN}Test 3: ALL MUTED${NC}"
    echo "  1. MIX 100%, mute all processors"
    echo "  2. Run frequency sweep"
    echo "  3. Expected: Clean passthrough"
    echo ""
    
    print_section "IF MIX 0% FAILS"
    echo ""
    echo "Add threshold check in PluginProcessor.cpp:"
    echo ""
    echo -e "${DIM}  const float mixWet = smoothedMixWet.getNextValue();${NC}"
    echo -e "${DIM}  if (mixWet < 0.0001f) {${NC}"
    echo -e "${DIM}      // Pure dry - skip wet blend${NC}"
    echo -e "${DIM}      return;${NC}"
    echo -e "${DIM}  }${NC}"
    echo ""
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: setup
#═══════════════════════════════════════════════════════════════════════════════

cmd_setup() {
    print_header
    echo -e "${BOLD}Setting up project harness...${NC}"
    echo ""
    
    mkdir -p "$PROJECT_ROOT/scripts"
    
    if [ ! -f "$VERSION_FILE" ]; then
        echo "1.8.4" > "$VERSION_FILE"
        echo -e "${GREEN}✓${NC} Created VERSION.txt"
    else
        echo -e "${DIM}⊘${NC} VERSION.txt exists"
    fi
    
    cat > "$PROJECT_ROOT/scripts/update_version.sh" << 'SCRIPT'
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
SCRIPT
    chmod +x "$PROJECT_ROOT/scripts/update_version.sh"
    echo -e "${GREEN}✓${NC} Created scripts/update_version.sh"
    
    echo ""
    echo -e "${GREEN}Setup complete!${NC}"
    echo "Next: ./harness.sh build"
}

#═══════════════════════════════════════════════════════════════════════════════
# COMMAND: help
#═══════════════════════════════════════════════════════════════════════════════

cmd_help() {
    print_header
    
    echo -e "${BOLD}COMMANDS${NC}"
    echo ""
    echo -e "  ${CYAN}status${NC}   Dashboard (version, builds, health)"
    echo -e "  ${CYAN}build${NC}    Build with auto version sync"
    echo -e "  ${CYAN}test${NC}     Interactive validation checklist"
    echo -e "  ${CYAN}check${NC}    Quick health check (CI-friendly)"
    echo -e "  ${CYAN}phase${NC}    Phase progress tracker"
    echo -e "  ${CYAN}null${NC}     Null test diagnostic guide"
    echo -e "  ${CYAN}setup${NC}    First-time initialization"
    echo -e "  ${CYAN}help${NC}     This help"
    echo ""
    echo -e "${BOLD}EXAMPLES${NC}"
    echo "  ./harness.sh status"
    echo "  ./harness.sh build"
    echo "  ./harness.sh check && git push"
    echo ""
}

#═══════════════════════════════════════════════════════════════════════════════
# MAIN
#═══════════════════════════════════════════════════════════════════════════════

main() {
    cd "$PROJECT_ROOT"
    
    case "${1:-help}" in
        status)  cmd_status ;;
        build)   cmd_build ;;
        test)    cmd_test ;;
        check)   cmd_check ;;
        phase)   cmd_phase ;;
        null)    cmd_null ;;
        setup)   cmd_setup ;;
        help)    cmd_help ;;
        *)       
            echo "Unknown: $1"
            echo "Run: ./harness.sh help"
            exit 1
            ;;
    esac
}

main "$@"
