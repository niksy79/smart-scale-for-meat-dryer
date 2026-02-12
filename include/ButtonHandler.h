#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "ScaleManager.h"
#include "DryingSessionManager.h"
#include "DisplayManager.h"


class ButtonHandler {
public:
    enum OperationMode {
        OP_MODE_NORMAL,    // Нормален кантар
        OP_MODE_DRYING     // Режим на сушене
    };

    ButtonHandler(uint8_t tarePin, uint8_t unitPin, uint8_t startPin);
    
    void begin();
    void update(ScaleManager& scale, DryingSessionManager& drying, DisplayManager& display, float currentWeight);
    
    OperationMode getMode();
    void setMode(OperationMode mode);
     int getHistoryIndex() { return historyIndex; }

private:
    uint8_t btnTarePin;
    uint8_t btnUnitPin;
    uint8_t btnStartPin;
    
    OperationMode currentMode;
    int historyIndex;  // За навигация в историята
    
    // Button states
    bool lastButtonStates[3];
    unsigned long buttonPressTime[3];
    bool buttonHoldDetected[3];
    
    // Timing
    unsigned long lastButtonCheck;
    const unsigned long DEBOUNCE_MS = 200;
    const unsigned long HOLD_MS = 3000;
    
    // Button handling
    void handleNormalMode(ScaleManager& scale, DisplayManager& display);
    void handleDryingMode(ScaleManager& scale, DryingSessionManager& drying, DisplayManager& display, float currentWeight);
    
    bool isButtonPressed(uint8_t buttonIndex);
    bool isButtonHeld(uint8_t buttonIndex);
    void resetButton(uint8_t buttonIndex);


};

#endif