#include "addonmanager.h"
#include "usbhostmanager.h"

bool AddonManager::LoadAddon(std::unique_ptr<GPAddon> addon) {
    if (addon->available()) {
        auto block = std::make_unique<AddonBlock>();
        addon->setup();
        block->ptr = std::move(addon);
        addons.push_back(std::move(block));
        return true;
    }
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
    for (const auto& block : addons) {
        block->ptr->preprocess();
    }
}

void AddonManager::ProcessAddons() {
    for (const auto& block : addons) {
        block->ptr->process();
    }
}

void AddonManager::PostprocessAddons(bool reportSent) {
    for (const auto& block : addons) {
        block->ptr->postprocess(reportSent);
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
