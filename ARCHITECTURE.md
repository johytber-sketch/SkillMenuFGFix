# SkillMenuFGFix Plugin Architecture

## Problem Statement
Crimson Desert's Skill Menu and other fullscreen UI overlays automatically disable Frame Generation (DLSS FG / FSR FG / XeSS FG), but the 3D world continues rendering at full load. This causes:
- Severe UI input lag and stuttering
- GPU power spikes
- Poor frame time variance (1% lows)

## Solution Strategy

### Primary Approach (Default): Soft Simulation Quality Throttle
When a heavy menu opens:
1. Detect menu state via game UI manager or panel function hooks
2. Query current Simulation Quality setting
3. Temporarily reduce to **Minimum/Low** while menu is open
4. Restore previous value when menu closes
5. **Advantage**: Non-intrusive, game-native, survives patches better

### Secondary Approach (Optional): Frame Generation Lock
Intercept FG enable/disable and prevent disabling while menu is open.
- **Disadvantage**: Fragile, may cause HUD artifacts, fights game logic
- Enabled via config if desired

## Required Game Function Signatures

### 1. Menu State Detection
**Function**: UI Panel Open/Close Handler
- **Purpose**: Detect when Skill Menu panel enters/exits open state
- **Location**: BlackSpace engine UI manager
- **Pattern**: Look for panel toggle functions or state flags
- **Usage**: Hook to track menu visibility

**Alternative**: UI Manager Flag Reader
- Direct read of menu-open boolean in UI manager
- More robust than function hooks

### 2. Simulation Quality Setter
**Function**: `SetSimulationQuality(int quality_level)` or similar
- **Purpose**: Modify world simulation load
- **Location**: Game settings/world manager
- **Pattern**: Look for quality enum checks (0=Min, 1=Low, 2=Medium, 3=High)
- **Usage**: Call before/after menu state changes

**Quality Levels** (inferred):
- 0 = Minimum
- 1 = Low
- 2 = Medium
- 3 = High

### 3. Simulation Quality Getter (Optional)
**Function**: `GetSimulationQuality()` or read from settings struct
- **Purpose**: Query current quality before changing
- **Usage**: Store previous value for restoration

### 4. Frame Generation Toggle (Optional, Secondary Strategy)
**Function**: `SetFrameGenerationEnabled(bool enabled)` or direct DLSS/FSR API hook
- **Purpose**: Prevent FG disable
- **Location**: Graphics/rendering subsystem
- **Pattern**: Look for Streamline API calls or vendor-specific FG enable/disable

## Hook Strategy

| Function | Hook Type | Priority | Risk |
|----------|-----------|----------|------|
| Menu Open/Close | Mid-function or post-call | HIGH | Low (state detection only) |
| SimQuality Setter | Pre-call | MEDIUM | Low (game-native setter) |
| FG Toggle | Pre-call | OPTIONAL | Medium (may fight game logic) |

## Code Organization

```
SkillMenuFGFix/
├── src/
│   ├── dllmain.cpp              # Entry point, initialization
│   ├── hooks.cpp/hpp            # All hook implementations
│   ├── pattern.hpp              # AOB scanner and signature definitions
│   ├── logger.hpp               # Logging utility (file + console)
│   ├── config.hpp               # INI config reader
│   ├── version.h                # Version constants
│   └── game_apis.hpp            # Game function pointers and types
├── CMakeLists.txt               # Build configuration
├── .gitignore                   # Git ignore rules
├── SkillMenuFGFix.ini           # Example configuration
└── README.md                    # User documentation
```

## Pattern Scanning Strategy

1. **Delay**: Wait 2-4 seconds after DLL load for all modules to settle
2. **Scope**: Scan primary game executable (CrimsonDesert.exe)
3. **Fallback**: Scan loaded DLLs in order if primary fails
4. **Logging**: Log all discovered addresses for debugging
5. **Error Handling**: If critical signature not found, disable feature and log warning

## Thread Safety

- Use `std::atomic<bool>` for menu state flags
- Protect quality level with `std::atomic<int>`
- All hook functions must be thread-safe (minimal locking)
- Read-only operations don't require locking

## Configuration (SkillMenuFGFix.ini)

```ini
[General]
Enabled=1
LogLevel=2          ; 0=Error, 1=Warning, 2=Info, 3=Debug
EnableMenuDetection=1
EnableSimQualityThrottle=1
EnableFGLock=0      ; Disabled by default (fragile)

[SimulationQuality]
TargetQualityInMenu=0  ; 0=Min, 1=Low, 2=Med, 3=High
RestoreOnMenuClose=1
ThrottleDelay=50    ; ms to wait before applying throttle

[FrameGeneration]
PreventDisableInMenu=0
WarnOnConflict=1
```

## Testing Checklist

- [ ] Plugin loads without crashing
- [ ] Menu state detection works (log output confirms)
- [ ] Simulation Quality changes on menu open/close
- [ ] Frame rate / GPU load improves when menu is open
- [ ] Restore on close is reliable
- [ ] Config INI is read correctly
- [ ] Multiple open/close cycles work
- [ ] Plugin survives game restart

## Known Limitations & Future Work

- **Limitation**: Signature scanning requires reverse engineering investment
- **Limitation**: Each game patch may require signature update
- **Future**: Community signature database / auto-update mechanism
- **Future**: Per-menu customization (different throttle for different UI)
- **Future**: Telemetry on effectiveness
