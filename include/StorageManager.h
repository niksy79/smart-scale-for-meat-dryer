#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define MAX_DAILY_RECORDS 60  // До 60 дни история

struct DailyRecord {
    uint8_t day;
    uint32_t timestamp;
    float weight;
    float lossPercent;
    float dayChange;
};

struct DryingSession {
    bool isActive;
    float initialWeight;
    float targetLossPercent;
    uint32_t startTimestamp;
    uint8_t currentDay;
     uint32_t lastRecordTimestamp; 
    
    DailyRecord records[MAX_DAILY_RECORDS];
    uint8_t recordCount;
};

class StorageManager {
public:
    StorageManager();
    
    bool begin();
    void format();
    
    // Сесия
    bool saveSession(const DryingSession& session);
    bool loadSession(DryingSession& session);
    void clearSession();
    
    // Дневен запис
    bool addDailyRecord(DryingSession& session, float weight);
    
    // Статистика
    void printFileSystem();
    size_t getUsedSpace();
    size_t getTotalSpace();

private:
    const char* SESSION_FILE = "/session.json";
    const char* RECORDS_FILE = "/records.json";
    
    bool saveSessionInfo(const DryingSession& session);
    bool saveRecords(const DryingSession& session);
    bool loadRecords(DryingSession& session);
};

#endif