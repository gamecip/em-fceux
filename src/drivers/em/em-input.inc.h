// Legend:
//   KEY(inputId, defaultKeyCode, enumName, description)
// The key codes were taken from:
//   https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/keyCode
// TODO: tsone: mappings for 2nd controller?
KEYS_PRE
// System inputs
KEY(0x0100, 0x152, SYSTEM_RESET, "Reset") // Ctrl+R
KEY(0x0101, 0x009, SYSTEM_THROTTLE, "Throttle") // Tab
KEY(0x0102, 0x050, SYSTEM_PAUSE, "Pause") // P
KEY(0x0103, 0x0DC, SYSTEM_FRAME_ADVANCE, "Advance frame") // Backslash
KEY(0x0104, 0x074, SYSTEM_STATE_SAVE, "Save state") // F5
KEY(0x0105, 0x076, SYSTEM_STATE_LOAD, "Load state") // F7
KEY(0x0106, 0x030, SYSTEM_STATE_SELECT_0, "Select state 0") // 0-9
KEY(0x0107, 0x031, SYSTEM_STATE_SELECT_1, "Select state 1") // ...
KEY(0x0108, 0x032, SYSTEM_STATE_SELECT_2, "Select state 2")
KEY(0x0109, 0x033, SYSTEM_STATE_SELECT_3, "Select state 3")
KEY(0x010A, 0x034, SYSTEM_STATE_SELECT_4, "Select state 4")
KEY(0x010B, 0x035, SYSTEM_STATE_SELECT_5, "Select state 5")
KEY(0x010C, 0x036, SYSTEM_STATE_SELECT_6, "Select state 6")
KEY(0x010D, 0x037, SYSTEM_STATE_SELECT_7, "Select state 7")
KEY(0x010E, 0x038, SYSTEM_STATE_SELECT_8, "Select state 8")
KEY(0x010F, 0x039, SYSTEM_STATE_SELECT_9, "Select state 9")
// Controller 1
KEY(0x0200, 0x046, GAMEPAD0_A, "A (controller 1)") // F
KEY(0x0201, 0x044, GAMEPAD0_B, "B (controller 1)") // D
KEY(0x0202, 0x053, GAMEPAD0_SELECT, "Select (controller 1)") // S
KEY(0x0203, 0x00D, GAMEPAD0_START, "Start (controller 1)") // Enter
KEY(0x0204, 0x026, GAMEPAD0_UP, "Up (controller 1)") // Up arrow
KEY(0x0205, 0x028, GAMEPAD0_DOWN, "Down (controller 1)") // Down arrow
KEY(0x0206, 0x025, GAMEPAD0_LEFT, "Left (controller 1)") // Left arrow
KEY(0x0207, 0x027, GAMEPAD0_RIGHT, "Right (controller 1)") // Right arrow
KEY(0x0208, 0x000, GAMEPAD0_TURBO_A, "Turbo A (controller 1)")
KEY(0x0209, 0x000, GAMEPAD0_TURBO_B, "Turbo B (controller 1)")
// Controller 2
KEY(0x0210, 0x000, GAMEPAD1_A, "A (controller 2)")
KEY(0x0211, 0x000, GAMEPAD1_B, "B (controller 2)")
KEY(0x0212, 0x000, GAMEPAD1_SELECT, "Select (controller 2)")
KEY(0x0213, 0x000, GAMEPAD1_START, "Start (controller 2)")
KEY(0x0214, 0x000, GAMEPAD1_UP, "Up (controller 2)")
KEY(0x0215, 0x000, GAMEPAD1_DOWN, "Down (controller 2)")
KEY(0x0216, 0x000, GAMEPAD1_LEFT, "Left (controller 2)")
KEY(0x0217, 0x000, GAMEPAD1_RIGHT, "Right (controller 2)")
KEY(0x0218, 0x000, GAMEPAD1_TURBO_A, "Turbo A (controller 2)")
KEY(0x0219, 0x000, GAMEPAD1_TURBO_B, "Turbo B (controller 2)")
#if 0
// Controller 3
KEY(0x0220, 0x000, GAMEPAD2_A, "A (controller 3)")
KEY(0x0221, 0x000, GAMEPAD2_B, "B (controller 3)")
KEY(0x0222, 0x000, GAMEPAD2_SELECT, "Select (controller 3)")
KEY(0x0223, 0x000, GAMEPAD2_START, "Start (controller 3)")
KEY(0x0224, 0x000, GAMEPAD2_UP, "Up (controller 3)")
KEY(0x0225, 0x000, GAMEPAD2_DOWN, "Down (controller 3)")
KEY(0x0226, 0x000, GAMEPAD2_LEFT, "Left (controller 3)")
KEY(0x0227, 0x000, GAMEPAD2_RIGHT, "Right (controller 3)")
KEY(0x0228, 0x000, GAMEPAD2_TURBO_A, "Turbo A (controller 3)")
KEY(0x0229, 0x000, GAMEPAD2_TURBO_B, "Turbo B (controller 3)")
// Controller 4
KEY(0x0230, 0x000, GAMEPAD3_A, "A (controller 4)")
KEY(0x0231, 0x000, GAMEPAD3_B, "B (controller 4)")
KEY(0x0232, 0x000, GAMEPAD3_SELECT, "Select (controller 4)")
KEY(0x0233, 0x000, GAMEPAD3_START, "Start (controller 4)")
KEY(0x0234, 0x000, GAMEPAD3_UP, "Up (controller 4)")
KEY(0x0235, 0x000, GAMEPAD3_DOWN, "Down (controller 4)")
KEY(0x0236, 0x000, GAMEPAD3_LEFT, "Left (controller 4)")
KEY(0x0237, 0x000, GAMEPAD3_RIGHT, "Right (controller 4)")
KEY(0x0238, 0x000, GAMEPAD3_TURBO_A, "Turbo A (controller 4)")
KEY(0x0239, 0x000, GAMEPAD3_TURBO_B, "Turbo B (controller 4)")
#endif
KEYS_POST
