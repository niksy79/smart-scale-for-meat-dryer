#include "DisplayManager.h"

DisplayManager::DisplayManager() 
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
    currentMode = MODE_NORMAL;
}

bool DisplayManager::begin() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("[Display] OLED allocation failed");
        return false;
    }
    
    display.clearDisplay();
    display.display();
    Serial.println("[Display] Initialized successfully");
    return true;
}

void DisplayManager::clear() {
    display.clearDisplay();
    display.display();
}

void DisplayManager::setMode(DisplayMode mode) {
    currentMode = mode;
    Serial.printf("[Display] Mode changed to: %d\n", mode);
}

DisplayManager::DisplayMode DisplayManager::getMode() {
    return currentMode;
}

// ============= NORMAL MODE =============

void DisplayManager::showNormalWeight(float weight, String unit) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Форматиране на теглото
    String weightStr;
    
    // Проверка за нула според единицата
    float zeroThreshold;
    if (unit == "kg") {
        zeroThreshold = 0.002f;  // 2 грама в kg
    } else if (unit == "lb") {
        zeroThreshold = 0.004f;  // 2 грама в lb
    } else if (unit == "oz") {
        zeroThreshold = 0.07f;   // 2 грама в oz
    } else {
        zeroThreshold = 2.0f;    // 2 грама
    }
    
    if (abs(weight) <= zeroThreshold) {
        weightStr = "0.0";
    } else {
        if (unit == "g" && abs(weight) >= 1000) {
            weightStr = String((int)weight);
        } else if (unit == "kg") {
            weightStr = String(weight, 3);  // 3 decimal places
        } else if (unit == "oz") {
            weightStr = String(weight, 2);  // 2 decimal places
        } else if (unit == "lb") {
            weightStr = String(weight, 3);  // 3 decimal places
        } else {
            weightStr = String(weight, 1);  // 1 decimal place
        }
    }
    // Размер на текста
    int textSize = 3;
    int charWidth = textSize * 6;
    int totalWidth = weightStr.length() * charWidth;
    
    if (totalWidth > SCREEN_WIDTH - 10) {
        textSize = 2;
        charWidth = textSize * 6;
        totalWidth = weightStr.length() * charWidth;
    }
    
    // Центриране
    int x = (SCREEN_WIDTH - totalWidth) / 2;
    int y = (SCREEN_HEIGHT - (textSize * 8)) / 2 - 5;
    
    display.setTextSize(textSize);
    display.setCursor(x, y);
    display.print(weightStr);
    
    // Единица долу вдясно
    display.setTextSize(1);
    int unitX = SCREEN_WIDTH - (unit.length() * 6) - 2;
    int unitY = SCREEN_HEIGHT - 10;
    display.setCursor(unitX, unitY);
    display.print(unit);
    
    display.display();
}

void DisplayManager::showUnitChange(String unit) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    
    String fullName;
    if (unit == "g") fullName = "GRAMS";
    else if (unit == "kg") fullName = "KILOGRAMS";
    else if (unit == "oz") fullName = "OUNCES";
    else if (unit == "lb") fullName = "POUNDS";
    
    centerText(fullName, 24, 2);
    display.display();
    // Махнато delay - loop() ще обнови екрана
}

// ============= DRYING MODE - LIVE =============

void DisplayManager::showDryingLive(DryingSessionManager& drying, float currentWeight) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    DryingSession& session = drying.getSession();
    
    // Header - Ден и начално тегло
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("DAY ");
    display.print(session.currentDay);
    display.print(" | ");
    display.print((int)session.initialWeight);
    display.print("g");
    
    // Текущо тегло (голям шрифт)
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print((int)currentWeight);
    display.println("g");
    
    // Загуба в %
    float lossPercent = drying.getCurrentLossPercent();
    display.setTextSize(1);
    display.setCursor(0, 32);
    display.print("Loss: ");
    display.setTextSize(2);
    display.print(lossPercent, 1);
    display.println("%");
    
    // Progress bar
    display.setTextSize(1);
    drawProgressBar(0, 52, 100, 10, lossPercent, session.targetLossPercent);
    
    // Статус/Оставащо
    display.setCursor(104, 54);
    if (drying.isReady()) {
        display.print("OK");
    } else {
        int remaining = (int)(session.targetLossPercent - lossPercent);
        if (remaining > 0) {
            display.print(remaining);
        }
    }
    
    display.display();
}

// ============= DRYING MODE - STATS =============

