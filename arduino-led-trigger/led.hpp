
#ifndef LED_HPP
#define LED_HPP

#include <IRremote.h>         // IR remote
#include <FastLED.h>          // NeoPixel ARGB
#include <cppQueue.h>         // queue for RGB color states
#include <EEPROM.h>           // save ROM data durong off state

#include "receiver.hpp"       // HC-12 module

// ARGB pin
#define NUM_LEDS      144
#define MAX_INTENSITY 32    // 255 / 128 / 64 / 32 / 16 / 8
CRGB led[NUM_LEDS];

#define LED_RED       5
#define LED_GREEN     6
#define LED_BLUE      9

// piezo pin
#define PIEZO_PIN     A0
unsigned int PIEZO_THRESH = 300;

// modifier tied to PWR button
bool modifier = false;

// delay threshold for flash duration
int DELAY_THRESHOLD = 100;

// EEPROM addresses
#define RED_ADDR      0
#define GREEN_ADDR    1
#define BLUE_ADDR     2
#define RAINBOW_ADDR  3

// Color array for rainbow effect
bool rainbow = false;
int color_index = 0;
CRGB RainbowColors[] = {
  CRGB::Red,
  CRGB::Orange,
  CRGB::Yellow,
  CRGB::Green,
  CRGB::Blue,
  CRGB::Indigo,
  CRGB::Violet
};

// For trail ripple effect
const int TRAIL_LENGTH = 25;
const int TRAIL_MAX = 10;  // Maximum number of simultaneous trails

struct Trail {
  int position;
  bool active;
  CRGB color;
};

Trail trails[TRAIL_MAX];

cppQueue CRGBQueue(sizeof(CRGB), 5);  // queue for custom colors
cppQueue multicolorQueue(sizeof(CRGB), 5);  // queue for multicolor effect

Receiver receiver;

bool IR_disabled = false;

class LED {
  public:
    bool multicolor;

    LED(int pin) {
      this->pin = pin;
      pinMode(pin, OUTPUT);
    }

