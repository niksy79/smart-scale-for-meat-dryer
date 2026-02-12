#include "ButtonHandler.h"

// Референция към флага за съобщения от main.cpp
extern bool showingMessage;
extern unsigned long messageDisplayTime;
extern unsigned long lastDisplayUpdate;
extern float lastDisplayedWeight;

ButtonHandler::ButtonHandler(uint8_t tarePin, uint8_t unitPin, uint8_t startPin) {
    btnTarePin = tarePin;
    btnUnitPin = unitPin;
    btnStartPin = startPin;
    
    currentMode = OP_MODE_NORMAL;
    historyIndex = 0;
    lastButtonCheck = 0;
    
    for (int i = 0; i < 3; i++) {
        lastButtonStates[i] = HIGH;
        buttonPressTime[i] = 0;
        buttonHoldDetected[i] = false;
    }
}

void ButtonHandler::begin() {
    pinMode(btnTarePin, INPUT_PULLUP);
    pinMode(btnUnitPin, INPUT_PULLUP);
    pinMode(btnStartPin, INPUT_PULLUP);
    
    Serial.println("[Buttons] Initialized");
}

void ButtonHandler::update(ScaleManager& scale, DryingSessionManager& drying, DisplayManager& display, float currentWeight) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastButtonCheck < DEBOUNCE_MS) {
        return;
    }
    
    // Четене на текущо състояние на бутоните
    bool currentStates[3] = {
        digitalRead(btnTarePin),
        digitalRead(btnUnitPin),
        digitalRead(btnStartPin)
    };
    
    // Обработка на hold detection САМО ЗА START бутон
    if (currentStates[2] == LOW) {
        if (buttonPressTime[2] == 0) {
            buttonPressTime[2] = currentTime;
        }
        
        if (!buttonHoldDetected[2] && (currentTime - buttonPressTime[2] >= HOLD_MS)) {
            buttonHoldDetected[2] = true;
            
            // START HOLD ACTION - Превключване режим или край на сесия
            if (currentMode == OP_MODE_NORMAL) {
                // Преминаване в Drying Mode + Нова сесия
                if (!drying.isActive()) {
                    float initialWeight = scale.getRawWeight();
                    
                    if (!isnan(initialWeight) && abs(initialWeight) > 5.0f) {
                        drying.startNewSession(abs(initialWeight), 40.0f);
                        currentMode = OP_MODE_DRYING;
                        display.setMode(DisplayManager::MODE_DRYING_LIVE);
                        display.showSessionStart(abs(initialWeight));
                        
                        // Активирай временно съобщение
                        showingMessage = true;
                        messageDisplayTime = millis();
                        
                        Serial.println("[Buttons] Switched to DRYING mode");
                    } else {
                        display.showMessage("Error", "Invalid weight", 0);
                        showingMessage = true;
                        messageDisplayTime = millis();
                        Serial.printf("[Buttons] Invalid weight: %.1f\n", initialWeight);
                    }
                }
            } else {
                // Край на сесия + връщане в Normal Mode
                drying.endSession();
                currentMode = OP_MODE_NORMAL;
                display.setMode(DisplayManager::MODE_NORMAL);
                display.showSessionEnd();

                 // Форсирай display update след съобщението
    lastDisplayUpdate = 0;
    lastDisplayedWeight = -999.0f;  // ← ДОБАВИ ТОВА! Форсира обновяван
                
                // Активирай временно съобщение
                showingMessage = true;
                messageDisplayTime = millis();
                
                Serial.println("[Buttons] Switched to NORMAL mode");
            }
        }
    } else {
        resetButton(2);
    }
    
    // Обработка на нормални натискания (само ако няма hold на START)
    if (!buttonHoldDetected[2]) {
        if (currentMode == OP_MODE_NORMAL) {
            handleNormalMode(scale, display);
        } else {
            handleDryingMode(scale, drying, display, currentWeight);
        }
    }
    
    // Запазване на състоянията
    for (int i = 0; i < 3; i++) {
        lastButtonStates[i] = currentStates[i];
    }
    
    lastButtonCheck = currentTime;
}

void ButtonHandler::handleNormalMode(ScaleManager& scale, DisplayManager& display) {
    // TARE бутон - тариране
    if (isButtonPressed(0)) {
        scale.performTare();
        Serial.println("[Buttons] Tare");
    }
    
    // UNIT бутон - смяна единици
    if (isButtonPressed(1)) {
        ScaleManager::WeightUnit currentUnit = scale.getUnit();
        ScaleManager::WeightUnit nextUnit = (ScaleManager::WeightUnit)((currentUnit + 1) % 4);
        scale.setUnit(nextUnit);
        display.showUnitChange(scale.getUnitString());
        
        // Активирай временно съобщение
        showingMessage = true;
        messageDisplayTime = millis();
        
        Serial.println("[Buttons] Unit changed");
    }
    
    // START бутон (кратко) - няма действие в Normal Mode
}

