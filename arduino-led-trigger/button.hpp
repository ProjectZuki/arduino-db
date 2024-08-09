
#ifndef BUTTON_HPP
#define BUTTON_HPP

#define BUTTON_PIN    21

class Button {
  public:
    Button(int pin) {
      this->pin = pin;      // set pin variable
      pinMode(pin, INPUT);  // set pin as input
    }

    /**
    * @brief Checks the button for input
    * 
    * This function will check the button for input. On input, the color from the queue
    * will be popped and set as the current color.
    * 
    * @return N/A
    */
    bool isPressed() {return digitalRead(pin) == HIGH;}

  private:
    int pin;
};

#endif  // BUTTON_HPP