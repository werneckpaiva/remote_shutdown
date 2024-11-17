#ifndef display_h
#define display_h

#include <Arduino.h>
#include <TFT_eSPI.h>

class Display {
    private:
        static constexpr const byte LCD_POWER_ON_PIN = 15;
        static constexpr const byte LCD_BACKLIGHT_PIN = 38;
        static constexpr const byte DEFAULT_WAKE_SEC = 5;

        TaskHandle_t displayTimerHandle = NULL;

        TFT_eSPI* display;

    public:
        Display();
        void wakeDisplay();
        TFT_eSPI* getTFT();

        void println(const char c[]);
};

Display::Display(){
    pinMode(Display::LCD_BACKLIGHT_PIN, OUTPUT);
    pinMode(Display::LCD_POWER_ON_PIN, OUTPUT);

    display = new TFT_eSPI();
    
    digitalWrite(LCD_POWER_ON_PIN, HIGH);
    display->init();
    display->setRotation(0);
    display->fillScreen(TFT_BLACK);
    display->setTextColor(TFT_WHITE, TFT_BLUE);
    display->setTextWrap(true);
    display->setTextSize(2);
    digitalWrite(LCD_BACKLIGHT_PIN, LOW);
}

TFT_eSPI* Display::getTFT(){
    return this->display;
}


void Display::wakeDisplay(){
    digitalWrite(Display::LCD_BACKLIGHT_PIN, HIGH);
    Serial.println("wake on display");

    if (displayTimerHandle != NULL){
        vTaskDelete(displayTimerHandle);
        displayTimerHandle = NULL;
    }

    xTaskCreate([](void* parameter) {
            for(;;){
                vTaskDelay(Display::DEFAULT_WAKE_SEC * 1000 / portTICK_PERIOD_MS);
                digitalWrite(Display::LCD_BACKLIGHT_PIN, LOW);
            }
            vTaskDelete(NULL); 
        },
        "display_timer",
        (5 * 1024),
        NULL,
        1,
        &displayTimerHandle);
}

void Display::println(const char c[]) {
    display->setTextColor(TFT_WHITE, TFT_BLUE);
    display->println(c);
    this->wakeDisplay();
}


#endif