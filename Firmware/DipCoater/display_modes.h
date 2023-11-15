#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "bitmaps.h"

// На реальном дисплее цвета инвертированные, поэтому определим их заранее

#ifdef INVERTED_COLORS
#define BLACK 0xFFFF
#define WHITE 0x0000
#define GREEN 0xf81f
#define RED 0x07ff

#else
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07e0
#define RED 0xf800

#endif


class MainDisplay
{
    public:
        MainDisplay(Adafruit_ST7789* display){
            this->display = display;
        }
        
        void render_main_menu(const unsigned char * bitmap)
        {
            display->fillScreen(BLACK);
            display->drawBitmap(0, 0, bitmap, 320, 240, WHITE);
        }
        
        void render_prev_item(const unsigned char * bitmap, char * name)
        {
        display->fillRect(10, 10, 280, 60, BLACK);
        display->drawBitmap(28, 24, bitmap, 32, 32, WHITE, BLACK);
        display->setCursor(80,35);
        display->setTextSize(2);
        display->setTextColor(WHITE, BLACK);
        display->print(name);
        }

        void render_current_item(const unsigned char * bitmap, char * name)
        {
        display->fillRect(10, 91, 280, 60, BLACK);
        display->drawBitmap(28, 105, bitmap, 32, 32, WHITE, BLACK);
        display->setCursor(80,115);
        display->setTextSize(2);
        display->setTextColor(WHITE, BLACK);
        display->print(name);
        }

        void render_past_item(const unsigned char * bitmap, char * name)
        {
        display->fillRect(10, 170, 280, 60, BLACK);
        display->drawBitmap(28, 185, bitmap, 32, 32, WHITE, BLACK);
        display->setCursor(80,195);
        display->setTextSize(2);
        display->setTextColor(WHITE, BLACK);
        display->print(name);
        }

    private:
        Adafruit_ST7789* display;
        const unsigned char * bitmap = main_menu_bitmap_Bitmap;
};


class BaseModeMenu
{
    public:
        BaseModeMenu(Adafruit_ST7789* display, String mode_name)
            {
                this->display = display;
                this->mode_name = mode_name;
            }
    
    protected:
        Adafruit_ST7789* display;
        String mode_name;
};





class WorkerModeMenu: public BaseModeMenu
{
    public:
        WorkerModeMenu(Adafruit_ST7789* display, String mode_name): BaseModeMenu(display, mode_name)
        {}

        void render_main_screen()
        {
            display->fillScreen(BLACK);
            display->setTextSize(2);
            display->setTextColor(WHITE, BLACK);
	        print_center(mode_name, 0, 40, WHITE, 2);
            print_center("STATUS:", 0, 90, WHITE, 2);
        }

        void print_center(String text, int x, int y, uint16_t color, uint8_t text_size)
        {
            int16_t  x1, y1;
            uint16_t w, h;
            display->getTextBounds(text, x, y, &x1, &y1, &w, &h);
            display->setCursor((320 - w) / 2, y);
            display->setTextSize(text_size);
            display->setTextColor(color, BLACK);
            display->print(text);
        }        
        
        void print_info_text(String text, uint16_t color)
        {
            display->fillRect(0, 134, 320, 105, BLACK);
            print_center(text, 26, 135, color, 2);    
        }
        
        void print_positive(String text)
        {
            print_info_text(text, GREEN);
        }

        void print_negative(String text)
        {
            print_info_text(text, RED);
        }

};


class InfoModeMenu: public WorkerModeMenu
{   
    public:
        InfoModeMenu(Adafruit_ST7789* display, String mode_name): WorkerModeMenu(display, mode_name)
        {}
        void render_main_screen()
        {
            display->fillScreen(BLACK);
            display->setTextSize(2);
            display->setTextColor(WHITE, BLACK);
	        print_center(mode_name, 0, 20, WHITE, 2);
        }

        void render_info(uint32_t steps_per_mm, double max_speed_mm , uint8_t stepper_mode, String log_lvl)
        {
	        
            print_center("STEPS PER MM: " + String(steps_per_mm), 0, 70, WHITE, 2);
            print_center("STEP DIVISION: " + String(stepper_mode), 0, 100, WHITE, 2);
            print_center("MAX SPEED: " + String(max_speed_mm) + " mm./s.", 0, 130, WHITE, 2);
            print_center("LOG LEVEL: " + log_lvl, 0, 160, WHITE, 2);
            print_center("Version: alpha-0.0.1", 0, 220,  WHITE, 1);
        }


};