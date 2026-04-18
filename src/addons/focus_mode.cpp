#include "addons/focus_mode.h"
#include "storagemanager.h"
#include "hardware/gpio.h"

bool FocusModeAddon::available() {
	const FocusModeOptions& options = Storage::getInstance().getAddonOptions().focusModeOptions;
	return options.enabled && options.buttonLockMask != 0;
}

void FocusModeAddon::setup() {
	const FocusModeOptions& options = Storage::getInstance().getAddonOptions().focusModeOptions;
	buttonLockMask = options.buttonLockMask;
}

void FocusModeAddon::process() {
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	FocusModeOptions& focusModeOptions = Storage::getInstance().getAddonOptions().focusModeOptions;
	// Override	// Enable Focus Mode if the button is pressed
	if (!focusModeOptions.enabled &&
		(gamepad->mapFocusMode.pinMask && (gamepad->debouncedGpio & gamepad->mapFocusMode.pinMask))) {
		focusModeOptions.enabled = true;

		Storage::getInstance().save();
	}
	// If focus mode is enabled, apply button locks
	if (focusModeOptions.enabled) {
		if (buttonLockMask & GAMEPAD_MASK_DU) {
			gamepad->state.dpad &= ~GAMEPAD_MASK_UP;
		}
		if (buttonLockMask & GAMEPAD_MASK_DD) {
			gamepad->state.dpad &= ~GAMEPAD_MASK_DOWN;
		}
		if (buttonLockMask & GAMEPAD_MASK_DL) {
			gamepad->state.dpad &= ~GAMEPAD_MASK_LEFT;
		}
		if (buttonLockMask & GAMEPAD_MASK_DR) {
			gamepad->state.dpad &= ~GAMEPAD_MASK_RIGHT;
		}
		gamepad->state.buttons &= ~buttonLockMask;
	}
}