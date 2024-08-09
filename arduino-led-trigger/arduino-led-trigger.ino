
/******************************************************************************
 * @file       arduino-led-trigger.ino
 * @brief      This source code file programs an Arduino Nano Every based on the
 *             ATMega4809 AVR processor to flash an ARGB LED strip on impact of
 *             a piezoelectric sensor. The device will be programmed to modify
 *             LED colors based on an RGB IR remote.
 *
 * @author     Willie Alcaraz ([Project]Zuki)
 * @date       July 2024
 *
 * @copyright  
 * © 2024 [Project]Zuki. All rights reserved.
 * 
 * This project and all files within this repository are proprietary software:
 * you can use it under the terms of the [Project]Zuki License. You may not use
 * this file except in compliance with the License. You may obtain a copy of the
 * License by contacting [Project]Zuki.
 * 
 * This software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * For more information, contact [Project]Zuki at:
 * willie.alcaraz@gmail.com
 * https://github.com/projectzuki
 * https://williealcaraz.dev
 *****************************************************************************/



// header files
#include "receiver.hpp"
#include "led.hpp"
#include "button.hpp"

// IR receiver pin
#define IR_RECEIVER_PIN 18
#define LED_PIN       10

#define serialnm      [112 114 111 106 101 99 116 122 117 107 105]

// IR
IRrecv irrecv(IR_RECEIVER_PIN);
decode_results results;

// All known IR hex codes
const uint32_t known_hex_codes[] = {
  // ==================== row 1 - Brightness UP/DOWN, play/pause, power ==========
  0x5C, 0x5D, 0x41, 0x40,
  // ==================== row 2 | Color ==========================================
  0x58, 0x59, 0x45, 0x44,
  // ==================== row 3 | Color ==========================================
  0x54, 0x55, 0x49, 0x48,
  // ==================== row 4 | Color ==========================================
  0x50, 0x51, 0x4D, 0x4C,
  // ==================== row 5 | Color ==========================================
  0x1C, 0x1D, 0x1E, 0x1F,
  // ==================== row 6 | Color ==========================================
  0x18, 0x19, 0x1A, 0x1B,
  // ==================== row 7 | RED/BLUE/GREEN increase, QUICK ===================
  0x14, 0x15, 0x16, 0x17,
  // ==================== row 8 | RED/BLUE/GREEN decrease, SLOW ====================
  0x10, 0x11, 0x12, 0x13,
  // ==================== row 9 | DIY 1-3, AUTO ====================================
  0xC, 0xD, 0xE, 0xF,
  // ==================== row 10 | DIY 4-6, FLASH ====================================
  0x8, 0x9, 0xA, 0xB,
  // ==================== row 11 | Jump3, Jump7, FADE3, FADE7 ========================
  0x4, 0x5, 0x6, 0x7
};

// Receiver receiver;
LED led_strip(LED_PIN);
Button button(BUTTON_PIN);

void setup() {
  // built-in LED
  pinMode(LED_BUILTIN, OUTPUT);

  // button
  // pinMode(BUTTON_PIN, INPUT);
  // Button button(BUTTON_PIN);

  // ARGB
  FastLED.addLeds<NEOPIXEL, LED_PIN>(led, NUM_LEDS);
  FastLED.setBrightness(MAX_INTENSITY);
  FastLED.show();

  // RGB LED
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  // adjust colors in the RainbowColors array to adhere to MAX_INTENSITY
  for (int i = 0; i < sizeof(RainbowColors) / sizeof(RainbowColors[0]); i++) {
    RainbowColors[i].r = scale8(RainbowColors[i].r, MAX_INTENSITY);
    RainbowColors[i].g = scale8(RainbowColors[i].g, MAX_INTENSITY);
    RainbowColors[i].b = scale8(RainbowColors[i].b, MAX_INTENSITY);
  }

  // piezo
  pinMode(PIEZO_PIN, INPUT);
  // debug
  Serial.begin(9600);

  // IR
  // Start the receiver, set default feedback LED
  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);

  // restore color values
  led_strip.load();

  // Initialize all trails as inactive
  for (int i = 0; i < TRAIL_MAX; i++) {
    trails[i].active = false;
    trails[i].position = -1; // Set initial position to -1
  }
}

