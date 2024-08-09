
#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include <SoftwareSerial.h>   // HC-12 module
#include <FastLED.h>          // FastLED library

// HC-12 module
SoftwareSerial HC12(2, 3);  // HC-12 TX Pin, HC-12 RX Pin
#define START_MARKER 0x7E
#define END_MARKER 0x7F

// bool ledonrx = false;
// bool rainbowrx = false;

struct dataPacket {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  bool ledon;
  bool rainbow;
  uint8_t checksum;
};

dataPacket packet;

class Receiver {
  public:
    Receiver() {
        // HC-12 module communication
        HC12.begin(9600);
        ledon = false;
        rainbow_effect = false;
        red = 0;
        green = 0;
        blue = 0;
    }
    
    /**
     * @brief Receives transmitter data when available
     * 
     * This function will check transmitter data values red, green, blue, ledon, and rainboweffect when available.
     * 
     * @return N/A
     */
    bool receiveData() {
      static bool receiving = false;
      static byte buffer[sizeof(dataPacket)];
      static uint8_t bufferIndex = 0;

      while (HC12.available() > 0) {
        byte receivedByte = HC12.read();
        
        if (receivedByte == START_MARKER) {
          receiving = true;
          bufferIndex = 0;
        } else if (receivedByte == END_MARKER) {
          if (receiving && bufferIndex == sizeof(dataPacket)) {
            dataPacket packet;
            memcpy(&packet, buffer, sizeof(dataPacket));

            // Calculate the checksum of the received packet
            uint8_t calculatedChecksum = calculateChecksum(packet);

            // Debug prints for received packet and calculated checksum
            // Serial.print("Received packet: ");
            // Serial.print(packet.red);
            // Serial.print(", ");
            // Serial.print(packet.green);
            // Serial.print(", ");
            // Serial.print(packet.blue);
            // Serial.print(", LED: ");
            // Serial.print(packet.ledon);
            // Serial.print(", Rainbow: ");
            // Serial.println(packet.rainbow);
            // Serial.print("Received checksum: ");
            // Serial.println(packet.checksum);
            // Serial.print("Calculated checksum: ");
            // Serial.println(calculatedChecksum);

            // Validate integrity
            if (packet.checksum == calculatedChecksum) {
              // Update your variables here
              red = packet.red;
              green = packet.green;
              blue = packet.blue;
              ledon = packet.ledon;
              rainbow_effect = packet.rainbow;
              return true;
            } else {
              Serial.println("ERROR: checksum mismatch, possible data corruption");
            }
          }
          receiving = false; // Reset receiving state after end marker
        } else if (receiving) {
          if (bufferIndex < sizeof(dataPacket)) {
            buffer[bufferIndex++] = receivedByte;
          } else {
            // Buffer overflow, reset receiving
            receiving = false;
            bufferIndex = 0;
            Serial.println("ERROR: Buffer overflow, resetting receiving state.");
            return;
          }
        }
      }
      return false;
    }

    /**
     * @brief Gets the color values
     * 
     * This function will return the color values.
     * 
     * @return the color values
     */
    CRGB getColors() {
        return CRGB(red, green, blue);
    }

    bool getOnState() {
        return ledon;
    }

    bool getRainbowEffectState() {
        return rainbow_effect;
    }

    void setOnState(bool ledon) {
        this->ledon = ledon;
    }

    void setRainbowEffect(bool rainbow_effect) {
        this->rainbow_effect = rainbow_effect;
    }


  private:
    int8_t red;
    int8_t green;
    int8_t blue;
    bool ledon;
    bool rainbow_effect;
    
    /**
     * @brief Calculates the checksum for the data packet
     * 
     * This function will calculate the checksum for the data packet.
     * 
     * @param packet the data packet to calculate the checksum
     * @return the checksum value
     */
    uint8_t calculateChecksum(const dataPacket& packet) {
      uint8_t checksum = 0;
      const uint8_t* ptr = (const uint8_t*)&packet;

      // Calculate checksum for the packet excluding the checksum field itself
      for (size_t i = 0; i < sizeof(packet) - sizeof(packet.checksum); ++i) {
        checksum += ptr[i];
      }
      return checksum;
    }
};

#endif // RECEIVER_HPP
