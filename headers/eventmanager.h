#ifndef _EVENTMANAGER_H_
#define _EVENTMANAGER_H_

#include <map>
#include <vector>
#include <string>
#include <deque>
#include <array>
#include <functional>
#include <cctype>
#include "config.pb.h"
#include "enums.pb.h"

#include "GPEvent.h"
#include "GPGamepadEvent.h"
#include "GPEncoderEvent.h"
#include "GPMenuNavigateEvent.h"
#include "GPProfileEvent.h"
#include "GPRestartEvent.h"
#include "GPStorageSaveEvent.h"
#include "GPSystemErrorEvent.h"
#include "GPSystemRebootEvent.h"
#include "GPUSBHostEvent.h"

#define EVENTMGR EventManager::getInstance()

class EventManager {
    public:
        typedef std::function<void(GPEvent* event)> EventFunction;

        EventManager(EventManager const&) = delete;
        void operator=(EventManager const&)  = delete;
        static EventManager& getInstance()
        {
            static EventManager instance;
            return instance;
        }

        void init();
        void clearEventHandlers();

        void registerEventHandler(GPEventType eventType, EventFunction handler);
        void unregisterEventHandler(GPEventType eventType, EventFunction handler);
        void triggerEvent(GPEvent& event);
        void triggerEvent(GPEvent* event) { triggerEvent(*event); delete event; }
    private:
        EventManager(){}

        std::array<std::vector<EventFunction>, _GPEventType_ARRAYSIZE> eventHandlers;
};

#endif