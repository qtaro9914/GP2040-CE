#include "GamepadState.h"
#include "drivermanager.h"

// SOCD LUT for UP_PRIORITY mode (16 entries for 4-bit dpad)
// Input: [UP|DOWN|LEFT|RIGHT] = bits [0|1|2|3]
// UP+DOWN -> UP, LEFT+RIGHT -> NEUTRAL
// constexpr ensures compile-time evaluation and optimal code generation
static constexpr uint8_t socd_up_priority_lut[16] = {
    0b0000, // 0:  none
    0b0001, // 1:  up
    0b0010, // 2:  down
    0b0001, // 3:  up+down -> up (UP_PRIORITY)
    0b0100, // 4:  left
    0b0101, // 5:  up+left
    0b0110, // 6:  down+left
    0b0101, // 7:  up+down+left -> up+left
    0b1000, // 8:  right
    0b1001, // 9:  up+right
    0b1010, // 10: down+right
    0b1001, // 11: up+down+right -> up+right
    0b0000, // 12: left+right -> neutral
    0b0001, // 13: up+left+right -> up
    0b0010, // 14: down+left+right -> down
    0b0001, // 15: all -> up
};

// SOCD LUT for NEUTRAL mode
// UP+DOWN -> NEUTRAL, LEFT+RIGHT -> NEUTRAL
// constexpr ensures compile-time evaluation and optimal code generation
static constexpr uint8_t socd_neutral_lut[16] = {
    0b0000, // 0:  none
    0b0001, // 1:  up
    0b0010, // 2:  down
    0b0000, // 3:  up+down -> neutral
    0b0100, // 4:  left
    0b0101, // 5:  up+left
    0b0110, // 6:  down+left
    0b0100, // 7:  up+down+left -> left (UD cancelled)
    0b1000, // 8:  right
    0b1001, // 9:  up+right
    0b1010, // 10: down+right
    0b1000, // 11: up+down+right -> right (UD cancelled)
    0b0000, // 12: left+right -> neutral
    0b0001, // 13: up+left+right -> up (LR cancelled)
    0b0010, // 14: down+left+right -> down (LR cancelled)
    0b0000, // 15: all -> neutral
};

// Cached joystick mid value (shared between dpadToAnalogX/Y)
// Value is fetched once on first use, doesn't change at runtime
static uint16_t getCachedJoystickMid() {
	static uint16_t cachedMid = 0;
	static bool cached = false;
	if (!cached) {
		GPDriver* driver = DriverManager::getInstance().getDriver();
		cachedMid = (driver != nullptr) ? driver->GetJoystickMidValue() : GAMEPAD_JOYSTICK_MID;
		cached = true;
	}
	return cachedMid;
}

// Convert the horizontal GamepadState dpad axis value into an analog value
uint16_t dpadToAnalogX(uint8_t dpad)
{
	switch (dpad & (GAMEPAD_MASK_LEFT | GAMEPAD_MASK_RIGHT))
	{
		case GAMEPAD_MASK_LEFT:
			return GAMEPAD_JOYSTICK_MIN;

		case GAMEPAD_MASK_RIGHT:
			return GAMEPAD_JOYSTICK_MAX;

		default:
			return getCachedJoystickMid();
	}
}

// Convert the vertical GamepadState dpad axis value into an analog value
uint16_t dpadToAnalogY(uint8_t dpad)
{
	switch (dpad & (GAMEPAD_MASK_UP | GAMEPAD_MASK_DOWN))
	{
		case GAMEPAD_MASK_UP:
			return GAMEPAD_JOYSTICK_MIN;

		case GAMEPAD_MASK_DOWN:
			return GAMEPAD_JOYSTICK_MAX;

		default:
			return getCachedJoystickMid();
	}
}

uint8_t getMaskFromDirection(DpadDirection direction)
{
	return dpadMasks[direction-1];
}

// LUT for 4-way mode: maps dpad input (4 bits) to single direction output
// Priority order: RIGHT > LEFT > DOWN > UP (higher priority for later directions)
// This matches the original behavior where filterToFourWayMode processed directions
// in order UP, DOWN, LEFT, RIGHT, with the last processed direction winning
static constexpr uint8_t fourway_priority_lut[16] = {
	0,                    // 0b0000: none
	GAMEPAD_MASK_UP,      // 0b0001: up
	GAMEPAD_MASK_DOWN,    // 0b0010: down
	GAMEPAD_MASK_DOWN,    // 0b0011: up+down -> down (down has higher priority)
	GAMEPAD_MASK_LEFT,    // 0b0100: left
	GAMEPAD_MASK_LEFT,    // 0b0101: up+left -> left
	GAMEPAD_MASK_LEFT,    // 0b0110: down+left -> left
	GAMEPAD_MASK_LEFT,    // 0b0111: up+down+left -> left
	GAMEPAD_MASK_RIGHT,   // 0b1000: right
	GAMEPAD_MASK_RIGHT,   // 0b1001: up+right -> right
	GAMEPAD_MASK_RIGHT,   // 0b1010: down+right -> right
	GAMEPAD_MASK_RIGHT,   // 0b1011: up+down+right -> right
	GAMEPAD_MASK_RIGHT,   // 0b1100: left+right -> right
	GAMEPAD_MASK_RIGHT,   // 0b1101: up+left+right -> right
	GAMEPAD_MASK_RIGHT,   // 0b1110: down+left+right -> right
	GAMEPAD_MASK_RIGHT,   // 0b1111: all -> right
};

