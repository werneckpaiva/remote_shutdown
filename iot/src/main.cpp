#include <Arduino.h>

#include <ESPmDNS.h>
#include "wifi_config.h"
#include "local_wifi.h"

#define DEBUG_FAUXMO
#include <fauxmoESP.h>

#include <TFT_eSPI.h>

#include "OneButton.h"

const byte LCD_POWER_ON_PIN = 15;
const byte LCD_BACKLIGHT_PIN = 38;

const byte BTN_RIGHT_PIN = 0;
const byte BTN_LEFT_PIN = 14;
const byte PIN_BAT_VOLT = 4;

const byte PIN_TOUCH_INT = 16;
const byte PIN_TOUCH_RES = 21;


String SERVER_BASE_URL = "http://hawking.local:8090";
const char* SERVER_NAME = "home-control";

TFT_eSPI* display;

void initDisplay(){
    pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
    pinMode(LCD_POWER_ON_PIN, OUTPUT);

    display = new TFT_eSPI();
    
    digitalWrite(LCD_POWER_ON_PIN, HIGH);
    display->init();
    display->setRotation(0);
    display->fillScreen(TFT_BLACK);
    digitalWrite(LCD_BACKLIGHT_PIN, LOW);
}


TaskHandle_t displayTimerHandle = NULL;
void timedDisplay(String msg){
    digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
    display->fillScreen(TFT_BLUE);
    display->setTextColor(TFT_WHITE, TFT_BLUE); 
    display->drawCentreString(msg, 80, 14, 2);

    if (displayTimerHandle != NULL){
        vTaskDelete(displayTimerHandle);
        displayTimerHandle = NULL;
    }

    xTaskCreate([](void* parameter) {
            for(;;){
                vTaskDelay(10000 / portTICK_PERIOD_MS);
                digitalWrite(LCD_BACKLIGHT_PIN, LOW);
            }
            vTaskDelete(NULL); 
        },
        "display_timer",
        (5 * 1024),
        NULL,
        1,
        &displayTimerHandle);

}

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
    Serial.println(url);

    HTTPClient http;
    http.setTimeout(2000);
    http.begin(url);
    int httpResponseCode = http.GET();
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
    http.end();
    ServerStatus serverStatus;
    serverStatus.serverIsOn = (httpResponseCode == 200);
    if (serverStatus.serverIsOn) {
        timedDisplay(response);
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
        alexa->setState(SERVER_NAME, false, 0);
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

void initAlexa(){
    alexa = new fauxmoESP();

    alexa->addDevice(SERVER_NAME);
    ServerStatus serverStatus = getServerStatus();
    alexa->setState(SERVER_NAME, serverStatus.serverIsOn, serverStatus.serverIsOn?100:0);

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
        (5 * 1024),
        NULL,
        1,
        NULL);
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
    Serial.println("btnLeftOnClick");
    timedDisplay("Left Clicked");
}

void btnRightOnClick() {
    Serial.println("btnRightClick");
    timedDisplay("Right Clicked");
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


void setup() {
    Serial.begin(115200);
    initDisplay();
    connectToWifi();
    MDNS.begin("home-control");
    initTime();
    initAlexa();
    initButtons();

}


void loop(){}