    /**
     * @brief Process IR hex code
     * 
     * This function will process the IR hex code recieved from the IR remote, setting
     *  the appropriate colors for RED, GREEN, BLUE according to the hex code from the IR remote.
     * 
     * @param IRvalue the hex code recieved from the IR remote
     * @return -1 if the IR hex code is invalid
     */
    int processHexCode(int IRvalue) {
      /*
      * process codes
      */
      switch(IRvalue) {
        // ==================== row 1 - Brightness UP/DOWN, play/pause, power ==========

        // increase brightness
        case 0x5C:
          FastLED.setBrightness(constrain(FastLED.getBrightness() +20, 1, 255));
          break;
        // decrease brightness
        case 0x5D:
          FastLED.setBrightness(constrain(FastLED.getBrightness() -20, 1, 255));
          break;
        // play/pause
        case 0x41:
          // reverse lit status
          toggleOnOff();
          break;
        // PWR
        case 0x40:
          if (!modifier) {
            modifier = true;    // trigger alt modifier for next input
            led[0] = CRGB(0, 0, MAX_INTENSITY);
            FastLED.show();
            Serial.println("Modifier ON");
            return;
          } else {
            IR_disabled = true;
            IrReceiver.disableIRIn();
            Serial.println("IR disabled");
          }
          break;

        // ==================== row 2 | Color ==========================================
        case 0x58:
          setColor(CRGB::Red);
          break;
        case 0x59:
          setColor(CRGB::Green);
          break;
        case 0x45:
          setColor(CRGB::Blue);
          break;
        case 0x44:
          setColor(CRGB::White);
          break;

        // ==================== row 3 | Color ==========================================
        case 0x54:
          setColor(CRGB::Orange);
          break;
        case 0x55:
          setColor(CRGB::LawnGreen);
          break;
        case 0x49:
          setColor(CRGB::Aqua);
          break;

        case 0x48:
          setColor(CRGB::DeepPink);
          break;

        // ==================== row 4 | Color ==========================================
        case 0x50:
          setColor(CRGB::Gold);
          break;
        case 0x51:
          setColor(CRGB::Cyan);
          break;
        case 0x4D:
          setColor(CRGB::DarkViolet);
          break;
        case 0x4C:
          setColor(CRGB::Coral);
          break;

        // ==================== row 5 | Color ==========================================
        case 0x1C:
          setColor(CRGB::DarkGoldenrod);
          break;
        case 0x1D:
          setColor(CRGB::DarkCyan);
          break;
        case 0x1E:
          setColor(CRGB::Magenta);
          break;
        case 0x1F:
          setColor(CRGB::PowderBlue);
          break;

        // ==================== row 6 | Color ==========================================
        case 0x18:
          setColor(CRGB::Yellow);
          break;
        case 0x19:
          setColor(CRGB::DarkTurquoise);
          break;
        case 0x1A:
          setColor(CRGB::DeepPink);
          break;
        case 0x1B:
          setColor(CRGB::LightSteelBlue);
          break;

        // ==================== row 7 | RED/BLUE/GREEN increase, QUICK ===================

        case 0x14:
          adj_color(red, MAX_INTENSITY/5);
          break;
        case 0x15:
          adj_color(green, MAX_INTENSITY/5);
          break;
        case 0x16:
          adj_color(blue, MAX_INTENSITY/5);
          break;
        // QUICK | Sensitivity down
        case 0x17:
        {
          if (!modifier) {
            // increase sensitivity
            PIEZO_THRESH -= 50;
            if (PIEZO_THRESH <= 0 || PIEZO_THRESH >= 1023) {  // unsigned int < 0 will become 65535
              PIEZO_THRESH = 10;
            }
          } else {
            modifier = false;
            // decrease delay (quicker flash)
            DELAY_THRESHOLD -= 50;
            led[0] = CRGB(0, 0, 0);
            FastLED.show();
          }
          break;
        }
        // ==================== row 8 | RED/BLUE/GREEN decrease, SLOW ====================

        case 0x10:
          adj_color(red, MAX_INTENSITY/-5);
          break;
        case 0x11:
          adj_color(green, MAX_INTENSITY/-5);
          break;
        case 0x12:
          adj_color(blue, MAX_INTENSITY/-5);
          break;
        // SLOW | Sensitivity up
        case 0x13:
        {
          if (!modifier) {
            // decrease sensitivity
            PIEZO_THRESH += 50;
            if (PIEZO_THRESH >= 1023) {
              PIEZO_THRESH = constrain(PIEZO_THRESH, 0, 1023);
            }
          } else {
            modifier = false;
            // increase delay (slower flash)
            DELAY_THRESHOLD += 50;
            led[0] = CRGB(0, 0, 0);
            FastLED.show();
          }
          break;
        }
        // ==================== row 9 | DIY 1-3, AUTO ====================================


        // DIY1
        case 0xC:
        {
          // loop until IR signal is received
          while (true) {
            // continue checking for valid IR signal
            if (IrReceiver.decode()) {
              if (processHexCode(IrReceiver.decodedIRData.command) != -1) {
                break;
              }
              IrReceiver.resume();    // resume IR input
            }
            ripple();
          }
          break;
        }
        // DIY2
        case 0xD:
        {
          // while (true) {
          //   // continue checking for valid IR signal
          //   if (IrReceiver.decode()) {
          //     if (processHexCode(IrReceiver.decodedIRData.command) != -1) {
          //       break;
          //     }
          //     IrReceiver.resume();    // resume IR input
          //   }
          //   ripple2();
          // }

          // custom multicolor
          pushback(multicolorQueue, red, green, blue);
          break;
        }
        //DIY3
        case 0xE:
          // add to color queue
          pushback(CRGBQueue, red, green, blue);
          // // check current color queue
          // check_colorQueue();
          break;
        // AUTO(save) | IR lock
        case 0xF:
        {
        //   save(red, green, blue);    // save current color
          EEPROM.write(RAINBOW_ADDR, rainbow);
          flashConfirm();                   // flash to confirm save
          break;
        }
        // ==================== row 10 | DIY 4-6, FLASH ====================================

        // DIY4
        case 0x8:
          rainbow_effect();
          break;
        // DIY5
        case 0x9:
          multicolor = !multicolor;
          if (multicolor) {
            check_colorQueue(multicolorQueue);
          }
          break;
        // DIY6
        case 0xA:
          // check current color queue
          check_colorQueue(CRGBQueue);
          break;
        // FLASH
        case 0xB:
          // modify the type of flash
          break;

        // ==================== row 11 | Jump3, Jump7, FADE3, FADE7 ========================

        // JUMP3
        case 0x4:
          // Rainbow color effect
          rainbow = true;
          return;   // return early to prevent color change
        // JUMP7
        case 0x5:
          // other rainbow effect
          break;
        // FADE3
        case 0x6:
          break;
        // FADE7
        case 0x7:
          break;
        
        // Default print error for debug
        default:
          Serial.println("ERROR: IR recieved unknown value: " + String(IRvalue));
          flashError(2);
          return -1;
      }

      rainbow = false;
      modifier = false;
      fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
      FastLED.show();
      return IRvalue;
    }

