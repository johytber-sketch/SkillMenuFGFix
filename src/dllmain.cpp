#include <Windows.h>
#include <thread>
#include <chrono>
#include "logger.hpp"
#include "config.hpp"
#include "game_apis.hpp"
#include "pattern.hpp"
#include "hooks.hpp"
#include "version.h"

// ============================================================================
// Global State
// ============================================================================

Logger g_logger;
GameAPI::FunctionPointers g_api_funcs;
GameAPI::MenuState g_menu_state;
Config g_config;

// ============================================================================
// Function Discovery via Pattern Scanning
// ============================================================================

bool discover_game_functions() {
    g_logger.info("Starting game function discovery via pattern scanning...");

    // Validate that patterns are filled in
    if (!GameSignatures::validate_patterns()) {
        g_logger.error("Game signatures are empty! Reverse engineer must fill in AOB patterns.");
        g_logger.error("See pattern.hpp for required signatures.");
        return false;
    }

    // Scan for SetSimulationQuality
    uintptr_t set_quality_addr = PatternScanner::scan_module("CrimsonDesert.exe", 
                                                              GameSignatures::SIMULATION_QUALITY_SETTER_PATTERN);
    if (set_quality_addr) {
        g_api_funcs.set_simulation_quality = reinterpret_cast<GameAPI::SetSimulationQualityFunc>(set_quality_addr);
    }

    // Scan for GetSimulationQuality (optional)
    uintptr_t get_quality_addr = PatternScanner::scan_module("CrimsonDesert.exe",
                                                              GameSignatures::SIMULATION_QUALITY_GETTER_PATTERN);
    if (get_quality_addr) {
        g_api_funcs.get_simulation_quality = reinterpret_cast<GameAPI::GetSimulationQualityFunc>(get_quality_addr);
    }

    // Scan for SetFrameGeneration (optional)
    uintptr_t fg_toggle_addr = PatternScanner::scan_module("CrimsonDesert.exe",
                                                            GameSignatures::FRAME_GENERATION_TOGGLE_PATTERN);
    if (fg_toggle_addr) {
        g_api_funcs.set_frame_generation = reinterpret_cast<GameAPI::SetFrameGenerationFunc>(fg_toggle_addr);
    }

    g_api_funcs.log_status();

    if (!g_api_funcs.are_critical_functions_loaded()) {
        g_logger.error("Failed to discover critical game functions!");
        return false;
    }

    g_logger.info("Game function discovery complete!");
    return true;
}

// ============================================================================
// Initialization
// ============================================================================

bool initialize_plugin() {
    g_logger.info("========================================");
    g_logger.info(SKILLMENU_PLUGIN_NAME " v" SKILLMENU_VERSION_STRING);
    g_logger.info("========================================");
    
    // Load configuration
    if (!g_config.load("SkillMenuFGFix.ini")) {
        g_logger.warning("SkillMenuFGFix.ini not found. Using defaults.");
    } else {
        g_logger.info("Configuration loaded from SkillMenuFGFix.ini");
    }

    // Check if plugin is enabled
    if (!g_config.get_bool("General", "Enabled", true)) {
        g_logger.warning("Plugin is disabled in configuration");
        return false;
    }

    // Set log level from config
    LogLevel log_level = static_cast<LogLevel>(g_config.get_int("General", "LogLevel", 2));
    g_logger.set_level(log_level);

    // Wait for game to fully load modules
    g_logger.info("Waiting 3 seconds for game modules to load...");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Discover game functions
    if (!discover_game_functions()) {
        g_logger.error("Plugin initialization failed: could not discover game functions");
        return false;
    }

    // Initialize hooks
    if (!Hooks::initialize_hooks(&g_api_funcs, &g_menu_state, g_config)) {
        g_logger.error("Plugin initialization failed: could not initialize hooks");
        return false;
    }

    g_logger.info("Plugin initialized successfully!");
    return true;
}

void shutdown_plugin() {
    g_logger.info("Shutting down plugin...");
    Hooks::shutdown_hooks();
    g_menu_state.reset();
    g_logger.info("Plugin shutdown complete");
}

// ============================================================================
// DLL Entry Point
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            // Log file path: <game_directory>/SkillMenuFGFix.log
            char log_path[MAX_PATH];
            if (GetCurrentDirectoryA(MAX_PATH, log_path)) {
                strcat_s(log_path, MAX_PATH, "\\SkillMenuFGFix.log");
            } else {
                strcpy_s(log_path, MAX_PATH, "SkillMenuFGFix.log");
            }

            // Initialize logger
            if (!g_logger.initialize(log_path, LogLevel::Info)) {
                return FALSE;
            }

            // Run initialization in a separate thread to avoid blocking game startup
            std::thread init_thread([]() {
                if (!initialize_plugin()) {
                    g_logger.error("Plugin initialization thread failed");
                }
            });
            init_thread.detach();

            break;
        }

        case DLL_PROCESS_DETACH: {
            shutdown_plugin();
            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

// ============================================================================
// ASI Loader Export (for Ultimate ASI Loader)
// ============================================================================

extern "C" {
    __declspec(dllexport) void ASIPluginInit() {
        // Plugin is already initialized via DllMain
    }
}
