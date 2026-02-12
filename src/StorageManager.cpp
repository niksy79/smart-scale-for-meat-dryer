#include "StorageManager.h"

StorageManager::StorageManager() {
}

bool StorageManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Storage] Failed to mount LittleFS");
        return false;
    }
    
    Serial.println("[Storage] LittleFS mounted successfully");
    printFileSystem();
    return true;
}

void StorageManager::format() {
    Serial.println("[Storage] Formatting LittleFS...");
    LittleFS.format();
    Serial.println("[Storage] Format complete");
}

bool StorageManager::saveSession(const DryingSession& session) {
    if (!saveSessionInfo(session)) {
        return false;
    }
    
    if (!saveRecords(session)) {
        return false;
    }
    
    Serial.println("[Storage] Session saved successfully");
    return true;
}

bool StorageManager::saveSessionInfo(const DryingSession& session) {
    StaticJsonDocument<256> doc;
    
    doc["active"] = session.isActive;
    doc["initialWeight"] = session.initialWeight;
    doc["targetLoss"] = session.targetLossPercent;
    doc["startTime"] = session.startTimestamp;
    doc["currentDay"] = session.currentDay;
    doc["recordCount"] = session.recordCount;
    doc["lastRecordTime"] = session.lastRecordTimestamp;  // Запази
    
    File file = LittleFS.open(SESSION_FILE, "w");
    if (!file) {
        Serial.println("[Storage] Failed to open session.json for writing");
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        Serial.println("[Storage] Failed to write session.json");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("[Storage] Session info saved");
    return true;
}

bool StorageManager::saveRecords(const DryingSession& session) {
    DynamicJsonDocument doc(4096); // До 60 записа
    JsonArray recordsArray = doc.createNestedArray("records");
    
    for (int i = 0; i < session.recordCount; i++) {
        JsonObject record = recordsArray.createNestedObject();
        record["day"] = session.records[i].day;
        record["timestamp"] = session.records[i].timestamp;
        record["weight"] = session.records[i].weight;
        record["loss"] = session.records[i].lossPercent;
        record["change"] = session.records[i].dayChange;
    }
    
    File file = LittleFS.open(RECORDS_FILE, "w");
    if (!file) {
        Serial.println("[Storage] Failed to open records.json for writing");
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        Serial.println("[Storage] Failed to write records.json");
        file.close();
        return false;
    }
    
    file.close();
    Serial.printf("[Storage] %d records saved\n", session.recordCount);
    return true;
}

bool StorageManager::loadSession(DryingSession& session) {
    // Зареждане на session info
    File file = LittleFS.open(SESSION_FILE, "r");
    if (!file) {
        Serial.println("[Storage] No session file found");
        session.isActive = false;
        return false;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[Storage] Failed to parse session.json: %s\n", error.c_str());
        return false;
    }
    
    session.isActive = doc["active"] | false;
    session.initialWeight = doc["initialWeight"] | 0.0f;
    session.targetLossPercent = doc["targetLoss"] | 40.0f;
    session.startTimestamp = doc["startTime"] | 0;
    session.currentDay = doc["currentDay"] | 0;
    session.recordCount = doc["recordCount"] | 0;
    session.lastRecordTimestamp = doc["lastRecordTime"] | 0;  // Зареди
    
    Serial.println("[Storage] Session info loaded");
    
    // Зареждане на records
    if (!loadRecords(session)) {
        session.recordCount = 0;
    }
    
    return true;
}

bool StorageManager::loadRecords(DryingSession& session) {
    File file = LittleFS.open(RECORDS_FILE, "r");
    if (!file) {
        Serial.println("[Storage] No records file found");
        return false;
    }
    
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[Storage] Failed to parse records.json: %s\n", error.c_str());
        return false;
    }
    
    JsonArray recordsArray = doc["records"];
    session.recordCount = 0;
    
    for (JsonObject record : recordsArray) {
        if (session.recordCount >= MAX_DAILY_RECORDS) break;
        
        session.records[session.recordCount].day = record["day"] | 0;
        session.records[session.recordCount].timestamp = record["timestamp"] | 0;
        session.records[session.recordCount].weight = record["weight"] | 0.0f;
        session.records[session.recordCount].lossPercent = record["loss"] | 0.0f;
        session.records[session.recordCount].dayChange = record["change"] | 0.0f;
        
        session.recordCount++;
    }
    
    Serial.printf("[Storage] %d records loaded\n", session.recordCount);
    return true;
}

void StorageManager::clearSession() {
    LittleFS.remove(SESSION_FILE);
    LittleFS.remove(RECORDS_FILE);
    Serial.println("[Storage] Session cleared");
}

bool StorageManager::addDailyRecord(DryingSession& session, float weight) {
    if (session.recordCount >= MAX_DAILY_RECORDS) {
        Serial.println("[Storage] Max records reached!");
        return false;
    }
    
    // Изчисляване на % загуба
    float totalLoss = session.initialWeight - weight;
    float lossPercent = (totalLoss / session.initialWeight) * 100.0f;
    
    // Изчисляване на промяна от предишния ден
    float dayChange = 0.0f;
    if (session.recordCount > 0) {
        dayChange = session.records[session.recordCount - 1].weight - weight;
    }
    
    // Добавяне на нов запис
    DailyRecord& record = session.records[session.recordCount];
    record.day = session.currentDay;
    record.timestamp = millis() / 1000; // Unix time (simplified)
    record.weight = weight;
    record.lossPercent = lossPercent;
    record.dayChange = dayChange;
    
    session.recordCount++;
    session.currentDay++;

    session.lastRecordTimestamp = record.timestamp; 
    
    Serial.printf("[Storage] Day %d recorded: %.1fg, Loss: %.1f%%, Change: %.1fg\n",
                  record.day, record.weight, record.lossPercent, record.dayChange);
    
    // Автоматично запазване
    return saveSession(session);
}

void StorageManager::printFileSystem() {
    Serial.println("[Storage] === File System Info ===");
    Serial.printf("Total: %d bytes\n", getTotalSpace());
    Serial.printf("Used: %d bytes\n", getUsedSpace());
    Serial.println("[Storage] === Files ===");
    
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
    
    Serial.println("[Storage] ===================");
}

size_t StorageManager::getUsedSpace() {
    return LittleFS.usedBytes();
}

size_t StorageManager::getTotalSpace() {
    return LittleFS.totalBytes();
}