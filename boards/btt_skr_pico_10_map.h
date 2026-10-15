/*
  btt_skr_pico_10_map.h - driver code for RP2040 ARM processors

  Part of grblHAL

  Copyright (c) 2022-2026 Terje Io

  grblHAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  grblHAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with grblHAL. If not, see <http://www.gnu.org/licenses/>.
*/

#if N_ABC_MOTORS > 1
#error "Axis configuration is not supported!"
#endif

#if WIFI_ENABLE || BLUETOOTH_ENABLE == 1
#error "Wireless networking is not supported!"
#endif

// Heat-output variant (selected at build time, see CMakeLists.txt HOTWIRE_HEAT):
//   default     -> spindle PWM on GP29 (SERVO header, external MOSFET module)
//   HOTWIRE_HEAT_ON_HB -> spindle PWM on GP21 (on-board heated-bed MOSFET)
// The board name carries the variant so `$I` identifies the flashed firmware.
#ifdef HOTWIRE_HEAT_ON_HB
#define BOARD_NAME "SKR Pico v1.0 HotWire4X HB - R.ILTIS"
#else
#define BOARD_NAME "SKR Pico v1.0 HotWire4X SERVOS - R.ILTIS"
#endif
#define BOARD_URL "https://github.com/bigtreetech/SKR-Pico"

#undef DEFAULT_SPINDLE_PWM_FREQ
#define DEFAULT_SPINDLE_PWM_FREQ      200.0f      // $33: 200 Hz pour fil chaud

// Foam-cutter / hotwire SERVO variant of the BTT SKR Pico 1.0:
//   * Spindle PWM is routed to the SERVO header (GPIO 29) so it can drive an
//     external opto-isolated MOSFET module that switches the hotwire load.
//     See AUXOUTPUT0_PIN below.
//   * The HotWire4X STOP button is the safety-door input (no E-stop input
//     is compiled; a real emergency stop cuts the supplies). It lands on
//     GPIO 22, exposed
//     directly on the PROBE connector via the on-board P_S/IO22/PROBE
//     3-pin strap (must be set on the PROBE side, not on the P_S/WD-DET
//     side). Wiring is a momentary NO push-button between PROBE and GND,
//     identical to the limit switches; the RP2040 internal pull-up holds
//     the line high at rest. The opto-coupled WD-DET path (P_S pin) is
//     unsuitable here because it expects a powered sensor signal, not a
//     passive switch to GND - a momentary contact never drives the opto
//     LED, so the input would never trigger.
//   * Probe is disabled (foam cutting does not probe); this frees GPIO 22
//     from the default AUXINPUT0 probe assignment for the STOP button.
#undef  PROBE_ENABLE
#define PROBE_ENABLE                0

#define SERIAL1_PORT 1

#undef TRINAMIC_ENABLE
#undef TRINAMIC_UART_ENABLE
#define TRINAMIC_ENABLE 2209
#define TRINAMIC_UART_ENABLE 1
#define TRINAMIC_STREAM 1
#undef TRINAMIC_MIXED_DRIVERS
#define TRINAMIC_MIXED_DRIVERS 0
#define HAS_BOARD_INIT

// Define step pulse output pins.
#define STEP_PORT                   GPIO_PIO_1 // Single pin PIO SM
#define X_STEP_PIN                  11
#define Y_STEP_PIN                  6
#define Z_STEP_PIN                  19

// Define step direction output pins.
#define DIRECTION_PORT              GPIO_OUTPUT
#define DIRECTION_OUTMODE           GPIO_MAP
#define X_DIRECTION_PIN             10
#define Y_DIRECTION_PIN             5
#define Z_DIRECTION_PIN             28

// Define stepper driver enable/disable output pin.
#define ENABLE_PORT                 GPIO_OUTPUT
#define X_ENABLE_PIN                12
#define Y_ENABLE_PIN                7
#define Z_ENABLE_PIN                2

// Define ganged axis or A axis step pulse and step direction output pins.
#if N_ABC_MOTORS > 0
#define M3_AVAILABLE
#define M3_STEP_PIN                 14
#define M3_DIRECTION_PIN            13
#define M3_ENABLE_PIN               15
#if N_AXIS > 3 || N_AUTO_SQUARED
#define M3_LIMIT_PIN                16
#endif
#endif

// Define homing/hard limit switch input pins.
#define X_LIMIT_PIN                 4
#define Y_LIMIT_PIN                 3
#define Z_LIMIT_PIN                 25

