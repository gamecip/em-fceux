/*

Listing of all em-fceux inputs.

An input is defined with the following macro:

  INPUT(inputId, defaultKeyBinding, defaultGamepadBinding, enumName, description)
 
Parameter details:

  inputId - The unique ID of the input (enumeration value).
  defaultKeyBinding - Key code of the bound key.
  defaultGamepadBinding - Gamepad binding bitfield (see input.cpp).
  enumName - Name of the input (enumeration name).
  description - Short description of the input (what it does).

The key codes were taken from:
  https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/keyCode

!!!! NOTE: DO NOT CHANGE THE ORDER OF THE INPUTS !!!!

*/
INPUT_PRE
// System inputs
INPUT(0x0100, 0x000, 0x00, SYSTEM_RESET, "Reset") // Ctrl+R
INPUT(0x0101, 0x000, 0x00, SYSTEM_THROTTLE, "Throttle") // Tab
INPUT(0x0102, 0x000, 0x00, SYSTEM_PAUSE, "Pause") // P
INPUT(0x0103, 0x000, 0x00, SYSTEM_FRAME_ADVANCE, "Single Frame Step") // Backslash
INPUT(0x0104, 0x000, 0x00, SYSTEM_STATE_SAVE, "Save State") // F5
INPUT(0x0105, 0x000, 0x00, SYSTEM_STATE_LOAD, "Load State") // F7
// Controller 1
INPUT(0x0200, 0x046, 0x01, GAMEPAD0_A, "A (Controller 1)") // F
INPUT(0x0201, 0x044, 0x11, GAMEPAD0_B, "B (Controller 1)") // D
INPUT(0x0202, 0x053, 0x21, GAMEPAD0_SELECT, "Select (Controller 1)") // S
INPUT(0x0203, 0x00D, 0x31, GAMEPAD0_START, "Start (Controller 1)") // Enter
INPUT(0x0204, 0x026, 0x12, GAMEPAD0_UP, "Up (Controller 1)") // Up arrow
INPUT(0x0205, 0x028, 0x13, GAMEPAD0_DOWN, "Down (Controller 1)") // Down arrow
INPUT(0x0206, 0x025, 0x02, GAMEPAD0_LEFT, "Left (Controller 1)") // Left arrow
INPUT(0x0207, 0x027, 0x03, GAMEPAD0_RIGHT, "Right (Controller 1)") // Right arrow
INPUT(0x0208, 0x000, 0x41, GAMEPAD0_TURBO_A, "Turbo A (Controller 1)")
INPUT(0x0209, 0x000, 0x51, GAMEPAD0_TURBO_B, "Turbo B (Controller 1)")
// Controller 2
INPUT(0x0210, 0x000, 0x00, GAMEPAD1_A, "A (Controller 2)")
INPUT(0x0211, 0x000, 0x00, GAMEPAD1_B, "B (Controller 2)")
INPUT(0x0212, 0x000, 0x00, GAMEPAD1_SELECT, "Select (Controller 2)")
INPUT(0x0213, 0x000, 0x00, GAMEPAD1_START, "Start (Controller 2)")
INPUT(0x0214, 0x000, 0x00, GAMEPAD1_UP, "Up (Controller 2)")
INPUT(0x0215, 0x000, 0x00, GAMEPAD1_DOWN, "Down (Controller 2)")
INPUT(0x0216, 0x000, 0x00, GAMEPAD1_LEFT, "Left (Controller 2)")
INPUT(0x0217, 0x000, 0x00, GAMEPAD1_RIGHT, "Right (Controller 2)")
INPUT(0x0218, 0x000, 0x00, GAMEPAD1_TURBO_A, "Turbo A (Controller 2)")
INPUT(0x0219, 0x000, 0x00, GAMEPAD1_TURBO_B, "Turbo B (Controller 2)")
#if 0
// Controller 3
INPUT(0x0220, 0x000, 0x00, GAMEPAD2_A, "A (Controller 3)")
INPUT(0x0221, 0x000, 0x00, GAMEPAD2_B, "B (Controller 3)")
INPUT(0x0222, 0x000, 0x00, GAMEPAD2_SELECT, "Select (Controller 3)")
INPUT(0x0223, 0x000, 0x00, GAMEPAD2_START, "Start (Controller 3)")
INPUT(0x0224, 0x000, 0x00, GAMEPAD2_UP, "Up (Controller 3)")
INPUT(0x0225, 0x000, 0x00, GAMEPAD2_DOWN, "Down (Controller 3)")
INPUT(0x0226, 0x000, 0x00, GAMEPAD2_LEFT, "Left (Controller 3)")
INPUT(0x0227, 0x000, 0x00, GAMEPAD2_RIGHT, "Right (Controller 3)")
INPUT(0x0228, 0x000, 0x00, GAMEPAD2_TURBO_A, "Turbo A (Controller 3)")
INPUT(0x0229, 0x000, 0x00, GAMEPAD2_TURBO_B, "Turbo B (Controller 3)")
// Controller 4
INPUT(0x0230, 0x000, 0x00, GAMEPAD3_A, "A (Controller 4)")
INPUT(0x0231, 0x000, 0x00, GAMEPAD3_B, "B (Controller 4)")
INPUT(0x0232, 0x000, 0x00, GAMEPAD3_SELECT, "Select (Controller 4)")
INPUT(0x0233, 0x000, 0x00, GAMEPAD3_START, "Start (Controller 4)")
INPUT(0x0234, 0x000, 0x00, GAMEPAD3_UP, "Up (Controller 4)")
INPUT(0x0235, 0x000, 0x00, GAMEPAD3_DOWN, "Down (Controller 4)")
INPUT(0x0236, 0x000, 0x00, GAMEPAD3_LEFT, "Left (Controller 4)")
INPUT(0x0237, 0x000, 0x00, GAMEPAD3_RIGHT, "Right (Controller 4)")
INPUT(0x0238, 0x000, 0x00, GAMEPAD3_TURBO_A, "Turbo A (Controller 4)")
INPUT(0x0239, 0x000, 0x00, GAMEPAD3_TURBO_B, "Turbo B (Controller 4)")
#endif
INPUT_POST
