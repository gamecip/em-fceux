// List of em-fceux inputs.
// Legend:
//   INPUT(inputId, defaultKeyCode, enumName, description)
// The key codes were taken from:
//   https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/keyCode
// NOTE: DO NOT CHANGE THE ORDER!
INPUT_PRE
// System inputs
INPUT(0x0100, 0x152, SYSTEM_RESET, "Reset") // Ctrl+R
INPUT(0x0101, 0x009, SYSTEM_THROTTLE, "Throttle") // Tab
INPUT(0x0102, 0x050, SYSTEM_PAUSE, "Pause") // P
INPUT(0x0103, 0x0DC, SYSTEM_FRAME_ADVANCE, "Single Frame Step") // Backslash
INPUT(0x0104, 0x074, SYSTEM_STATE_SAVE, "Save State") // F5
INPUT(0x0105, 0x076, SYSTEM_STATE_LOAD, "Load State") // F7
INPUT(0x0106, 0x030, SYSTEM_STATE_SELECT_0, "Select State 0") // 0-9
INPUT(0x0107, 0x031, SYSTEM_STATE_SELECT_1, "Select State 1") // ...
INPUT(0x0108, 0x032, SYSTEM_STATE_SELECT_2, "Select State 2")
INPUT(0x0109, 0x033, SYSTEM_STATE_SELECT_3, "Select State 3")
INPUT(0x010A, 0x034, SYSTEM_STATE_SELECT_4, "Select State 4")
INPUT(0x010B, 0x035, SYSTEM_STATE_SELECT_5, "Select State 5")
INPUT(0x010C, 0x036, SYSTEM_STATE_SELECT_6, "Select State 6")
INPUT(0x010D, 0x037, SYSTEM_STATE_SELECT_7, "Select State 7")
INPUT(0x010E, 0x038, SYSTEM_STATE_SELECT_8, "Select State 8")
INPUT(0x010F, 0x039, SYSTEM_STATE_SELECT_9, "Select State 9")
// Controller 1
INPUT(0x0200, 0x046, GAMEPAD0_A, "A (Controller 1)") // F
INPUT(0x0201, 0x044, GAMEPAD0_B, "B (Controller 1)") // D
INPUT(0x0202, 0x053, GAMEPAD0_SELECT, "Select (Controller 1)") // S
INPUT(0x0203, 0x00D, GAMEPAD0_START, "Start (Controller 1)") // Enter
INPUT(0x0204, 0x026, GAMEPAD0_UP, "Up (Controller 1)") // Up arrow
INPUT(0x0205, 0x028, GAMEPAD0_DOWN, "Down (Controller 1)") // Down arrow
INPUT(0x0206, 0x025, GAMEPAD0_LEFT, "Left (Controller 1)") // Left arrow
INPUT(0x0207, 0x027, GAMEPAD0_RIGHT, "Right (Controller 1)") // Right arrow
INPUT(0x0208, 0x000, GAMEPAD0_TURBO_A, "Turbo A (Controller 1)")
INPUT(0x0209, 0x000, GAMEPAD0_TURBO_B, "Turbo B (Controller 1)")
// Controller 2
INPUT(0x0210, 0x000, GAMEPAD1_A, "A (Controller 2)")
INPUT(0x0211, 0x000, GAMEPAD1_B, "B (Controller 2)")
INPUT(0x0212, 0x000, GAMEPAD1_SELECT, "Select (Controller 2)")
INPUT(0x0213, 0x000, GAMEPAD1_START, "Start (Controller 2)")
INPUT(0x0214, 0x000, GAMEPAD1_UP, "Up (Controller 2)")
INPUT(0x0215, 0x000, GAMEPAD1_DOWN, "Down (Controller 2)")
INPUT(0x0216, 0x000, GAMEPAD1_LEFT, "Left (Controller 2)")
INPUT(0x0217, 0x000, GAMEPAD1_RIGHT, "Right (Controller 2)")
INPUT(0x0218, 0x000, GAMEPAD1_TURBO_A, "Turbo A (Controller 2)")
INPUT(0x0219, 0x000, GAMEPAD1_TURBO_B, "Turbo B (Controller 2)")
#if 0
// Controller 3
INPUT(0x0220, 0x000, GAMEPAD2_A, "A (Controller 3)")
INPUT(0x0221, 0x000, GAMEPAD2_B, "B (Controller 3)")
INPUT(0x0222, 0x000, GAMEPAD2_SELECT, "Select (Controller 3)")
INPUT(0x0223, 0x000, GAMEPAD2_START, "Start (Controller 3)")
INPUT(0x0224, 0x000, GAMEPAD2_UP, "Up (Controller 3)")
INPUT(0x0225, 0x000, GAMEPAD2_DOWN, "Down (Controller 3)")
INPUT(0x0226, 0x000, GAMEPAD2_LEFT, "Left (Controller 3)")
INPUT(0x0227, 0x000, GAMEPAD2_RIGHT, "Right (Controller 3)")
INPUT(0x0228, 0x000, GAMEPAD2_TURBO_A, "Turbo A (Controller 3)")
INPUT(0x0229, 0x000, GAMEPAD2_TURBO_B, "Turbo B (Controller 3)")
// Controller 4
INPUT(0x0230, 0x000, GAMEPAD3_A, "A (Controller 4)")
INPUT(0x0231, 0x000, GAMEPAD3_B, "B (Controller 4)")
INPUT(0x0232, 0x000, GAMEPAD3_SELECT, "Select (Controller 4)")
INPUT(0x0233, 0x000, GAMEPAD3_START, "Start (Controller 4)")
INPUT(0x0234, 0x000, GAMEPAD3_UP, "Up (Controller 4)")
INPUT(0x0235, 0x000, GAMEPAD3_DOWN, "Down (Controller 4)")
INPUT(0x0236, 0x000, GAMEPAD3_LEFT, "Left (Controller 4)")
INPUT(0x0237, 0x000, GAMEPAD3_RIGHT, "Right (Controller 4)")
INPUT(0x0238, 0x000, GAMEPAD3_TURBO_A, "Turbo A (Controller 4)")
INPUT(0x0239, 0x000, GAMEPAD3_TURBO_B, "Turbo B (Controller 4)")
#endif
INPUT_POST
