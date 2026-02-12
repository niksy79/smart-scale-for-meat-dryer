#include "WebServerManager.h"
#include "WebPages.h"

WebServerManager::WebServerManager() : server(80) {
    dryingPtr = nullptr;
    currentWeightPtr = nullptr;
}

void WebServerManager::init(DryingSessionManager* dryingMgr, float* currentWeight) {
    dryingPtr = dryingMgr;
    currentWeightPtr = currentWeight;
    
    Serial.println("[WebServer] Initialized with pointers");
}

bool WebServerManager::begin(const char* ssid, const char* password) {
    if (!dryingPtr || !currentWeightPtr) {
        Serial.println("[WebServer] ERROR: Not initialized! Call init() first.");
        return false;
    }
    
    Serial.println("\n[WebServer] Connecting to WiFi...");
    Serial.printf("[WebServer] SSID: %s\n", ssid);
    
    WiFi.mode(WIFI_OFF);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(1000);
    
    for (int attempt = 1; attempt <= 3; attempt++) {
        Serial.printf("[WebServer] Connection attempt %d/3\n", attempt);
        
        WiFi.begin(ssid, password);
        delay(1000);
        
        int attempts = 0;
        const int maxAttempts = 40;
        
        while (attempts < maxAttempts) {
            int strength = WiFi.RSSI();
            
            if (WiFi.status() == WL_CONNECTED && strength < 0) {
                Serial.println("\n[WebServer] WiFi connected!");
                Serial.print("[WebServer] IP Address: ");
                Serial.println(WiFi.localIP());
                Serial.printf("[WebServer] Signal strength (RSSI): %d dBm\n", strength);
                
                setupRoutes();
                server.begin();
                
                Serial.println("[WebServer] HTTP server started");
                return true;
            }
            
            delay(500);
            Serial.print(".");
            attempts++;
            
            if (attempts % 10 == 0) {
                Serial.printf("\n[WebServer] Status: %d, RSSI: %d\n", WiFi.status(), strength);
            }
        }
        
        if (WiFi.RSSI() == 0 && attempt < 3) {
            Serial.println("\n[WebServer] Signal strength is 0, retrying...");
            WiFi.disconnect(true);
            delay(1000);
        } else {
            break;
        }
    }
    
    Serial.println("\n[WebServer] WiFi connection FAILED!");
    return false;
}

void WebServerManager::setupRoutes() {
    // Синхронен WebServer - [this] работи перфектно!
    server.on("/", HTTP_GET, [this]() {
        handleMonitorPage();
    });
    
    server.on("/history", HTTP_GET, [this]() {
        handleHistoryPage();
    });
    
    server.on("/status/data", HTTP_GET, [this]() {
        handleStatusData();
    });
    
    server.on("/history/data", HTTP_GET, [this]() {
        handleHistoryData();
    });
}

// Handler функции
void WebServerManager::handleMonitorPage() {
    server.send(200, "text/html", MONITOR_PAGE);
}

void WebServerManager::handleHistoryPage() {
    server.send(200, "text/html", HISTORY_PAGE);
}

void WebServerManager::handleStatusData() {
    server.send(200, "application/json", getStatusJSON());
}

void WebServerManager::handleHistoryData() {
    server.send(200, "application/json", getHistoryJSON());
}

// Helper функции
String WebServerManager::getStatusJSON() {
    if (!dryingPtr || !currentWeightPtr) {
        return "{\"error\":\"Not initialized\"}";
    }
    
    // Статични променливи за кеширане
    static float lastSentWeight = 0.0f;
    static String cachedJson = "";
    static unsigned long lastUpdate = 0;
    static bool lastActiveState = false;
    
    const float WEIGHT_UPDATE_THRESHOLD = 1.0f;  // 1 грам буфер, както при дисплея
    const unsigned long FORCE_UPDATE_INTERVAL = 5000;  // Форсирано обновяване на 5 сек
    
    unsigned long now = millis();
    bool isActive = dryingPtr->isActive();
    float currentW = *currentWeightPtr;
    
    // Проверка дали трябва да обновим JSON
    bool needsUpdate = false;
    
    // 1. Форсирано обновяване на всеки 5 сек
    if (now - lastUpdate >= FORCE_UPDATE_INTERVAL) {
        needsUpdate = true;
    }
    
    // 2. Промяна на статус (активна/неактивна сесия)
    if (isActive != lastActiveState) {
        needsUpdate = true;
        lastActiveState = isActive;
    }
    
    // 3. Значителна промяна на теглото (>=1g)
    if (isActive && abs(currentW - lastSentWeight) >= WEIGHT_UPDATE_THRESHOLD) {
        needsUpdate = true;
    }
    
    // 4. Първо извикване (празен кеш)
    if (cachedJson.isEmpty()) {
        needsUpdate = true;
    }
    
    // Генериране на нов JSON само при нужда
    if (needsUpdate) {
        String json = "{";
        json += "\"active\":" + String(isActive ? "true" : "false") + ",";
        
        if (isActive) {
            DryingSession& session = dryingPtr->getSession();
            
            float realtimeLoss = 0.0f;
            if (session.initialWeight > 0) {
                float totalLoss = session.initialWeight - currentW;
                realtimeLoss = (totalLoss / session.initialWeight) * 100.0f;
            }
            
            json += "\"initialWeight\":" + String(session.initialWeight, 1) + ",";
            json += "\"currentWeight\":" + String(currentW, 1) + ",";
            json += "\"targetLoss\":" + String(session.targetLossPercent, 1) + ",";
            json += "\"currentLoss\":" + String(realtimeLoss, 1) + ",";
            json += "\"currentDay\":" + String(session.currentDay) + ",";
            json += "\"recordCount\":" + String(session.recordCount) + ",";
            
            int daysRemaining = dryingPtr->estimateDaysRemaining();
            json += "\"daysRemaining\":" + String(daysRemaining) + ",";
            json += "\"isReady\":" + String(dryingPtr->isReady() ? "true" : "false");
            
            lastSentWeight = currentW;  // Запази последното изпратено тегло
        } else {
            json += "\"initialWeight\":0,";
            json += "\"currentWeight\":0,";
            json += "\"targetLoss\":0,";
            json += "\"currentLoss\":0,";
            json += "\"currentDay\":0,";
            json += "\"recordCount\":0,";
            json += "\"daysRemaining\":0,";
            json += "\"isReady\":false";
            
            lastSentWeight = 0.0f;
        }
        
        json += "}";
        
        cachedJson = json;
        lastUpdate = now;
    }
    
    return cachedJson;
}

String WebServerManager::getHistoryJSON() {
    if (!dryingPtr) {
        return "{\"error\":\"Not initialized\"}";
    }
    
    String json = "{";
    json += "\"active\":" + String(dryingPtr->isActive() ? "true" : "false") + ",";
    json += "\"records\":[";
    
    if (dryingPtr->isActive()) {
        int count = dryingPtr->getRecordCount();
        for (int i = 0; i < count; i++) {
            DailyRecord* record = dryingPtr->getRecord(i);
            if (record) {
                if (i > 0) json += ",";
                json += "{";
                json += "\"day\":" + String(record->day) + ",";
                json += "\"weight\":" + String(record->weight, 1) + ",";
                json += "\"loss\":" + String(record->lossPercent, 1) + ",";
                json += "\"change\":" + String(record->dayChange, 1);
                json += "}";
            }
        }
    }
    
    json += "]}";
    return json;
}

void WebServerManager::handle() {
    server.handleClient();  // ВАЖНО: Извикваме в loop()
}

bool WebServerManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WebServerManager::getIPAddress() {
    return WiFi.localIP().toString();
}