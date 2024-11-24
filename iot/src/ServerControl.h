#ifndef server_control_h
#define server_control_h

#include <Arduino.h>
#include <WiFi.h>

#include "server_config.h"

#define NONE_MODE 0
#define SHUTDOWN_MODE 1
#define SUSPEND_MODE 2

typedef struct {
    bool serverIsOn = false;
    bool shutdownIsRunning = false;
    byte shutdownMode = NONE_MODE;
} ServerStatus;


uint64_t lcg_random(uint64_t seed, uint16_t salt, byte size) {
    uint64_t a = 1664525;
    uint64_t c = 1013904223;
    uint64_t current = seed;
    uint64_t max_number = pow(10, size);  // 10 raised to the power of size

    for (int i = 0; i < salt; i++) {
        current = (a * current + c) % max_number;
    }
    return current;
}


class ServerControl{
    private:

        IPAddress serverIp;
        const char* hostname;
        const int port;

        uint64_t getCurrentToken();
        bool resolveServerName();
        String getApiURL(const String &action, const uint64_t token);
    
    public:
        ServerStatus currentStatus;
        ServerControl(const char* hostname, int port);
        void callServerApi(const char* action);
        void loadServerStatus();
        void suspendServer();
        void shutdownServer();
        void cancelShutdownServer();

};

ServerControl::ServerControl(const char* hostname, int port):
    hostname(hostname),
    port(port),
    serverIp(IPAddress(0, 0, 0, 0)){

}

uint64_t ServerControl::getCurrentToken(){
    time_t now = time(NULL);
    long timestampMinutes = now / 60;
    uint64_t token = lcg_random(timestampMinutes, AUTHENTICATION_SALT, TOKEN_SIZE);
    return token;
}

bool ServerControl::resolveServerName(){
    for (int i = 0; i < 3; i++) {
        if (WiFi.hostByName(this->hostname, this->serverIp)) {
            return true;
        }
        delay(50);
    }
    return false;
}

String ServerControl::getApiURL(const String &action, const uint64_t token){
    bool useIp = false;
    if (this->serverIp == IPAddress(0, 0, 0, 0)){
        useIp = resolveServerName();
    }
    int bufferSize = 28 + action.length() + 20;
    char buffer[bufferSize];
    sprintf(buffer, "http://%s:%d/%s?token=%010llu",
        "192.168.0.78",
        SERVER_PORT,
        action.c_str(),
        token);
    return String(buffer);
}

void ServerControl::callServerApi(const char* action) {
    String url = getApiURL(action, getCurrentToken());
    Serial.print("URL: ");
    Serial.println(url);

    HTTPClient http;
    http.setTimeout(2000);
    http.begin(url);
    int httpResponseCode = http.GET();

    String response = http.getString();
    Serial.print("Code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
    http.end();
    ServerStatus serverStatus;
    serverStatus.serverIsOn = (httpResponseCode == 200);
    if (serverStatus.serverIsOn) {
        serverStatus.shutdownIsRunning = response.indexOf("SCHEDULED") != -1;
        if (response.indexOf("SUSPEND") != -1) {
            serverStatus.shutdownMode = SUSPEND_MODE;
        } else if (response.indexOf("SHUTDOWN") != -1) {
            serverStatus.shutdownMode = SHUTDOWN_MODE;
        } else {
            serverStatus.shutdownMode = NONE_MODE;
        }
    } else {
        serverStatus.shutdownIsRunning = false;
    }

    this->currentStatus = serverStatus;
}

void ServerControl::loadServerStatus(){
    this->callServerApi("status");
}

void ServerControl::suspendServer() {
    this->callServerApi("suspend");
}

void ServerControl::shutdownServer() {
    this->callServerApi("shutdown");
}

void ServerControl::cancelShutdownServer() {
    this->callServerApi("cancel");
}



#endif