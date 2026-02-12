#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ScaleManager.h"
#include "DryingSessionManager.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

class DisplayManager {
public:
    enum DisplayMode {
        MODE_NORMAL,           // Нормален кантар
        MODE_DRYING_LIVE,      // Сушене - Live данни
        MODE_DRYING_STATS,     // Сушене - Статистика
        MODE_DRYING_HISTORY    // Сушене - История
    };

    DisplayManager();
    
    bool begin();
    void clear();
    void setMode(DisplayMode mode);
    DisplayMode getMode();
    
    // Normal Mode екрани
    void showNormalWeight(float weight, String unit);
    void showUnitChange(String unit);
    
    // Drying Mode екрани
    void showDryingLive(DryingSessionManager& drying, float currentWeight);
    void showDryingStats(DryingSessionManager& drying);
    void showDryingHistory(DryingSessionManager& drying, int recordIndex);
    
    // Калибрация екрани
    void showCalibrationStep1();
    void showCalibrationStep2(float weight);
    void showCalibrationProgress();
    void showCalibrationResult(bool success, float error);
    
    // Сесия екрани
    void showSessionStart(float initialWeight);
    void showSessionEnd();
    void showDailyRecorded(int day, float weight, float lossPercent);
    
    // Помощни
    void showMessage(String title, String message, int delayMs = 2000);
    void drawProgressBar(int x, int y, int width, int height, float percent, float target);

private:
    Adafruit_SSD1306 display;
    DisplayMode currentMode;
    
    void centerText(String text, int y, int textSize = 1);
};

#endif