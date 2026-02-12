#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include "HX711.h"
#include <Preferences.h>

class ScaleManager {
public:
    enum WeightUnit {
        GRAMS = 0,
        KILOGRAMS = 1,
        OUNCES = 2,
        POUNDS = 3
    };

    ScaleManager(uint8_t dataPin, uint8_t clockPin);
    
    void begin();
    void loadConfiguration();
    void saveConfiguration();
    
    // Калибрация
    bool performCalibration(float knownWeight);
    void performTare();
    
    // Четене
    float getRawWeight();
    float getWeight();
    bool isReady();
    
    // Единици
    void setUnit(WeightUnit unit);
    WeightUnit getUnit();
    String getUnitString();
    
    // Статус
    bool isCalibrated();
    float getCalibrationFactor();

private:
    HX711 scale;
    Preferences prefs;
    
    float calibrationFactor;
    long tareOffset;
    bool calibrated;
    WeightUnit currentUnit;
    
    float convertWeight(float grams);
};

#endif