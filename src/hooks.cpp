#include "hooks.hpp"
#include "config.hpp"
#include "logger.hpp"

namespace Hooks {
    std::unique_ptr<HookManager> g_hook_manager = nullptr;

    bool initialize_hooks(GameAPI::FunctionPointers* funcs, GameAPI::MenuState* state,
                         const Config& config) {
        if (g_hook_manager) {
            g_logger.warning("Hook manager already initialized");
            return true;
        }

        g_hook_manager = std::make_unique<HookManager>(funcs, state);

        // Configure from INI
        int target_quality = config.get_int("SimulationQuality", "TargetQualityInMenu", 0);
        bool enable_throttle = config.get_bool("SimulationQuality", "RestoreOnMenuClose", true);
        bool enable_fg_lock = config.get_bool("FrameGeneration", "PreventDisableInMenu", false);

        g_hook_manager->set_throttle_target_quality(target_quality);
        g_hook_manager->set_enable_throttle(enable_throttle);
        g_hook_manager->set_enable_fg_lock(enable_fg_lock);

        // Install all hooks
        if (!g_hook_manager->install_hooks()) {
            g_logger.error("Failed to install hooks");
            g_hook_manager.reset();
            return false;
        }

        return true;
    }

    void shutdown_hooks() {
        if (g_hook_manager) {
            g_hook_manager->unhook_all();
            g_hook_manager.reset();
        }
    }
}