// Hotwire output wiring (GPIO 29, SERVO header):
// Connect this pin directly to the IN terminal of an opto-isolated MOSFET
// module - common 3.3-24V trigger modules already integrate the input
// limiting resistor sized for that voltage range. DO NOT add a series
// resistor: the RP2040 drives 3.3V CMOS levels, and an extra series
// resistor would drop the voltage below the opto-coupler's forward
// threshold and the module would fail to switch. Tie the module's GND to
// the SKR Pico GND. Power the hotwire load (48V) on the module's load side.
#define AUXOUTPUT0_PORT             GPIO_OUTPUT // Spindle PWM (hotwire heat)
#ifdef HOTWIRE_HEAT_ON_HB
#define AUXOUTPUT0_PIN              21 // HB header -> on-board heated-bed MOSFET (12-24V short wire, no external module)
#else
#define AUXOUTPUT0_PIN              29 // SERVO header (3.3V) -> external opto-isolated MOSFET module (wire supply >24V)
#endif
#define AUXOUTPUT1_PORT             GPIO_OUTPUT // Spindle direction
#define AUXOUTPUT1_PIN              18 // FAN2 PWM
#define AUXOUTPUT2_PORT             GPIO_OUTPUT // Spindle enable
#define AUXOUTPUT2_PIN              17 // FAN1 PWM
#define AUXOUTPUT3_PORT             GPIO_OUTPUT // Coolant flood
#define AUXOUTPUT3_PIN              20 // FAN3 PWM, Laser PWM
#define AUXOUTPUT4_PORT             GPIO_OUTPUT // Coolant mist
#define AUXOUTPUT4_PIN              23 // HE PWM

// Define driver spindle pins

#if DRIVER_SPINDLE_ENABLE
#define SPINDLE_PORT                GPIO_OUTPUT
#endif
#if DRIVER_SPINDLE_ENABLE & SPINDLE_ENA
#define SPINDLE_ENABLE_PIN          AUXOUTPUT2_PIN 
#endif
#if DRIVER_SPINDLE_ENABLE & SPINDLE_PWM
#define SPINDLE_PWM_PIN             AUXOUTPUT0_PIN
#endif
#if DRIVER_SPINDLE_ENABLE & SPINDLE_DIR
#define SPINDLE_DIRECTION_PIN       AUXOUTPUT1_PIN
#endif

// Define flood and mist coolant enable output pins.
#if COOLANT_ENABLE
#define COOLANT_PORT                GPIO_OUTPUT
#endif
#if COOLANT_ENABLE & COOLANT_FLOOD
#define COOLANT_FLOOD_PIN           AUXOUTPUT3_PIN
#endif
#if COOLANT_ENABLE & COOLANT_MIST
#define COOLANT_MIST_PIN            AUXOUTPUT4_PIN
#endif

// HotWire4X STOP button on GPIO 22 - the SAFETY DOOR input, not an E-stop
// (HotWire4X docs/stop_button.md): it stops the motion with a controlled
// deceleration, turns the heat off once stopped and keeps the motors
// energized ($1); the sender then cancels the rest. A real emergency stop
// cuts the motor and heat supplies. No reset / E-stop input is compiled.
// Exposed on the PROBE connector via the on-board P_S/IO22/PROBE strap (must
// be set on the PROBE side - see board header). Probe is disabled above to
// free this pin. GPIO 29 (SERVO) is taken by AUXOUTPUT0 (spindle PWM).
// SAFETY_DOOR_ENABLE is set in my_machine.h.
#ifndef M3_LIMIT_PIN
#define AUXINPUT1_PIN               16 // E0 limit
#else
#if SAFETY_DOOR_ENABLE
#define AUXINPUT1_PIN               22 // PROBE connector (raw GPIO via P_S/IO22/PROBE strap) - STOP button
#endif
#endif
#if SAFETY_DOOR_ENABLE && defined(AUXINPUT1_PIN)
#define SAFETY_DOOR_PIN             AUXINPUT1_PIN
#endif

// Cycle Start (Resume) on GP1 / Feed Hold on GP0 - TFT serial header (UART0,
// unused since USB is primary). GP1 chosen for Cycle Start because it is the
// pin adjacent to GND on the 5-pin TFT connector. Direct 3.3V GPIOs with no
// isolation - same wiring philosophy as the STOP on PROBE: momentary NO
// push-buttons to GND, internal pull-up keeps the line high at rest.
#if CONTROL_ENABLE & CONTROL_HALT
#define AUXINPUT2_PIN               1
#define AUXINPUT3_PIN               0
#undef CONTROL_ENABLE
#define CONTROL_ENABLE              (CONTROL_CYCLE_START|CONTROL_FEED_HOLD)
#define CYCLE_START_PIN             AUXINPUT2_PIN
#define FEED_HOLD_PIN               AUXINPUT3_PIN
#endif

#if PROBE_ENABLE
#define PROBE_PIN                   AUXINPUT0_PIN
#endif

#if RGB_LED_ENABLE
#define NEOPIXELS_PIN               24
#endif

#if MODBUS_ENABLE
#define MODBUS_RTU_STREAM           0
#endif
