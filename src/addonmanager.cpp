#include "addonmanager.h"
#include "usbhostmanager.h"

bool AddonManager::LoadAddon(std::unique_ptr<GPAddon> addon) {
    if (addon->available()) {
        auto block = std::make_unique<AddonBlock>();
        addon->setup();
        // Add to categorized lists for optimized iteration
        preprocessList.push_back(addon.get());
        processList.push_back(addon.get());
        postprocessList.push_back(addon.get());

        block->ptr = std::move(addon);
        addons.push_back(std::move(block));
        
        
        return true;
    } 
    // unique_ptr will handle deletion if not moved

    return false;
}

bool AddonManager::LoadUSBAddon(std::unique_ptr<GPAddon> addon) {
    auto listener = addon->getListener(); // Get listener before moving addon
    if (LoadAddon(std::move(addon))) {
        USBHostManager::getInstance().pushListener(listener);
        return true;
    }
    return false;
}

void AddonManager::ReinitializeAddons() {
    for (const auto& block : addons) {
        block->ptr->reinit();
    }
}

void AddonManager::PreprocessAddons() {
    for (GPAddon* addon : preprocessList) {
        addon->preprocess();
    }
}

void AddonManager::ProcessAddons() {
    for (GPAddon* addon : processList) {
        addon->process();
    }
}

void AddonManager::PostprocessAddons(bool reportSent) {
    for (GPAddon* addon : postprocessList) {
        addon->postprocess(reportSent);
    }
}

// HACK : change this for NeoPicoLED
GPAddon * AddonManager::GetAddon(std::string name) { // hack for NeoPicoLED
    for (const auto& block : addons) {
        if ( block->ptr->name() == name )
            return block->ptr.get();
    }
    return nullptr;
}
