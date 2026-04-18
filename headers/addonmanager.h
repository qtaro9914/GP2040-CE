#ifndef _ADDONMANAGER_H_
#define _ADDONMANAGER_H_

#include "gpaddon.h"

#include <vector>
#include <memory>

enum ADDON_PROCESS {
    CORE0_INPUT,
    CORE0_USBREPORT,
    CORE1_ALWAYS,
    CORE1_LOOP
};

struct AddonBlock {
    std::unique_ptr<GPAddon> ptr;
    ADDON_PROCESS process;
};

class AddonManager {
public:
    AddonManager() {}
    ~AddonManager() {}
    bool LoadAddon(std::unique_ptr<GPAddon>);
    bool LoadUSBAddon(std::unique_ptr<GPAddon>);
    void ReinitializeAddons();
    void PreprocessAddons();
    void ProcessAddons();
    void PostprocessAddons(bool);
    GPAddon * GetAddon(std::string); // hack for NeoPicoLED
private:
    std::vector<std::unique_ptr<AddonBlock>> addons;    // addons currently loaded
    // Categorized addon lists for optimized iteration
    std::vector<GPAddon*> preprocessList;
    std::vector<GPAddon*> processList;
    std::vector<GPAddon*> postprocessList;
};

#endif