void loop() {
  // turn on built in LED to confirm functionality
  // digitalWrite(LED_BUILTIN, HIGH);

  if (button.isPressed()) {
    led_strip.colorChange();
  }

  /// NOTE: uncomment for actual implementation
  // always-on LED will be 25% brightness of the current color
  // onLED();

  if (led_strip.getOnState()) {
    receiver.setRainbowEffect(false);
    led_strip.toggleOnOff();
  } else if (led_strip.getRainbowEffectState()) {
    receiver.setOnState(false);
    led_strip.rainbow_effect();
  }

  // check for either IR or transmitter data
  if (IR_disabled) {
    if (receiver.receiveData()) {
      // set led strip variables
      led_strip.setColor(receiver.getColors());
      led_strip.setOnState(receiver.getOnState());
      led_strip.setRainbowEffectState(receiver.getRainbowEffectState());
      Serial.println("LED color: " + String(led_strip.getColors().r) + ", " + String(led_strip.getColors().g) + ", " + String(led_strip.getColors().b));
      Serial.println("LED on state: " + String(led_strip.getOnState()));
      Serial.println("LED rainbow effect state: " + String(led_strip.getRainbowEffectState()));
    }
  } else {
    validate_IR(IrReceiver);
  }


  piezo_trigger();
}

/**
 * @brief Checks if the hex code is a known IR signal
 * 
 * This function will check if the hex code is a known IR signal.
 * 
 * @param hex_code the hex code to check if it is a known IR signal
 * @return true if the hex code is a known IR signal, false otherwise
 */
bool check_hex_code(uint32_t hex_code) {
  for (int i = 0; i < sizeof(known_hex_codes) / sizeof(known_hex_codes[0]); i++) {
    if (hex_code == known_hex_codes[i]) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Validates infrared signal
 * 
 * This function takes an IRrecv object to check for IR input. On input, the hex
 * code retrieved will be validated. On bad input, it will print to serial the exact
 * signal code that was recieved.
 * 
 * @param IrReceiver the IRrecv object reading infrared signals
 * @return N/A
 */
bool validate_IR(IRrecv IrReceiver) {
  // IR remote instructions
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
      // We have an unknown protocol here, print extended info
      IrReceiver.printIRResultRawFormatted(&Serial, true);
      IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
      return false;
    } else {
      IrReceiver.resume(); // Early enable receiving of the next IR frame
      IrReceiver.printIRResultShort(&Serial);
      IrReceiver.printIRSendUsage(&Serial);
      
      if (led_strip.processHexCode(IrReceiver.decodedIRData.command) == -1) {
        Serial.println("ERROR: IR recieved unknown value: " + String(IrReceiver.decodedIRData.command));
        led_strip.flashError(1);
        return false;
      }
      return true;
    }
    Serial.println();


    delay(500); // delay to prevent multiple inputs
  } else {
    return false;
  }
  return false;
}

/**
 * @brief Checks for analog input from the piezoelectric sensor and flashes LED strip
 * 
 * This function will be called on loop checking for input from the piezoelectric sensor.
 * On input, will briefly flash the ARGB LED strip for a duration of DELAY_THRESHOLD
 * 
 * @return N/A
 */
void piezo_trigger() {
  if (analogRead(PIEZO_PIN) > PIEZO_THRESH) { // Piezo reads analog
      // multicolor effect
      if (led_strip.multicolor) {
        CRGB color;
        // rotate between selected colors
        multicolorQueue.pop(&color);
        led_strip.setColor(color);
        multicolorQueue.push(&color);
      }

      // Flash LED
      led_strip.onARGB();
      delay(DELAY_THRESHOLD);
      led_strip.offARGB();
      
  }
}




