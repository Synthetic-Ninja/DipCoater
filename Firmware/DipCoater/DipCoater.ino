#include <Arduino.h>
#include <GyverButton.h>
#include "logger.h"

// НАСТРОЙКИ ЭКРАНА
#define TFT_DC 2
#define TFT_CS 5
#define INVERTED_COLORS
#include "display_modes.h"

// НАСТРОЙКИ ДРАЙВЕРА ШАГОВИКА
#define STEP_PIN 14
#define DIR_PIN 12
#define ENABLE_PIN 27
#define STEPS_PER_MM 800

// НАСТРОЙКИ КНОПОК
#define STOP_BUTTON_PIN 32
#define ACCEPT_BUTTON_PIN 33
#define UP_BUTTON_PIN 26
#define DOWN_BUTTON_PIN 25

#include "modes.h"



Stepper* stepper = new Stepper{STEP_PIN, DIR_PIN, ENABLE_PIN, STEPS_PER_MM, false, true};
GButton* stop_btn = new GButton{STOP_BUTTON_PIN};
GButton* accept_btn = new GButton{ACCEPT_BUTTON_PIN};
GButton* up_btn = new GButton{UP_BUTTON_PIN};
GButton* down_btn = new GButton{DOWN_BUTTON_PIN};
Logger* logger = new Logger(LoggerLevel::INFO);
Adafruit_ST7789* tft = new Adafruit_ST7789{TFT_CS, TFT_DC, -1};
MainDisplay* main_display = new MainDisplay{tft};


WorkerModeMenu* authomatic_mode_menu = new WorkerModeMenu(tft, "AUTHOMATIC MODE");
WorkerModeMenu* manual_mode_menu = new WorkerModeMenu(tft, "MANUAL MODE");
WorkerModeMenu* connection_mode_menu = new WorkerModeMenu(tft, "CONNECTION MODE");
InfoModeMenu* information_mode_menu = new InfoModeMenu(tft, "DEVICE INFO");

struct ProgramMode
{
  BaseMode* mode;
  char* name;
  const unsigned char * bitmap;
};


class Program
{
  public:
    Program(ProgramMode* modes, int modes_count, MainDisplay* main_display)
    {
      this->modes = modes;
      this->modes_count=modes_count;
      this->main_display = main_display;
    }

    void run()
    {
        render_menu();
        stepper->disable();
        while (true)
        {
          if (up_btn->isClick())
          {
            set_mode(-1);
          }
          if (down_btn->isClick())
          {
            set_mode(1);
          }
          if (accept_btn->isClick())
          {
            stop_btn->resetStates();
            up_btn->resetStates();
            accept_btn->resetStates();
            down_btn->resetStates();
            modes[current_mode].mode->run();
            render_menu();
          }
        }
    }

  private:
    ProgramMode* modes;
    int modes_count;
    int prev_mode = 0;
    int current_mode = 1;
    int past_mode = 2;
    MainDisplay* main_display;

    void render_menu()
    {
      main_display->render_main_menu(main_menu_bitmap_Bitmap);
      main_display->render_prev_item(modes[prev_mode].bitmap, modes[prev_mode].name);
      main_display->render_current_item(modes[current_mode].bitmap, modes[current_mode].name);
      main_display->render_past_item(modes[past_mode].bitmap, modes[past_mode].name);
    }

    void set_mode(int direction)
    {
      current_mode += direction;
      if (current_mode > modes_count)
      {
        current_mode = 0;
      }
      if (current_mode < 0)
      {
        current_mode = modes_count;
      }
      prev_mode = current_mode - 1;
      if (prev_mode < 0)
      {
        prev_mode = modes_count;
      }
      if (prev_mode > modes_count)
      {
        prev_mode = 0;
      }
      past_mode = current_mode + 1;
      if (past_mode > modes_count)
      {
        past_mode = 0;
      }
      if (past_mode < 0)
      {
        past_mode = modes_count;
      }
      
      main_display->render_prev_item(modes[prev_mode].bitmap, modes[prev_mode].name);
      main_display->render_current_item(modes[current_mode].bitmap, modes[current_mode].name);
      main_display->render_past_item(modes[past_mode].bitmap, modes[past_mode].name);

      logger->info("Current mode is " + String(modes[current_mode].name));
    }
    
};


void setup()
{
  stop_btn->setTickMode(AUTO);
  accept_btn->setTickMode(AUTO);
  up_btn->setTickMode(AUTO);
  down_btn->setTickMode(AUTO);
  tft->init(240, 320);
  tft->invertDisplay(1);
  tft->setRotation(1);
  Serial.begin(115200);
  logger->info("I'M UP!");

  ProgramMode authomatic = ProgramMode{
    new AutomaticMode{stop_btn, accept_btn, down_btn, up_btn, authomatic_mode_menu, "Automatic", stepper, logger},
    "AUTHOMATIC MODE",
    robot_bitmap_Bitmap
  };
  
  ProgramMode manual = ProgramMode{
    new ManualMode{stop_btn, accept_btn, down_btn, up_btn, manual_mode_menu, "Manual", stepper, logger},
    "MANUAL MODE    ",
    hand_bitmap_Bitmap
  };

  ProgramMode connection = ProgramMode{
    new ConnetionMode{stop_btn, accept_btn, down_btn, up_btn, connection_mode_menu, "Connection", logger},
    "CONNECTION MODE",
    connection_bitmap_Bitmap
  };

  ProgramMode information = ProgramMode
  {
    new InformationMode{stop_btn, accept_btn, down_btn, up_btn, information_mode_menu, "Connection", logger},
    "DEVICE INFO",
    connection_bitmap_Bitmap
  };

  ProgramMode program_modes[4] = {authomatic, manual, connection, information};
  

  Program program = Program(program_modes, 3, main_display);
  program.run();
  
}

void loop()
{}