#include <Arduino.h>
#include <Wire.h>
#include "ScaleManager.h"
#include "StorageManager.h"
#include "DryingSessionManager.h"
#include "DisplayManager.h"
#include "ButtonHandler.h"
#include "WebServerManager.h"

// ============================================================================
// === PIN DEFINITIONS ===
// ============================================================================

// HX711 Scale (NodeMCU ESP32)
#define SCALE_DATA_PIN    18    // D5 = GPIO18
#define SCALE_CLOCK_PIN   19    // D6 = GPIO19

// Бутони
#define BTN_TARE_PIN      33    // GPIO33 - Тариране/Запис на деня
#define BTN_UNIT_PIN      25    // GPIO25 - Смяна единици/екрани
#define BTN_START_PIN     26    // GPIO26 - Start/Stop режим

// I2C за OLED (NodeMCU ESP32: SDA=D2/GPIO21, SCL=D1/GPIO22)
#define I2C_SDA           21
#define I2C_SCL           22

// ============================================================================
// === GLOBAL OBJECTS ===
// ============================================================================

ScaleManager scale(SCALE_DATA_PIN, SCALE_CLOCK_PIN);
StorageManager storage;
DryingSessionManager drying(storage);
DisplayManager display;
ButtonHandler buttons(BTN_TARE_PIN, BTN_UNIT_PIN, BTN_START_PIN);

// ============================================================================
// === TIMING ===
// ============================================================================

unsigned long lastWeightRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long messageDisplayTime = 0;

const unsigned long WEIGHT_READ_INTERVAL = 500;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500;
const unsigned long MESSAGE_DISPLAY_DURATION = 2000;

float currentWeight = 0.0f;
float lastDisplayedWeight = 0.0f;
bool showingMessage = false;

const float DISPLAY_UPDATE_THRESHOLD = 1.0f;

WebServerManager webServer; 

const char* WIFI_SSID = "nickygv";
const char* WIFI_PASSWORD = "10malkinegar4eta";

// ============================================================================
// === HELPER FUNCTIONS ===
// ============================================================================

void showTemporaryMessage(String title, String message) {
    display.showMessage(title, message, 0);
    showingMessage = true;
    messageDisplayTime = millis();
}

void forceDisplayUpdate() {
    lastDisplayUpdate = 0;
}

// ============================================================================
// === SETUP ===
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Drying Scale ===");
    
    Wire.begin(I2C_SDA, I2C_SCL);
    
    Serial.println("\n[Setup] Initializing components...");
    
    // Display
    if (!display.begin()) {
        Serial.println("[Setup] Display initialization FAILED!");
        while(1);
    }
    showTemporaryMessage("", "Starting...");
    
    // Scale
    scale.begin();
    if (!scale.isCalibrated()) {
        Serial.println("[Setup] WARNING: Scale not calibrated!");
        showTemporaryMessage("Warning", "Not calibrated");
    }
    
    // Storage
    if (!storage.begin()) {
        Serial.println("[Setup] Storage initialization FAILED!");
        showTemporaryMessage("Error", "Storage failed");
    }
    
    // Drying Session
    drying.begin();
    
    // Buttons
    buttons.begin();

    // === НОВА ИНИЦИАЛИЗАЦИЯ ===
    // Първо инициализирай указателите
    webServer.init(&drying, &currentWeight);
    
    // След това стартирай WiFi
    if (webServer.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("[Setup] Web server started successfully!");
        Serial.print("[Setup] Access at: http://");
        Serial.println(webServer.getIPAddress());
    } else {
        Serial.println("[Setup] Web server failed to start!");
    }
    
    // Проверка дали има активна сесия
    if (drying.isActive()) {
        Serial.println("[Setup] Active drying session detected!");
        buttons.setMode(ButtonHandler::OP_MODE_DRYING);
        display.setMode(DisplayManager::MODE_DRYING_LIVE);
        showTemporaryMessage("Resuming", "Active session");
        // НЕ тарираме - има активна сесия!
        currentWeight = scale.getRawWeight();
    } else {
        // Няма активна сесия - тарираме
        scale.performTare();
        currentWeight = 0.0f;
        lastDisplayedWeight = 0.0f;
        
        Serial.println("[Setup] Starting in NORMAL mode");
        buttons.setMode(ButtonHandler::OP_MODE_NORMAL);
        display.setMode(DisplayManager::MODE_NORMAL);
        
        // Показваме 0.0
        display.showNormalWeight(0.0f, scale.getUnitString());
    }
    
    // Форсирай display update
    lastDisplayUpdate = 0;
    lastWeightRead = 0;
    
    Serial.println("\n[Setup] System ready!");
    Serial.println("Commands:");
    Serial.println("  cal 1000  - Calibrate with 1000g");
    Serial.println("  tare      - Tare the scale");
    Serial.println("  format    - Format storage");
    Serial.println("  info      - Show system info\n");
}

