#pragma once

#include <cstdint>
#include <functional>
#include <atomic>
#include "logger.hpp"

/**
 * Game API function pointer types and wrapper.
 * These define the calling conventions and signatures for hooked functions.
 */

namespace GameAPI {

    // ========================================================================
    // Type Definitions
    // ========================================================================

    /// Menu open/close handler type
    using MenuStateCallback = std::function<void(bool is_open)>;

    /// Simulation quality setter
    using SetSimulationQualityFunc = void(*)(int quality_level);
    
    /// Simulation quality getter
    using GetSimulationQualityFunc = int(*)();

    /// Frame generation toggle
    using SetFrameGenerationFunc = void(*)(bool enabled);

    // ========================================================================
    // Game Function Pointers (discovered via pattern scanning)
    // ========================================================================

    class FunctionPointers {
    public:
        SetSimulationQualityFunc set_simulation_quality = nullptr;
        GetSimulationQualityFunc get_simulation_quality = nullptr;
        SetFrameGenerationFunc set_frame_generation = nullptr;

        bool are_critical_functions_loaded() const {
            return set_simulation_quality != nullptr;
        }

        void log_status() const {
            if (set_simulation_quality) {
                g_logger.info("SetSimulationQuality found");
            } else {
                g_logger.warning("SetSimulationQuality NOT FOUND");
            }

            if (get_simulation_quality) {
                g_logger.info("GetSimulationQuality found");
            } else {
                g_logger.debug("GetSimulationQuality not found (optional)");
            }

            if (set_frame_generation) {
                g_logger.info("SetFrameGeneration found");
            } else {
                g_logger.debug("SetFrameGeneration not found (optional)");
            }
        }
    };

    // ========================================================================
    // Menu State Tracking
    // ========================================================================

    class MenuState {
    private:
        std::atomic<bool> is_skill_menu_open{false};
        std::atomic<int> saved_simulation_quality{2}; // Default: Medium

    public:
        bool is_menu_open() const {
            return is_skill_menu_open.load(std::memory_order_acquire);
        }

        void set_menu_open(bool open) {
            is_skill_menu_open.store(open, std::memory_order_release);
        }

        int get_saved_quality() const {
            return saved_simulation_quality.load(std::memory_order_acquire);
        }

        void set_saved_quality(int quality) {
            saved_simulation_quality.store(quality, std::memory_order_release);
        }

        void reset() {
            is_skill_menu_open.store(false, std::memory_order_release);
            saved_simulation_quality.store(2, std::memory_order_release);
        }
    };

    // ========================================================================
    // Simulation Quality Helpers
    // ========================================================================

    enum class SimulationQuality : int {
        Minimum = 0,
        Low = 1,
        Medium = 2,
        High = 3
    };

    inline const char* quality_to_string(SimulationQuality q) {
        switch (q) {
            case SimulationQuality::Minimum: return "Minimum";
            case SimulationQuality::Low:     return "Low";
            case SimulationQuality::Medium:  return "Medium";
            case SimulationQuality::High:    return "High";
            default:                         return "Unknown";
        }
    }

    inline const char* quality_to_string(int q) {
        return quality_to_string(static_cast<SimulationQuality>(q));
    }
}

// Forward declaration for pattern scanner integration
#include "pattern.hpp"
