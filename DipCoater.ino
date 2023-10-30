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

void setup()
{
  Serial.begin(115200);
  Serial.println("i'm up");
  Logger* logger = new Logger(LoggerLevel::INFO);
  LiquidCrystal_I2C* lcd = new LiquidCrystal_I2C;
  //AutomaticMode* automatic = new AutomaticMode{stop_btn, accept_btn, down_btn, up_btn, lcd, "Automatic", stepper, logger};
  ManualMode* manual_mode = new ManualMode{stop_btn, accept_btn, down_btn, up_btn, lcd, "Automatic", stepper, logger};
  manual_mode->run();
  //automatic->run();

}

void loop()
{
  stop_btn->tick();
  accept_btn->tick();
  up_btn->tick();
  down_btn->tick();


  if (stop_btn->isPress())
  {
    Serial.println("STOP pressed");
  }
  if (accept_btn->isPress())
  {
    Serial.println("ACCEPT pressed");
  }
  if (down_btn->isPress())
  {
    Serial.println("DOWN pressed");
  }
  if (up_btn->isPress())
  {
    Serial.println("UP pressed");
  }

}