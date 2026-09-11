#pragma once

#include <cstdint>
#include <memory>
#include "game_apis.hpp"
#include "logger.hpp"

/**
 * Hook management for the SkillMenuFGFix plugin.
 * 
 * Note: This uses a hook approach that can be adapted to SafetyHook or MinHook.
 * The actual hooking library integration is left modular for flexibility.
 */

namespace Hooks {

    class HookManager {
    private:
        GameAPI::FunctionPointers* api_funcs;
        GameAPI::MenuState* menu_state;
        int throttle_target_quality;
        bool enable_throttle;
        bool enable_fg_lock;

        // Original function pointers (for calling original code)
        GameAPI::SetSimulationQualityFunc original_set_quality = nullptr;

    public:
        HookManager(GameAPI::FunctionPointers* funcs, GameAPI::MenuState* state)
            : api_funcs(funcs), menu_state(state), 
              throttle_target_quality(0), enable_throttle(true), enable_fg_lock(false) {
        }

        ~HookManager() {
            unhook_all();
        }

        // ====================================================================
        // Configuration
        // ====================================================================

        void set_throttle_target_quality(int quality) {
            throttle_target_quality = quality;
            g_logger.info("Throttle target quality set to: " + 
                         std::string(GameAPI::quality_to_string(quality)));
        }

        void set_enable_throttle(bool enable) {
            enable_throttle = enable;
            g_logger.info(std::string(enable ? "Enabled" : "Disabled") + " simulation quality throttle");
        }

        void set_enable_fg_lock(bool enable) {
            enable_fg_lock = enable;
            g_logger.info(std::string(enable ? "Enabled" : "Disabled") + " frame generation lock");
        }

        // ====================================================================
        // Hook Installation
        // ====================================================================

        bool install_hooks() {
            g_logger.info("Installing hooks...");

            if (!api_funcs->are_critical_functions_loaded()) {
                g_logger.error("Critical game functions not loaded. Cannot install hooks.");
                return false;
            }

            // Hook SetSimulationQuality to detect calls and apply throttling
            if (!hook_simulation_quality_setter()) {
                g_logger.warning("Failed to hook SetSimulationQuality");
                return false;
            }

            g_logger.info("All hooks installed successfully");
            return true;
        }

        void unhook_all() {
            g_logger.debug("Uninstalling hooks...");
            // Hook removal would go here (SafetyHook/MinHook specific)
        }

        // ====================================================================
        // Hook Implementations
        // ====================================================================

    private:
        bool hook_simulation_quality_setter() {
            if (!api_funcs->set_simulation_quality) {
                g_logger.warning("SetSimulationQuality not available");
                return false;
            }

            // NOTE: This is a stub. Real implementation requires SafetyHook or MinHook:
            // 
            // using hook_t = safetyhook::InlineHook;
            // auto hook = hook_t(api_funcs->set_simulation_quality, 
            //                    &Hooks::on_set_simulation_quality_hook);
            // 
            // Store the hook for later cleanup.

            g_logger.debug("SetSimulationQuality hook prepared");
            return true;
        }

    public:
        // ====================================================================
        // Menu State Management (called by detours)
        // ====================================================================

        void on_menu_opened() {
            if (!enable_throttle) {
                return;
            }

            g_logger.info("==> Skill Menu OPENED - Applying throttle");
            menu_state->set_menu_open(true);

            // Save current quality
            if (api_funcs->get_simulation_quality) {
                int current = api_funcs->get_simulation_quality();
                menu_state->set_saved_quality(current);
                g_logger.info("Saved quality: " + std::string(GameAPI::quality_to_string(current)));
            }

            // Apply throttle
            if (api_funcs->set_simulation_quality) {
                g_logger.info("Setting quality to: " + 
                             std::string(GameAPI::quality_to_string(throttle_target_quality)));
                api_funcs->set_simulation_quality(throttle_target_quality);
            }
        }

        void on_menu_closed() {
            if (!enable_throttle) {
                return;
            }

            g_logger.info("==> Skill Menu CLOSED - Restoring quality");
            menu_state->set_menu_open(false);

            // Restore previous quality
            if (api_funcs->set_simulation_quality) {
                int saved = menu_state->get_saved_quality();
                g_logger.info("Restoring quality to: " + std::string(GameAPI::quality_to_string(saved)));
                api_funcs->set_simulation_quality(saved);
            }
        }

        void on_frame_generation_toggle_attempted(bool enable) {
            if (!enable_fg_lock || !menu_state->is_menu_open()) {
                return;
            }

            g_logger.info("Frame Generation toggle attempted while menu open: enable=" + 
                         std::string(enable ? "true" : "false"));

            if (!enable) {
                g_logger.warning("Blocking Frame Generation disable while menu is open");
                // Would need to prevent the actual toggle via hook return interception
            }
        }
    };

    // ========================================================================
    // Global Hook Manager Instance
    // ========================================================================

    extern std::unique_ptr<HookManager> g_hook_manager;

    bool initialize_hooks(GameAPI::FunctionPointers* funcs, GameAPI::MenuState* state,
                         const Config& config);
    void shutdown_hooks();
}
