#include "ScaleManager.h"

ScaleManager::ScaleManager(uint8_t dataPin, uint8_t clockPin) {
    scale.begin(dataPin, clockPin);
    calibrationFactor = 1.0f;
    tareOffset = 0;
    calibrated = false;
    currentUnit = GRAMS;
}

void ScaleManager::begin() {
    loadConfiguration();
}

void ScaleManager::loadConfiguration() {
    prefs.begin("scale", true);
    calibrationFactor = prefs.getFloat("cal_factor", 1.0f);
    tareOffset = prefs.getLong("tare_offset", 0);
    calibrated = prefs.getBool("calibrated", false);
    currentUnit = (WeightUnit)prefs.getUChar("unit", GRAMS);
    prefs.end();
    
    if (calibrated) {
        scale.set_scale(calibrationFactor);
        scale.set_offset(tareOffset);
        Serial.printf("[Scale] Config loaded: Factor=%.6f, Offset=%ld\n", 
                      calibrationFactor, tareOffset);
    }
}

void ScaleManager::saveConfiguration() {
    prefs.begin("scale", false);
    prefs.putFloat("cal_factor", calibrationFactor);
    prefs.putLong("tare_offset", tareOffset);
    prefs.putBool("calibrated", calibrated);
    prefs.putUChar("unit", currentUnit);
    prefs.end();
    Serial.println("[Scale] Configuration saved");
}

bool ScaleManager::performCalibration(float knownWeight) {
    if (!scale.is_ready()) {
        Serial.println("[Scale] Not ready!");
        return false;
    }
    
    Serial.println("[Scale] STEP 1: Remove all weight...");
    delay(5000);
    
    // Tare без тежест
    long tareSum = 0;
    for (int i = 0; i < 10; i++) {
        tareSum += scale.read();
        delay(100);
    }
    tareOffset = tareSum / 10;
    Serial.printf("[Scale] Tare Offset: %ld\n", tareOffset);
    
    Serial.printf("[Scale] STEP 2: Hang %.0fg...\n", knownWeight);
    delay(10000);
    
    // Стабилизиране
    for(int i = 0; i < 20; i++) {
        delay(100);
    }
    
    // Raw четене с тежест
    long rawSum = 0;
    for (int i = 0; i < 10; i++) {
        rawSum += scale.read();
        delay(100);
    }
    long rawReading = rawSum / 10;
    
    long difference = rawReading - tareOffset;
    calibrationFactor = (float)difference / knownWeight;
    
    Serial.printf("[Scale] Raw: %ld, Diff: %ld, Factor: %.6f\n", 
                  rawReading, difference, calibrationFactor);
    
    scale.set_scale(calibrationFactor);
    
    // Тест
    float testWeight = scale.get_units(10);
    float error = abs(testWeight - knownWeight);
    float errorPercent = (error / knownWeight) * 100.0f;
    
    Serial.printf("[Scale] Test: %.1fg (Expected: %.1fg), Error: %.1f%%\n", 
                  testWeight, knownWeight, errorPercent);
    
    if (errorPercent < 5.0f) {
        calibrated = true;
        saveConfiguration();
        Serial.println("[Scale] Calibration successful!");
        return true;
    }
    
    Serial.println("[Scale] Calibration failed!");
    return false;
}

void ScaleManager::performTare() {
    scale.tare();
    Serial.println("[Scale] Tared");
}

float ScaleManager::getRawWeight() {
    if (!scale.is_ready()) {
        return NAN;
    }
    
    if (calibrated) {
        return scale.get_units(1);
    } else {
        return scale.read() - tareOffset;
    }
}

float ScaleManager::getWeight() {
    float grams = getRawWeight();
    if (isnan(grams)) return NAN;
    return convertWeight(grams);
}

bool ScaleManager::isReady() {
    return scale.is_ready();
}

void ScaleManager::setUnit(WeightUnit unit) {
    currentUnit = unit;
    saveConfiguration();
    Serial.printf("[Scale] Unit changed to: %s\n", getUnitString().c_str());
}

ScaleManager::WeightUnit ScaleManager::getUnit() {
    return currentUnit;
}

String ScaleManager::getUnitString() {
    switch(currentUnit) {
        case GRAMS: return "g";
        case KILOGRAMS: return "kg";
        case OUNCES: return "oz";
        case POUNDS: return "lb";
        default: return "g";
    }
}

bool ScaleManager::isCalibrated() {
    return calibrated;
}

float ScaleManager::getCalibrationFactor() {
    return calibrationFactor;
}

float ScaleManager::convertWeight(float grams) {
    switch(currentUnit) {
        case GRAMS:
            return grams;
        case KILOGRAMS:
            return grams / 1000.0f;
        case OUNCES:
            return grams * 0.035274f;
        case POUNDS:
            return grams * 0.00220462f;
        default:
            return grams;
    }
}