/**
 * @brief Filter diagonals out of the dpad, making the device work as a 4-way lever.
 *
 * The most recent cardinal direction wins.
 * Optimized LUT-based implementation: no heap allocation, O(1) execution.
 *
 * @param dpad The GameState.dpad value.
 * @return uint8_t The new dpad value (single direction only).
 */
uint8_t __not_in_flash_func(filterToFourWayMode)(uint8_t dpad)
{
	static uint8_t prevDpad = 0;
	static uint8_t currentOutput = 0;

	uint8_t dpadMasked = dpad & 0x0F;

	// Detect newly pressed directions (pressed this frame but not last frame)
	uint8_t newlyPressed = dpadMasked & ~prevDpad;

	if (newlyPressed) {
		// New direction(s) pressed - select highest priority from newly pressed
		currentOutput = fourway_priority_lut[newlyPressed];
	} else if (!(dpadMasked & currentOutput)) {
		// Current output direction was released - select from remaining pressed
		currentOutput = fourway_priority_lut[dpadMasked];
	}
	// else: keep current output (direction still held, no new input)

	prevDpad = dpadMasked;
	return currentOutput;
}

/**
 * @brief Run SOCD cleaning against a D-pad value.
 *
 * Uses LUT for UP_PRIORITY and NEUTRAL modes (O(1) lookup).
 * Falls back to state-based logic for SECOND/FIRST_INPUT_PRIORITY modes.
 *
 * Placed in RAM for fastest execution (avoiding flash access latency).
 *
 * @param mode The SOCD cleaning mode.
 * @param dpad The GamepadState.dpad value.
 * @return uint8_t The clean D-pad value.
 */
uint8_t __not_in_flash_func(runSOCDCleaner)(SOCDMode mode, uint8_t dpad)
{
	// State tracking for SECOND/FIRST_INPUT_PRIORITY modes
	// Declared at function scope so all modes can reset state on mode switch
	static DpadDirection lastUD = DIRECTION_NONE;
	static DpadDirection lastLR = DIRECTION_NONE;

	switch (mode) {
		case SOCD_MODE_BYPASS:
			return dpad;
		case SOCD_MODE_UP_PRIORITY:
			lastUD = DIRECTION_NONE;
			lastLR = DIRECTION_NONE;
			return socd_up_priority_lut[dpad & 0x0F];
		case SOCD_MODE_NEUTRAL:
			lastUD = DIRECTION_NONE;
			lastLR = DIRECTION_NONE;
			return socd_neutral_lut[dpad & 0x0F];
		default:
			break;
	}

	// State-based logic for SECOND_INPUT_PRIORITY and FIRST_INPUT_PRIORITY
	uint8_t newDpad = 0;

	switch (dpad & (GAMEPAD_MASK_UP | GAMEPAD_MASK_DOWN))
	{
		case (GAMEPAD_MASK_UP | GAMEPAD_MASK_DOWN):
			if (mode == SOCD_MODE_SECOND_INPUT_PRIORITY && lastUD != DIRECTION_NONE)
				newDpad |= (lastUD == DIRECTION_UP) ? GAMEPAD_MASK_DOWN : GAMEPAD_MASK_UP;
			else if (mode == SOCD_MODE_FIRST_INPUT_PRIORITY && lastUD != DIRECTION_NONE)
				newDpad |= (lastUD == DIRECTION_UP) ? GAMEPAD_MASK_UP : GAMEPAD_MASK_DOWN;
			else
				lastUD = DIRECTION_NONE;
			break;

		case GAMEPAD_MASK_UP:
			newDpad |= GAMEPAD_MASK_UP;
			lastUD = DIRECTION_UP;
			break;

		case GAMEPAD_MASK_DOWN:
			newDpad |= GAMEPAD_MASK_DOWN;
			lastUD = DIRECTION_DOWN;
			break;

		default:
			lastUD = DIRECTION_NONE;
			break;
	}

	switch (dpad & (GAMEPAD_MASK_LEFT | GAMEPAD_MASK_RIGHT))
	{
		case (GAMEPAD_MASK_LEFT | GAMEPAD_MASK_RIGHT):
			if (mode == SOCD_MODE_SECOND_INPUT_PRIORITY && lastLR != DIRECTION_NONE)
				newDpad |= (lastLR == DIRECTION_LEFT) ? GAMEPAD_MASK_RIGHT : GAMEPAD_MASK_LEFT;
			else if (mode == SOCD_MODE_FIRST_INPUT_PRIORITY && lastLR != DIRECTION_NONE)
				newDpad |= (lastLR == DIRECTION_LEFT) ? GAMEPAD_MASK_LEFT : GAMEPAD_MASK_RIGHT;
			else
				lastLR = DIRECTION_NONE;
			break;

		case GAMEPAD_MASK_LEFT:
			newDpad |= GAMEPAD_MASK_LEFT;
			lastLR = DIRECTION_LEFT;
			break;

		case GAMEPAD_MASK_RIGHT:
			newDpad |= GAMEPAD_MASK_RIGHT;
			lastLR = DIRECTION_RIGHT;
			break;

		default:
			lastLR = DIRECTION_NONE;
			break;
	}

	return newDpad;
}