// ============================================================================
// === LOOP ===
// ============================================================================

void loop() {
    unsigned long currentTime = millis();

     webServer.handle(); 
    
    // ========== SERIAL COMMANDS ==========
    if (Serial.available()) {
        String command = Serial.readString();
        command.trim();
        
        if (command.startsWith("cal")) {
            float knownWeight = command.substring(4).toFloat();
            if (knownWeight > 0) {
                display.showCalibrationStep1();
                delay(5000);
                display.showCalibrationStep2(knownWeight);
                delay(10000);
                display.showCalibrationProgress();
                
                bool success = scale.performCalibration(knownWeight);
                
                if (success) {
                    display.showCalibrationResult(true, 0);
                } else {
                    display.showCalibrationResult(false, 5.0);
                }
                delay(3000);
                lastDisplayUpdate = 0; // Форсирай обновяване след калибрация
            } else {
                Serial.println("Invalid weight! Use: cal 1000");
            }
        } 
        else if (command == "tare") {
            scale.performTare();
            showTemporaryMessage("", "Tared");
        }
        else if (command == "format") {
            showTemporaryMessage("Formatting", "Storage...");
            storage.format();
            showTemporaryMessage("Format", "Complete");
        }
        else if (command == "info") {
            Serial.println("\n=== SYSTEM INFO ===");
            Serial.printf("Scale calibrated: %s\n", scale.isCalibrated() ? "YES" : "NO");
            Serial.printf("Calibration factor: %.6f\n", scale.getCalibrationFactor());
            Serial.printf("Current weight: %.1f %s\n", currentWeight, scale.getUnitString().c_str());
            Serial.printf("Operation mode: %s\n", buttons.getMode() == ButtonHandler::OP_MODE_NORMAL ? "NORMAL" : "DRYING");
            
            if (drying.isActive()) {
                DryingSession& session = drying.getSession();
                Serial.printf("\nActive Session:\n");
                Serial.printf("  Day: %d\n", session.currentDay);
                Serial.printf("  Initial: %.1fg\n", session.initialWeight);
                Serial.printf("  Target: -%.1f%%\n", session.targetLossPercent);
                Serial.printf("  Current loss: -%.1f%%\n", drying.getCurrentLossPercent());
                Serial.printf("  Records: %d\n", session.recordCount);
                
                int daysRemaining = drying.estimateDaysRemaining();
                if (daysRemaining >= 0) {
                    Serial.printf("  Estimated days: ~%d\n", daysRemaining);
                }
            }
            
            storage.printFileSystem();
            Serial.println("==================\n");
        }
        else if (command == "end") {
            if (drying.isActive()) {
                drying.endSession();
                buttons.setMode(ButtonHandler::OP_MODE_NORMAL);
                display.setMode(DisplayManager::MODE_NORMAL);
                showTemporaryMessage("Session", "Ended");
                Serial.println("Session ended");
            }
        }
        else {
            Serial.println("Unknown command!");
        }
    }
    
    // ========== WEIGHT READING ==========
    if (currentTime - lastWeightRead >= WEIGHT_READ_INTERVAL) {
        if (scale.isReady()) {
            float rawWeight = scale.getRawWeight();
            if (!isnan(rawWeight)) {
                currentWeight = rawWeight;
            }
        }
        lastWeightRead = currentTime;
    }
    
    // ========== MESSAGE TIMEOUT ==========
    if (showingMessage && (currentTime - messageDisplayTime >= MESSAGE_DISPLAY_DURATION)) {
        showingMessage = false;
        lastDisplayUpdate = 0; // Форсирай обновяване
        lastDisplayedWeight = 0.0f; // Форсирай показване на тегло
    }

    // ========== AUTO DAILY RECORD (DRYING MODE) ==========
if (buttons.getMode() == ButtonHandler::OP_MODE_DRYING && drying.isActive()) {
    DryingSession& session = drying.getSession();
    uint32_t currentTimestamp = millis() / 1000;
    uint32_t elapsed = currentTimestamp - session.lastRecordTimestamp;
    
    // Ако са минали 24 часа (86400 секунди)
    // ЗА ТЕСТВАНЕ: Използвай 60 секунди вместо 86400
    if (elapsed >= 86400) {  // 24 часа
        drying.recordDailyWeight(currentWeight);
        DailyRecord* lastRecord = drying.getLastRecord();

        if (lastRecord) {
            showTemporaryMessage("Day " + String(lastRecord->day), 
                                 "Loss: " + String(lastRecord->lossPercent, 1) + "%");
            Serial.printf("[Auto] Day %d recorded: %.1fg, Loss: %.1f%%\n", 
                         lastRecord->day, lastRecord->weight, lastRecord->lossPercent);
        }
    }
}
    
   // ========== DISPLAY UPDATE ==========
if (!showingMessage && currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    ButtonHandler::OperationMode mode = buttons.getMode();
    
    if (mode == ButtonHandler::OP_MODE_NORMAL) {
        // Normal mode
        if (!isnan(currentWeight)) {
            if (abs(currentWeight - lastDisplayedWeight) >= DISPLAY_UPDATE_THRESHOLD) {
                float displayWeight = currentWeight;
                String unit = scale.getUnitString();
                
                switch(scale.getUnit()) {
                    case ScaleManager::KILOGRAMS:
                        displayWeight = currentWeight / 1000.0f;
                        break;
                    case ScaleManager::OUNCES:
                        displayWeight = currentWeight * 0.035274f;
                        break;
                    case ScaleManager::POUNDS:
                        displayWeight = currentWeight * 0.00220462f;
                        break;
                    default:
                        displayWeight = currentWeight;
                        break;
                }
                
                display.showNormalWeight(displayWeight, unit);
                lastDisplayedWeight = currentWeight;
            }
        }
    } 
    else {
        // Drying mode
        DisplayManager::DisplayMode displayMode = display.getMode();
        
        // Провери дали е форсирано обновяване (lastDisplayUpdate == 0)
        bool forceUpdate = (lastDisplayUpdate == 0);
        
        switch (displayMode) {
            case DisplayManager::MODE_DRYING_LIVE:
                // Обнови ако има промяна ИЛИ е форсирано
                if (forceUpdate || abs(currentWeight - lastDisplayedWeight) >= DISPLAY_UPDATE_THRESHOLD) {
                    display.showDryingLive(drying, currentWeight);
                    lastDisplayedWeight = currentWeight;
                }
                break;
                
            case DisplayManager::MODE_DRYING_STATS:
                display.showDryingStats(drying);
                break;
                
            case DisplayManager::MODE_DRYING_HISTORY:
                display.showDryingHistory(drying, buttons.getHistoryIndex());
                break;
                
            default:
                break;
        }
    }
    
    lastDisplayUpdate = currentTime;
}
    
    // ========== BUTTON HANDLING ==========
    buttons.update(scale, drying, display, currentWeight);
    
    delay(10);
}