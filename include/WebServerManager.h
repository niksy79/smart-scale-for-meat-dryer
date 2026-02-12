#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "DryingSessionManager.h"

class WebServerManager {
public:
    WebServerManager();
    
    void init(DryingSessionManager* dryingMgr, float* currentWeightPtr);
    bool begin(const char* ssid, const char* password);
    void handle();
    bool isConnected();
    String getIPAddress();
    
private:
    WebServer server;
    DryingSessionManager* dryingPtr;
    float* currentWeightPtr;
    
    void setupRoutes();
    
    // Handler функции (като в работещия код)
    void handleMonitorPage();
    void handleHistoryPage();
    void handleStatusData();
    void handleHistoryData();
    
    // Helper функции
    String getStatusJSON();
    String getHistoryJSON();
};

#endif