    /**
     * @brief Sets the active LED to the current color value
     * 
     * This function will set the built-in LED to the color value.
     * 
     * @return N/A
     */
    void onLED() {
      // built-in LED
      digitalWrite(LED_RED, red);
      digitalWrite(LED_GREEN, green);
      digitalWrite(LED_BLUE, blue);
    }

    /**
     * @brief Turn off active LED
     * 
     * This function turns off the active LED
     * 
     * @return N/A
     */
    void offLED() {
      // built-in LED
      digitalWrite(LED_RED, 0);
      digitalWrite(LED_GREEN, 0);
      digitalWrite(LED_BLUE, 0);
    }

    /**
     * @brief Turn on LEDs
     * 
     * This function will activate the ARGB stip with the current color setting for
     *  RED, GREEN, BLUE
     * 
     * @return N/A
     */
    void onARGB() {
      // do the thing but ARGB
      fill_solid(led, NUM_LEDS, rainbow? RainbowColors[(color_index++) % sizeof(RainbowColors)] : CRGB(red, green, blue));

      FastLED.show();
    }

    /**
     * @brief Turn off LEDs
     * 
     * This function turns off the ARGB strip
     * 
     * @return N/A
     */
    void offARGB() {
      // do the off thing
      fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
      FastLED.show();
      Serial.println("turning LED off");
    }

