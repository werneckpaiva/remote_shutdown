#include <Arduino.h>

#include <ESPmDNS.h>
#include "server_config.h"
#include "wifi_config.h"
#include "local_wifi.h"
#include <WiFiUdp.h>
#include <WakeOnLan.h>

#define DEBUG_FAUXMO
#include <fauxmoESP.h>


#include "OneButton.h"

#include "Display.h"
#include "ServerControl.h"

#define LOAD_GFXFF

#define TT1 &TomThumb


const byte BTN_RIGHT_PIN = 0;
const byte BTN_LEFT_PIN = 14;
const byte PIN_BAT_VOLT = 4;

const byte PIN_TOUCH_INT = 16;
const byte PIN_TOUCH_RES = 21;

Display* display = NULL;


WiFiUDP UDP;
WakeOnLan WOL(UDP);


fauxmoESP *alexa;

ServerControl *server1;

void displayServerStatus(ServerStatus serverStatus){
    const char* label;
    int bgColor;
    hs_color_t alexaHSColor;
    if (!serverStatus.serverIsOn){
        label = "Server is off";
        bgColor = TFT_DARKGREY;
        alexaHSColor.hue = 0; alexaHSColor.sat=0;
    } else if (serverStatus.shutdownIsRunning){
        label = "Ongoing Shutdown";
        if (serverStatus.shutdownMode == SHUTDOWN_MODE){
            Serial.println("SHUTDOWN_MODE");
            bgColor = TFT_RED;
            alexaHSColor.hue = 0; alexaHSColor.sat=254;
        } else if (serverStatus.shutdownMode == SUSPEND_MODE){
            Serial.println("SUSPEND_MODE");
            bgColor = 0xCCC0;
            alexaHSColor.hue = 10923; alexaHSColor.sat=254;
        } else {
            Serial.println("OTHER");
            bgColor = TFT_BLUE;
            alexaHSColor.hue = 43690; alexaHSColor.sat=254;
        }
    } else {
        label = "Server is on";
        bgColor = TFT_DARKGREEN;
        alexaHSColor.hue = 21845; alexaHSColor.sat=254;
    }
    alexa->setState(ALEXA_DEVICE_NAME, serverStatus.serverIsOn, serverStatus.serverIsOn?254:0, alexaHSColor);
    display->printTopHeader(label, bgColor);
}

void handleAlexaAction(unsigned char device_id, const char * device_name, bool turnServerOn, unsigned char brightness, hs_color_t hs_color) {
    Serial.print("Turn: ");
    Serial.println(turnServerOn);
    Serial.print("Brightness: ");
    Serial.println(brightness);
    Serial.print("Color: ");
    Serial.print(hs_color.hue);
    Serial.print(", ");
    Serial.println(hs_color.sat);
    server1->loadServerStatus();
    if (!server1->currentStatus.serverIsOn){
        if (turnServerOn){
            WOL.sendMagicPacket(ServerMACAddress);
        }
    } else {
        // Red - Shutdown
        if (hs_color.hue==0  && hs_color.sat==254) {
            server1->shutdownServer();
        // Yellow - Suspend
        } else if (hs_color.hue==10923  && hs_color.sat==254) {
            server1->suspendServer();
        // Any other color - cancel
        } else if (server1->currentStatus.shutdownIsRunning) {
            server1->cancelShutdownServer();
        }
    }
    displayServerStatus(server1->currentStatus);
}


void initAlexa(){
    alexa = new fauxmoESP();

    alexa->addDevice(ALEXA_DEVICE_NAME);

    alexa->setPort(80); // required for gen3 devices
    alexa->enable(true);

    alexa->onSetState(handleAlexaAction);

    xTaskCreatePinnedToCore(
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
        NULL,
        1);
}


void initTime(){
    const char* ntpServer = "pool.ntp.org";
    const long  utcOffset_sec = 0; 
    const int   daylightOffset_sec = 0;

    configTime(utcOffset_sec, daylightOffset_sec, ntpServer);
}

OneButton* btnLeft;
OneButton* btnRight;

void btnLeftOnLongPress() {
    display->wakeDisplay();
    if (server1->currentStatus.shutdownMode == NONE_MODE) {
        server1->suspendServer();
    } else {
        server1->cancelShutdownServer();
    }
    displayServerStatus(server1->currentStatus);
}

void btnRightOnClick() {
    Serial.println("btnRightClick");
    display->wakeDisplay();
}

void initButtons(){
    btnLeft = new OneButton(BTN_RIGHT_PIN, true);
    btnRight = new OneButton(BTN_LEFT_PIN, true);
    btnLeft->setLongPressIntervalMs(4000);
    btnLeft->attachDuringLongPress(btnLeftOnLongPress);
    btnRight->attachClick(btnRightOnClick);
    xTaskCreatePinnedToCore([](void* parameter) {
            for(;;){
                btnLeft->tick();
                btnRight->tick();
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
        },
        "buttons_loop",
        (5 * 1024),
        NULL,
        1,
        NULL,
        0);
}

void initWakeOnLan(){
    WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms between. delay() is used between send packet function.
    Serial.print("Mask: ");
    Serial.println(WiFi.subnetMask());
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
}



void initStatus(){
    xTaskCreatePinnedToCore([](void* parameter) {
            for(;;){
                server1->loadServerStatus();
                displayServerStatus(server1->currentStatus);
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
        },
        "monitoring_state",
        (5 * 1024),
        NULL,
        1,
        NULL,
        0);
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

    server1 = new ServerControl(SERVER_HOST, SERVER_PORT);
    initStatus();

}


void loop(){}