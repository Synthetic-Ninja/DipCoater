#include <Arduino.h>
#include <GyverButton.h>
#include "modes.h"
#include "logger.h"

// НАСТРОЙКИ ДРАЙВЕРА ШАГОВИКА
#define STEP_PIN 14
#define DIR_PIN 12
#define ENABLE_PIN 27
#define STEPS_PER_MM 800

// НАСТРОЙКИ КНОПОК
#define STOP_BUTTON_PIN 32
#define ACCEPT_BUTTON_PIN 33
#define UP_BUTTON_PIN 25
#define DOWN_BUTTON_PIN 26


Stepper* stepper = new Stepper{STEP_PIN, DIR_PIN, ENABLE_PIN, STEPS_PER_MM, false, true};
GButton* stop_btn = new GButton{STOP_BUTTON_PIN};
GButton* accept_btn = new GButton{ACCEPT_BUTTON_PIN};
GButton* up_btn = new GButton{UP_BUTTON_PIN};
GButton* down_btn = new GButton{DOWN_BUTTON_PIN};
LiquidCrystal_I2C* lcd = new LiquidCrystal_I2C;
Logger* logger = new Logger(LoggerLevel::INFO);


class Program
{
  public:
    Program()
    {
      modes[0] = new AutomaticMode{stop_btn, accept_btn, down_btn, up_btn, lcd, "Automatic", stepper, logger};
      modes[1] = new ManualMode{stop_btn, accept_btn, down_btn, up_btn, lcd, "Manual", stepper, logger};
    }

    void run()
    {
        stepper->disable();
        while (true)
        {
          if (up_btn->isClick())
          {
            set_mode(1);
          }
          if (down_btn->isClick())
          {
            set_mode(-1);
          }
          if (accept_btn->isClick())
          {
            stop_btn->resetStates();
            up_btn->resetStates();
            accept_btn->resetStates();
            down_btn->resetStates();
            modes[current_mode]->run();
          }
        }
    }

  private:
    BaseMode* modes[2];
    int8_t current_mode = 0;
    int len_modes = 2;

    
    void set_mode(int direction)
    {
      current_mode += direction;
      if (current_mode > (len_modes - 1)){
        current_mode = 0;
      }
      else if (this->current_mode < 0){
        current_mode =  len_modes - 1;
      }
      logger->info("Current mode is " + modes[current_mode]->get_name());
    }
    
};


void setup()
{
  stop_btn->setTickMode(AUTO);
  accept_btn->setTickMode(AUTO);
  up_btn->setTickMode(AUTO);
  down_btn->setTickMode(AUTO);
  Serial.begin(115200);
  logger->info("I'M UP!");
  Program program = Program();
  program.run();
  
}

void loop()
{}