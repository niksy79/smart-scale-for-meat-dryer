#ifndef DRYING_SESSION_MANAGER_H
#define DRYING_SESSION_MANAGER_H

#include <Arduino.h>
#include "StorageManager.h"

class DryingSessionManager {
public:
    DryingSessionManager(StorageManager& storage);
    
    void begin();
    
    // Управление на сесия
    bool startNewSession(float initialWeight, float targetLossPercent = 40.0f);
    bool recordDailyWeight(float weight);
    void endSession();
    
    // Статус
    bool isActive();
    DryingSession& getSession();
    
    // Статистика
    float getCurrentLossPercent();
    float getRemainingLossPercent();
    int estimateDaysRemaining();
    bool isReady();
    
    // История
    DailyRecord* getRecord(int index);
    DailyRecord* getLastRecord();
    int getRecordCount();

private:
    StorageManager& storage;
    DryingSession session;
    
    void initializeSession();
    float calculateAverageDailyLoss(int lastNDays = 3);
};

#endif