void ButtonHandler::handleDryingMode(ScaleManager& scale, DryingSessionManager& drying, DisplayManager& display, float currentWeight) {
    DisplayManager::DisplayMode displayMode = display.getMode();
    
    // TARE бутон - Навигация НАЗАД във времето (по-стари дни)
    if (isButtonPressed(0)) {
        if (displayMode == DisplayManager::MODE_DRYING_LIVE) {
            // От Live → покажи предпоследния ден
            if (drying.getRecordCount() > 1) {
                historyIndex = drying.getRecordCount() - 2;
                display.setMode(DisplayManager::MODE_DRYING_HISTORY);
                lastDisplayUpdate = 0;
                Serial.printf("[Buttons] History - Day %d\n", historyIndex);
            }
        } 
        else if (displayMode == DisplayManager::MODE_DRYING_HISTORY) {
            // Навигация назад (по-стар запис)
            if (historyIndex > 0) {
                historyIndex--;
                lastDisplayUpdate = 0;
                Serial.printf("[Buttons] History - Day %d (older)\n", historyIndex);
            } else {
                Serial.println("[Buttons] Already at oldest record");
            }
        }
        else if (displayMode == DisplayManager::MODE_DRYING_STATS) {
            // От Stats → обратно към Live
            display.setMode(DisplayManager::MODE_DRYING_LIVE);
            lastDisplayUpdate = 0;
            Serial.println("[Buttons] Back to Live from Stats");
        }
    }
    
    // UNIT бутон - Навигация НАПРЕД / циклична смяна
    if (isButtonPressed(1)) {
        if (displayMode == DisplayManager::MODE_DRYING_LIVE) {
            display.setMode(DisplayManager::MODE_DRYING_STATS);
            lastDisplayUpdate = 0;
            Serial.println("[Buttons] Switched to Stats");
        } 
        else if (displayMode == DisplayManager::MODE_DRYING_STATS) {
            // От Stats → History (показваме ПОСЛЕДНИЯ ден)
            if (drying.getRecordCount() > 0) {
                historyIndex = drying.getRecordCount() - 1;
                display.setMode(DisplayManager::MODE_DRYING_HISTORY);
                lastDisplayUpdate = 0;
                Serial.printf("[Buttons] History - Day %d (latest)\n", historyIndex);
            }
        }
        else if (displayMode == DisplayManager::MODE_DRYING_HISTORY) {
            // Навигация напред (по-нов запис) ИЛИ wrap към Live
            if (historyIndex < drying.getRecordCount() - 1) {
                historyIndex++;
                lastDisplayUpdate = 0;
                Serial.printf("[Buttons] History - Day %d (newer)\n", historyIndex);
            } else {
                // Вече сме на най-новия → обратно към Live
                display.setMode(DisplayManager::MODE_DRYING_LIVE);
                lastDisplayUpdate = 0;
                Serial.println("[Buttons] Back to Live (cycle)");
            }
        }
    }
    
    // START бутон (кратко) - Директно към Live от всякъде
    if (isButtonPressed(2) && !buttonHoldDetected[2]) {
        if (displayMode != DisplayManager::MODE_DRYING_LIVE) {
            display.setMode(DisplayManager::MODE_DRYING_LIVE);
            lastDisplayUpdate = 0;
            Serial.println("[Buttons] Back to Live (START)");
        }
    }
}

bool ButtonHandler::isButtonPressed(uint8_t buttonIndex) {
    uint8_t pin;
    switch(buttonIndex) {
        case 0: pin = btnTarePin; break;
        case 1: pin = btnUnitPin; break;
        case 2: pin = btnStartPin; break;
        default: return false;
    }
    
    bool currentState = digitalRead(pin);
    
    // Pressed = LOW (falling edge) and not in hold
    if (currentState == LOW && lastButtonStates[buttonIndex] == HIGH && !buttonHoldDetected[buttonIndex]) {
        return true;
    }
    
    return false;
}

bool ButtonHandler::isButtonHeld(uint8_t buttonIndex) {
    return buttonHoldDetected[buttonIndex];
}

void ButtonHandler::resetButton(uint8_t buttonIndex) {
    buttonPressTime[buttonIndex] = 0;
    buttonHoldDetected[buttonIndex] = false;
}

ButtonHandler::OperationMode ButtonHandler::getMode() {
    return currentMode;
}

void ButtonHandler::setMode(OperationMode mode) {
    currentMode = mode;
    Serial.printf("[Buttons] Mode set to: %d\n", mode);
}