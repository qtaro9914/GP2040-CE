#include "eventmanager.h"
#include "storagemanager.h"
#include "enums.pb.h"

void EventManager::init() {
    clearEventHandlers();
}

void EventManager::registerEventHandler(GPEventType eventType, EventFunction handler) {
    eventHandlers[eventType].push_back(handler);
}

void EventManager::unregisterEventHandler(GPEventType eventType, EventFunction handler) {
    auto& handlers = eventHandlers[eventType];
    for (auto funcIt = handlers.begin(); funcIt != handlers.end(); ++funcIt) {
        if (*(uint32_t*)(uint8_t*)&handler == *(uint32_t*)(uint8_t*)&(*funcIt)) {
            handlers.erase(funcIt);
            break;
        }
    }
}

void EventManager::triggerEvent(GPEvent& event) {
    for (const auto& handler : eventHandlers[event.eventType()]) {
        handler(&event);
    }
}

void EventManager::clearEventHandlers() {

}
