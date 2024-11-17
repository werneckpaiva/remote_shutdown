#include <Arduino.h>

#include <ESPmDNS.h>
#include "wifi_config.h"
#include "local_wifi.h"
#include <WiFiUdp.h>
#include <WakeOnLan.h>

#define DEBUG_FAUXMO
#include <fauxmoESP.h>



#include "OneButton.h"

#include "Display.h"


const byte BTN_RIGHT_PIN = 0;
const byte BTN_LEFT_PIN = 14;
const byte PIN_BAT_VOLT = 4;

const byte PIN_TOUCH_INT = 16;
const byte PIN_TOUCH_RES = 21;

Display* display = NULL;


String SERVER_BASE_URL = "http://hawking.local:8090";
const char* ALEXA_DEVICE_NAME = "computer";
const char *ServerMACAddress = "70:4d:7b:61:d2:5c";

WiFiUDP UDP;
WakeOnLan WOL(UDP);


typedef struct {
    bool serverIsOn = false;
    bool shutdownIsRunning = false;
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

uint64_t getCurrentToken(){
    time_t now = time(NULL);
    long timestampMinutes = now / 60;
    uint64_t token = lcg_random(timestampMinutes, AUTHENTICATION_SALT, TOKEN_SIZE);
    return token;
}

String getApiURL(const String &action, const uint64_t token){
    int bufferSize = SERVER_BASE_URL.length() + action.length() + 20;
    char buffer[bufferSize];
    sprintf(buffer, "%s/%s?token=%010llu",
        SERVER_BASE_URL.c_str(),
        action.c_str(),
        token);
    return String(buffer);
}

ServerStatus callServerApi(const char* action) {
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
    } else {
        serverStatus.shutdownIsRunning = false;
    }
    
    return serverStatus;
}

ServerStatus getServerStatus(){
    return callServerApi("status");
}

ServerStatus shutdownServer() {
    return callServerApi("shutdown");
}

ServerStatus cancelShutdownServer() {
    return callServerApi("cancel");
}


fauxmoESP *alexa;

void handleAlexaAction(unsigned char device_id, const char * device_name, bool turnServerOn, unsigned char brightness, byte * rgb) {
    Serial.print("Turn: ");
    Serial.println(turnServerOn);
    ServerStatus serverStatus = getServerStatus();
    if (!serverStatus.serverIsOn){
        if (turnServerOn){
            display->println("Wake on LAN");
            WOL.sendMagicPacket(ServerMACAddress);
        }
    } else {
        if (turnServerOn){
            if (serverStatus.shutdownIsRunning){
                cancelShutdownServer();
            }
        } else {
            shutdownServer();
        }
    }
}

void updateStatus(){
    ServerStatus serverStatus = getServerStatus();
    alexa->setState(ALEXA_DEVICE_NAME, serverStatus.serverIsOn, serverStatus.serverIsOn?100:0);
    display->getTFT()->setTextColor(TFT_WHITE, TFT_RED);
    display->getTFT()->setTextDatum(MC_DATUM); 
    display->getTFT()->drawCentreString(serverStatus.serverIsOn ? "Server is on" : "Server is off", display->getTFT()->width()/2, 0, 1);
}

void initAlexa(){
    alexa = new fauxmoESP();

    alexa->addDevice(ALEXA_DEVICE_NAME);

    alexa->setPort(80); // required for gen3 devices
    alexa->enable(true);

    alexa->onSetState(handleAlexaAction);

    xTaskCreate(
        [](void* parameter) {
            for(;;){
                alexa->handle();
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        },
        "alexa_loop",
        (10 * 1024),
        NULL,
        1,
        NULL);

    updateStatus();
}


void initTime(){

    const char* ntpServer = "pool.ntp.org";
    const long  utcOffset_sec = 0; 
    const int   daylightOffset_sec = 0;

    configTime(utcOffset_sec, daylightOffset_sec, ntpServer);
}

OneButton* btnLeft;
OneButton* btnRight;

void btnLeftOnClick() {
}

void btnRightOnClick() {
    Serial.println("btnRightClick");
    display->wakeDisplay();
}

void initButtons(){
    btnLeft = new OneButton(BTN_RIGHT_PIN, true);
    btnRight = new OneButton(BTN_LEFT_PIN, true);
    btnLeft->attachClick(btnLeftOnClick);
    btnRight->attachClick(btnRightOnClick);
    xTaskCreate([](void* parameter) {
            for(;;){
                btnLeft->tick();
                btnRight->tick();
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        },
        "buttons_loop",
        (2 * 1024),
        NULL,
        1,
        NULL);
}

void initWakeOnLan(){
    WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms between. delay() is used between send packet function.
    Serial.print("Mask: ");
    Serial.println(WiFi.subnetMask());
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); 
}



void initStatus(){
    xTaskCreate([](void* parameter) {
            for(;;){
                updateStatus();
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
        },
        "monitoring_state",
        (5 * 1024),
        NULL,
        1,
        NULL);
}



void setup() {
    Serial.begin(115200);
    display = new Display();
    connectToWifi();
    initTime();
    initWakeOnLan();
    MDNS.begin("home-control");
    initAlexa();
    initButtons();
    initStatus();

}


void loop(){}