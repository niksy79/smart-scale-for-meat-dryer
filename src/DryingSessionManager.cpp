#include "DryingSessionManager.h"

DryingSessionManager::DryingSessionManager(StorageManager& storage) 
    : storage(storage) {
    initializeSession();
}

void DryingSessionManager::begin() {
    // Опит за зареждане на съществуваща сесия
    if (storage.loadSession(session)) {
        if (session.isActive) {
            Serial.printf("[Drying] Active session loaded: Day %d, Loss: %.1f%%\n", 
                         session.currentDay, getCurrentLossPercent());
        } else {
            Serial.println("[Drying] Inactive session loaded");
        }
    } else {
        Serial.println("[Drying] No previous session found");
        initializeSession();
    }
}

void DryingSessionManager::initializeSession() {
    session.isActive = false;
    session.initialWeight = 0.0f;
    session.targetLossPercent = 40.0f;
    session.startTimestamp = 0;
    session.currentDay = 0;
    session.recordCount = 0;
}

bool DryingSessionManager::startNewSession(float initialWeight, float targetLossPercent) {
    if (initialWeight <= 0) {
        Serial.println("[Drying] Invalid initial weight!");
        return false;
    }
    
    // Нова сесия
    session.isActive = true;
    session.initialWeight = initialWeight;
    session.targetLossPercent = targetLossPercent;
    session.startTimestamp = millis() / 1000;
    session.lastRecordTimestamp = session.startTimestamp;  // Запази кога е започнал

    
    Serial.printf("[Drying] New session started: %.1fg, Target: -%.1f%%\n", 
                  initialWeight, targetLossPercent);
    
// Започваме от Ден 1
session.currentDay = 1;
session.recordCount = 0;

// Запис на Ден 1 (първият ден)
DailyRecord& record = session.records[0];
record.day = 1;  // ← Ден 1 (не 0!)
record.timestamp = session.startTimestamp;
record.weight = initialWeight;
record.lossPercent = 0.0f;
record.dayChange = 0.0f;
session.recordCount = 1;
// currentDay вече е 1, не го променяме
    
    // Запазване
    return storage.saveSession(session);
}

bool DryingSessionManager::recordDailyWeight(float weight) {
    if (!session.isActive) {
        Serial.println("[Drying] No active session!");
        return false;
    }
    
    Serial.printf("[Drying] Recording Day %d weight: %.1fg\n", session.currentDay, weight);
    
    // Добавяне чрез StorageManager (той прави изчисленията)
    return storage.addDailyRecord(session, weight);
}

void DryingSessionManager::endSession() {
    if (!session.isActive) {
        return;
    }
    
    session.isActive = false;
    storage.saveSession(session);
    
    Serial.println("[Drying] Session ended");
}

bool DryingSessionManager::isActive() {
    return session.isActive;
}

DryingSession& DryingSessionManager::getSession() {
    return session;
}

float DryingSessionManager::getCurrentLossPercent() {
    if (session.recordCount == 0) {
        return 0.0f;
    }
    
    return session.records[session.recordCount - 1].lossPercent;
}

float DryingSessionManager::getRemainingLossPercent() {
    float current = getCurrentLossPercent();
    float remaining = session.targetLossPercent - current;
    return remaining > 0 ? remaining : 0.0f;
}

int DryingSessionManager::estimateDaysRemaining() {
    if (session.recordCount < 3) {
        return -1; // Недостатъчно данни
    }
    
    float avgDailyLoss = calculateAverageDailyLoss(3);
    
    if (avgDailyLoss <= 0.01f) {  // ← ВАЖНА ПРОВЕРКА!
        return -1; // Няма реална загуба
    }
    
    float currentLoss = getCurrentLossPercent();
    float remainingLoss = session.targetLossPercent - currentLoss;
    
    if (remainingLoss <= 0) {
        return 0; // Вече е готов
    }
    
    // Дни = оставаща загуба / средна дневна загуба
    int daysRemaining = (int)(remainingLoss / avgDailyLoss);
    
    // Ограничение: максимум 99 дни (за сигурност)
    if (daysRemaining > 99) {
        return -1;  // Нереалистично, вероятно грешка
    }
    
    return daysRemaining;
}

bool DryingSessionManager::isReady() {
    if (!session.isActive) {
        return false;
    }
    
    return getCurrentLossPercent() >= session.targetLossPercent;
}

DailyRecord* DryingSessionManager::getRecord(int index) {
    if (index < 0 || index >= session.recordCount) {
        return nullptr;
    }
    
    return &session.records[index];
}

DailyRecord* DryingSessionManager::getLastRecord() {
    if (session.recordCount == 0) {
        return nullptr;
    }
    
    return &session.records[session.recordCount - 1];
}

int DryingSessionManager::getRecordCount() {
    return session.recordCount;
}

float DryingSessionManager::calculateAverageDailyLoss(int lastNDays) {
    if (session.recordCount < 2 || lastNDays < 1) {
        return 0.0f;
    }
    
    int startIndex = session.recordCount - lastNDays - 1;
    if (startIndex < 0) {
        startIndex = 0;
    }
    
    float totalLossPercent = 0.0f;
    int count = 0;
    
    for (int i = startIndex + 1; i < session.recordCount; i++) {
        float dayLoss = session.records[i - 1].lossPercent - session.records[i].lossPercent;
        totalLossPercent += abs(dayLoss);
        count++;
    }
    
    return count > 0 ? totalLossPercent / count : 0.0f;
}