void DisplayManager::showDryingStats(DryingSessionManager& drying) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    DryingSession& session = drying.getSession();
    DailyRecord* lastRecord = drying.getLastRecord();
    
    // Title
    display.setCursor(20, 0);
    display.println("STATISTICS");
    
    // Данни
    display.setCursor(0, 12);
    display.print("Initial: ");
    display.print(session.initialWeight, 1);
    display.println("g");
    
    if (lastRecord) {
        display.setCursor(0, 22);
        display.print("Current: ");
        display.print(lastRecord->weight, 1);
        display.println("g");
    }
    
    display.setCursor(0, 32);
    display.print("Target:  -");
    display.print(session.targetLossPercent, 1);
    display.println("%");
    
    display.setCursor(0, 42);
    display.print("Status:  -");
    display.print(drying.getCurrentLossPercent(), 1);
    display.println("%");
    
    // Прогноза
    int daysRemaining = drying.estimateDaysRemaining();
    display.setCursor(0, 52);
    display.print("Remain:  ");
    if (daysRemaining >= 0) {
        display.print("~");
        display.print(daysRemaining);
        display.println(" days");
    } else {
        display.println("N/A");
    }
    
    display.display();
}

// ============= DRYING MODE - HISTORY =============

void DisplayManager::showDryingHistory(DryingSessionManager& drying, int recordIndex) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    DailyRecord* record = drying.getRecord(recordIndex);
    if (!record) {
        centerText("No data", 28);
        display.display();
        return;
    }
    
// Header
display.setCursor(0, 0);
display.print("DAY ");
display.print(record->day);

// Последният запис е "Today"
if (recordIndex == drying.getRecordCount() - 1) {
    display.print("  (Today)");
} 
// Предпоследен е "Yesterday"
else if (recordIndex == drying.getRecordCount() - 2 && drying.getRecordCount() > 1) {
    display.print(" (Yesterday)");
}
    
    // Данни
    display.setCursor(0, 16);
    display.print("Weight: ");
    display.print(record->weight, 1);
    display.println("g");
    
    display.setCursor(0, 28);
    display.print("Loss:   ");
    display.print(record->lossPercent, 1);
    display.println("%");
    
    display.setCursor(0, 40);
    display.print("Change: ");
    display.print(record->dayChange, 1);
    display.println("g");
    
    // Навигация
    display.setCursor(10, 54);
    display.print("< PREV    NEXT >");
    
    display.display();
}

// ============= CALIBRATION =============

void DisplayManager::showCalibrationStep1() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Calibration:");
    display.println("Remove weight!");
    display.println("Wait 5 sec...");
    display.display();
}

void DisplayManager::showCalibrationStep2(float weight) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.println("Calibration:");
    display.print("Hang ");
    display.print((int)weight);
    display.println("g");
    display.println("Wait 10 sec...");
    display.display();
}

void DisplayManager::showCalibrationProgress() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(20, 28);
    display.println("Calibrating...");
    display.display();
}

void DisplayManager::showCalibrationResult(bool success, float error) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 16);
    
    if (success) {
        display.println("  SUCCESSFUL");
        display.println("  calibration!");
    } else {
        display.println("    FAILED");
        display.println("  calibration!");
        display.print("  Error: ");
        display.print(error, 1);
        display.println("%");
    }
    
    display.display();
}

// ============= SESSION =============

void DisplayManager::showSessionStart(float initialWeight) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 16);
    display.println("SESSION STARTED");
    display.print("Initial: ");
    display.print((int)initialWeight);
    display.println("g");
    display.print("Target: -40%");
    display.display();
}

void DisplayManager::showSessionEnd() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println("SESSION");
    display.setCursor(25, 40);
    display.println("ENDED");
    display.display();
}

void DisplayManager::showDailyRecorded(int day, float weight, float lossPercent) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.print("DAY ");
    display.print(day);
    display.println(" RECORDED");
    display.println();
    display.print("Weight: ");
    display.print(weight, 1);
    display.println("g");
    display.print("Loss:   ");
    display.print(lossPercent, 1);
    display.println("%");
    display.display();
}

// ============= HELPERS =============

void DisplayManager::showMessage(String title, String message, int delayMs) {
    display.clearDisplay();
    display.setTextSize(1);
    
    if (title.length() > 0) {
        centerText(title, 16);
    }
    
    centerText(message, 32);
    display.display();
    
    // Само ако delayMs е зададен за setup съобщения
    if (delayMs > 0) {
        delay(delayMs);
    }
}

void DisplayManager::centerText(String text, int y, int textSize) {
    display.setTextSize(textSize);
    int charWidth = textSize * 6;
    int x = (SCREEN_WIDTH - (text.length() * charWidth)) / 2;
    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, float percent, float target) {
    // Рамка
    display.drawRect(x, y, width + 2, height, SSD1306_WHITE);
    
    // Запълване
    int fillWidth = (int)((percent / target) * width);
    if (fillWidth > width) fillWidth = width;
    if (fillWidth > 0) {
        display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE);
    }
}