    /**
     * @brief Toggle on/off for play/pause button
     * 
     * This function continously activates the ARGB strip until the play/pause button
     * is pressed again to deactivate the strip.
     * 
     * @return N/A
     */
    void toggleOnOff() {
      bool ledon = true;
      // toggle on/off for play/pause button
      onARGB();
      while (ledon) {
        if (IrReceiver.decode()) {
          if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
            Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
            // We have an unknown protocol here, print extended info
            IrReceiver.printIRResultRawFormatted(&Serial, true);
            IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
          } else {
            IrReceiver.resume(); // Early enable receiving of the next IR frame
            IrReceiver.printIRResultShort(&Serial);
            IrReceiver.printIRSendUsage(&Serial);
          }
          Serial.println();

          if (IrReceiver.decodedIRData.command == 0x41) {
            ledon = false;
            offARGB();
            offLED();
            break;
          } else {
            // apply modifications to color
            processHexCode(IrReceiver.decodedIRData.command);
          }
          // update color in case of change
          onARGB();
          onLED();
          delay(200);  // delay to reduce multiple inputs
          IrReceiver.resume();
        } else if (receiver.receiveData()) {
          ledon = receiver.getOnState();
          if (!ledon) {
            // reset
            offARGB();
            offLED();
          } else {
            // apply modifications to color
            setColor(receiver.getColors());
            onARGB();
            onLED();
          }
        }
      }
    }

    /**
     * @brief multicolor effect: Change color based on next in queue
     * 
     * This function will change the color based on the queue, then push the color back into the queue
     * 
     * @return N/A
     */
    void colorChange() {
      // return if empty, else pop color from queue
      if (CRGBQueue.isEmpty()) {
        return;
      }
      CRGB color;
      CRGBQueue.pop(&color);
      red = color.r;
      green = color.g;
      blue = color.b;

      // Serial.println("Color from queue: " + String(RED) + ", " + String(GREEN) + ", " + String(BLUE));
      delay(200);  // delay to prevent multiple pops
    }

    /**
     * @brief LOad color from EEPROM
     * 
     * This function will save the current color to EEPROM.
     * 
     * @return N/A
     */
    void load() {
      // read from EEPROM
      red = EEPROM.read(RED_ADDR);
      green = EEPROM.read(GREEN_ADDR);
      blue = EEPROM.read(BLUE_ADDR);
      rainbow = EEPROM.read(RAINBOW_ADDR);
    }

    /**
     * @brief Save color to EEPROM
     * 
     * This function will save the current color to EEPROM.
     * 
     * @return N/A
     */
    void save(uint8_t red, uint8_t green, uint8_t blue) {
      // write to EEPROM
      EEPROM.write(RED_ADDR, red);
      EEPROM.write(GREEN_ADDR, green);
      EEPROM.write(BLUE_ADDR, blue); 
    }

    /**
     * @brief Sets the values for RED, GREEN, BLUE
     * 
     * This function will set the values for RED, GREEN, BLUE to the CRGB color according
     *  to the input color.
     * 
     * @param color the CRGB color to set the values for RED, GREEN, BLUE
     * @return N/A
     */
    void setColor(CRGB color) {
      // set new RGB values, constrain to max intensity value
      red = scale8(color.r, MAX_INTENSITY);
      green = scale8(color.g, MAX_INTENSITY);
      blue = scale8(color.b, MAX_INTENSITY);
    }

    /**
     * @brief Adjusts the color value
     * 
     * This function will adjust the color value based on the scale factor provided.
     * 
     * @param color the color value to adjust
     * @param scale the scale factor to adjust the color value
     * 
     * @return N/A
     */
    void adj_color(uint8_t color, int scale) {
      // Adjust color value
      int newColor = color + scale;

      // Constrain new color value to be within 1 and MAX_INTENSITY
      newColor = constrain(newColor, 0, MAX_INTENSITY);

      // Set the adjusted color value
      color = newColor;

      // // Debug
      // Serial.println("Adjusted color: " + String(color));
      // Serial.println("Colors: " + String(RED) + ", " + String(GREEN) + ", " + String(BLUE));
    }

    /**
     * @brief Creates a ripple effect on impact
     * 
     * This function will create a ripple effect on the ARGB LED strip each time the
     *  piezo sensor is hit.
     * 
     * @return N/A
     */
    void ripple() {
      // Read the piezo value
      int piezoValue = analogRead(PIEZO_PIN);

      if (piezoValue > PIEZO_THRESH) {
        // Add a new trail if there is room
        for (int i = 0; i < TRAIL_MAX; i++) {
          if (!trails[i].active) {
            trails[i].position = 0;  // Initialize new trail position at the beginning
            trails[i].active = true;
            trails[i].color = rainbow ? RainbowColors[color_index] : CRGB(red, green, blue);
            if (rainbow) {
              color_index = (color_index + 1) % (sizeof(RainbowColors) / sizeof(RainbowColors[0]));
            }
            break;
          }
        }
      }

      // Clear the LED array for each frame
      fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
      
      // Update and display the trails
      for (int t = 0; t < TRAIL_MAX; t++) {
        if (trails[t].active) {
          // Draw the current trail with a gap
          for (int j = 0; j < TRAIL_LENGTH; j++) {
            int pos = trails[t].position - j;
            if (pos >= 0 && pos < NUM_LEDS) {
              led[pos] = trails[t].color;
            }
          }

          // Clear the LED just before the trail to create a gap
          int gapPos = trails[t].position - TRAIL_LENGTH;
          if (gapPos >= 0 && gapPos < NUM_LEDS) {
            led[gapPos] = CRGB(0, 0, 0);
          }
          
          // Update the position for the next frame
          trails[t].position++;

          // Deactivate the trail if it has moved past the LED strip
          if (trails[t].position >= NUM_LEDS + TRAIL_LENGTH + 1) { // Add 1 for the gap
            trails[t].active = false;
            trails[t].position = -1; // Reset position
          }
        }
      }

      FastLED.show();
      delay(1); // Adjust the delay for the speed of the ripple
    }

    /**
     * @brief Creates a ripple effect without requiring impact
     * 
     * This function will create a ripple effect on the ARGB LED strip without requiring
     *  input from the piezo sensor.
     * 
     * @return N/A
     */
    void ripple2() {
      // Same ripple trail without the need of piezo trigger

      // Add a new trail if there is room
      for (int i = 0; i < TRAIL_MAX; i++) {
        if (!trails[i].active) {
          trails[i].position = 0;  // Initialize new trail position at the beginning
          trails[i].active = true;
          trails[i].color = rainbow ? RainbowColors[color_index] : CRGB(red, green, blue);
          if (rainbow) {
            color_index = (color_index + 1) % (sizeof(RainbowColors) / sizeof(RainbowColors[0]));
          }
          break;
        }
      }

      // Clear the LED array for each frame
      fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
      
      // Update and display the trails
      for (int t = 0; t < TRAIL_MAX; t++) {
        if (trails[t].active) {
          // Draw the current trail with a gap
          for (int j = 0; j < TRAIL_LENGTH; j++) {
            int pos = trails[t].position - j;
            if (pos >= 0 && pos < NUM_LEDS) {
              led[pos] = trails[t].color;
            }
          }
          
          // Update the position for the next frame
          trails[t].position++;

          // Deactivate the trail if it has moved past the LED strip
          if (trails[t].position >= NUM_LEDS + TRAIL_LENGTH + 1) { // Add 1 for the gap
            trails[t].active = false;
            trails[t].position = -1; // Reset position
          }
        }
      }

      FastLED.show();
      delay(1); // Adjust the delay for the speed of the ripple
    }

    /**
     * @brief Creates a rainbow effect
     * 
     * This function will create a rainbow effect on the ARGB LED strip.
     * 
     * NOTE: This function will run indefinitely. Device must be powered off to reset.
     * 
     * @return N/A
     */
    void rainbow_effect() {
      while (true) {
        for (int j = 0; j < 255; j++) {
          for (int i = 0; i < NUM_LEDS; i++) {
            led[i] = CHSV(i - (j * 2), 255, 255); /* The higher the value 4 the less fade there is and vice versa */ 
          }
          FastLED.show();
          delay(25); /* Change this to your hearts desire, the lower the value the faster your colors move (and vice versa) */
          
          // check for RF signal
          if (receiver.receiveData()) {
            Serial.println("Received RF signal");
            if (receiver.getRainbowEffectState() == false) {
                Serial.println("Rainbow effect false");
              // reset
              offARGB();
              return;
            }
          }
        }
      }
    }

    /**
     * @brief Pushes RGB color to queue
     * 
     * This function will push the RGB color to the queue.
     * 
     * @param red, green, blue the RGB colors to be pushed to the queue
     * @return N/A
     */
    void pushback(cppQueue& q, int red, int green, int blue) {
      // save CRGB value to stack
      CRGB color = CRGB(red, green, blue);

      q.push(&color);
      // // DEBUG print stack size
      // Serial.println("Queue size: " + String(q.getCount()));
      // // print stack values
      // for (int i = 0; i < q.getCount(); i++) {
      //   CRGB color;
      //   q.peekIdx(&color, i);
      //   Serial.println("Queue value: " + String(color.r) + ", " + String(color.g) + ", " + String(color.b));
      // }

      // show contents of queue
      for (int i = 0; i < 3; i ++) {
        // queue size LEDs should flash the color based on the queue
        for (int j = 0; j < q.getCount(); j++) {
          CRGB color;
          q.peekIdx(&color, j);
          led[j] = color;
        }
        FastLED.show();
        delay(200);

        fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
        FastLED.show();
        delay(200);
      }
    }

    /**
     * @brief Visualizes the color queue
     * 
     * This function will flash the LED strip to show the colors in the queue.
     * 
     * @return N/A
     */
    void check_colorQueue(cppQueue& q) {
      // queue size LEDs should flash the color based on the queue
      for (int j = 0; j < q.getCount(); j++) {
        CRGB color;
        q.peekIdx(&color, j);
        led[j] = color;
      }
      FastLED.show();
      delay(2000);

      fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
      FastLED.show();
    }

    /**
     * @brief Flashes the LED strip to confirm a save
     * 
     * This function will flash the LED strip to confirm a save to EEPROM.
     * 
     * @return N/A
     */
    void flashConfirm() {
      for (int i = 0; i < 3; i ++) {
        led[0] = CRGB(red, green, blue);
        FastLED.show();

        onLED();

        delay(200);
        // led[0] = CRGB(0, 0, 0);
        fill_solid(led, NUM_LEDS, CRGB(0, 0, 0));
        FastLED.show();

        offLED();

        delay(200);
      }
    }

    /**
     * @brief Flashes the LED strip to indicate an error
     * 
     * This function will flash the LED strip to indicate an error based on the error code.
     * 
     * @param errorcode the error code to flash the LED strip (number of flashes)
     * @return N/A
     */
    void flashError(int errorcode) {
      /*
      * Flash error codes based on specific error.
      * 1: Invalid IR remote value recieved
      * 2: Unknown protocol from IR
      */
      /// TODO: Modify. Will react with any IR signals (e.g. iPhone Face ID and any other source of IR)
      for (int i = 0; i < errorcode; i++) {
        led[0] = CRGB(MAX_INTENSITY, 0, 0);
        FastLED.show();
        delay(50);
        led[0] = CRGB(0, 0, 0);
        FastLED.show();
      }
    } 

    void setOnState(bool ledon) {
        this->ledon = ledon;
    }

    void setRainbowEffectState(bool rainboweffect) {
        this->rainboweffect = rainboweffect;
    }

    CRGB getColors() {
        return CRGB(red, green, blue);
    }

    bool getOnState() {
        return ledon;
    }

    bool getRainbowEffectState() {
        return rainboweffect;
    }
  
  private:
    int pin;
    bool ledon;
    bool rainboweffect;
    int8_t red;
    int8_t green;
    int8_t blue;
};

#endif // LED_HPP
