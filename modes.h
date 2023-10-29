#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <GyverButton.h>
#include "stubs.h"
#include "stepper.h"


#define sd_cs 5


enum class ModeState{
  RUNNING,
  FORCE_STOPPING,
  STOPPED
};


enum class CommandState{
  CHOOSING,
  EXECUTING,
};


struct Command{
  uint8_t code;
  double arg1;
  double arg2;
  double arg3;
};


class BaseMode{
  public:
    BaseMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      LiquidCrystal_I2C* lcd,
      String name
    ){
      this->stop_btn = stop_btn;
      this->accept_btn = accept_btn;
      this->down_btn = down_btn;
      this->up_btn = up_btn;
      this->lcd = lcd;
      state = ModeState::STOPPED;
      this->name = name;
    }
    virtual void run() = 0;
    virtual void stop() = 0;
    virtual String get_name() = 0;
  
  protected:
    GButton* stop_btn;
    GButton* accept_btn;
    GButton* down_btn;
    GButton* up_btn;
    LiquidCrystal_I2C* lcd;
    ModeState state;
    CommandState command_state;
    String name;
};


class AutomaticMode: public BaseMode{
  public:
    AutomaticMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      LiquidCrystal_I2C* lcd,
      String name,
      Stepper* stepper
    ): BaseMode(stop_btn, accept_btn, down_btn, up_btn, lcd, name){
      this->stepper = stepper;
    }

    uint8_t get_command(String* command_for_check){
      if (*command_for_check == "UP"){
        return 0x01;
      }
      else if (*command_for_check == "DOWN"){
        return 0x02;
      }
      else if (*command_for_check == "IDLE_US"){
        return 0x03;
      }
      return 0x00;
    }
    
    int parse_json(File* json_file, Command** command_list){
      
      Serial.println("Reading from file");
      String json = json_file->readString();
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, json);
      if (error) {
        Serial.println("Deserialize Json failed");
        Serial.println(error.f_str());
        return -1;
      }

      if (!doc.containsKey("version")){
        Serial.println("Version not found");
        return -1;
      }
          
      Serial.print("Program version: ");
      Serial.println(doc["version"].as<String>());
          
      if (!doc.containsKey("program_body")){
        Serial.println("Program body not found");
        return -1;
      }

      if (!doc["program_body"].containsKey("commands_len")){
        Serial.print("commands_len not found");
        return -1;
      }

      int len_commands = doc["program_body"]["commands_len"];

      *command_list = new Command[len_commands];

      for (int i = 0; i < len_commands; i++){
        if (doc["program_body"]["commands_list"][i][0].isNull()){
          Serial.print("Invalid command");
          Serial.print(i);
          return -1;
        }
        String name = doc["program_body"]["commands_list"][i][0];
        
        uint8_t code = get_command(&name);

        if (code == 0x00){
           Serial.print("Command not found");
           Serial.print(i);
            return -1;
        }

        Serial.print("Adding command: ");
        Serial.println(name);
               
        double arg1;
        double arg2;
        double arg3;
        
        if (doc["program_body"]["commands_list"][i][1].isNull()){
          arg1 = 0;
        }
        else{
          arg1 = doc["program_body"]["commands_list"][i][1].as<double>();
        }
        
        if (doc["program_body"]["commands_list"][i][2].isNull()){
          arg2 = 0;
        }
        else{
          arg2 = doc["program_body"]["commands_list"][i][2].as<double>();
        }
        
        if (doc["program_body"]["commands_list"][i][3].isNull()){
          arg3 = 0;
        }
        else{
          arg3 = doc["program_body"]["commands_list"][i][3].as<double>();
        }

        (*command_list)[i].code = code;
        (*command_list)[i].arg1 = arg1;
        (*command_list)[i].arg2 = arg2;
        (*command_list)[i].arg3 = arg3;

      }

      return len_commands;
    }
    
    
    int get_program(Command** command_list){
      Serial.println("Initializing SD card...");
      if (!SD.begin(sd_cs)) {
        Serial.println("Initialization failed!");
        return -1;
      }
      Serial.println("Initialization success!");
      Serial.println("Opening program file");
      File myFile = SD.open("/program.json");
      if (myFile){
          int commands_count = parse_json(&myFile, command_list);

          if(commands_count < 1){
            myFile.close();
            return -1;
          }
          return commands_count;
      }
      else {
        Serial.println("Error opening file");
        myFile.close();
        return -1;
      }
    }

    void run(){
      Serial.println("Automatic Mode Runned...");
      Command *command_list = nullptr;
      int commands_count = get_program(&command_list);

      if (command_list == nullptr){
        Serial.println("NullPtr after get_program");
        return;
      }

      Serial.print("Commands count: ");
      Serial.println(commands_count);
      
      Serial.println("Running program...");
      
      
      if (commands_count > 0){
        int command_index = 0;
        stepper->enable();

        uint32_t program_start_time = micros();
        while(command_index < commands_count){
          stop_btn->tick();

          if (stop_btn->isPress()){
            Serial.println("Force stopping");
            break;
          }

          //Переменные нужные для работы режимов
          uint32_t idle_start;
          uint32_t target_position;
          
          switch (command_list[command_index].code){
            
            case 0x01:
              if (commands_state == CommandState::CHOOSING){
                Serial.println("Executing command UP");
                stepper->set_speed(stepper->get_steps_by_mm() * command_list[command_index].arg2);
                stepper->set_dir(1);
                stepper->reset_current_position();
                commands_state = CommandState::EXECUTING;
                target_position = int (command_list[command_index].arg1) * stepper->get_steps_by_mm();
              }
              else if (commands_state == CommandState::EXECUTING){
                if (target_position == stepper->get_current_position()){
                  commands_state = CommandState::CHOOSING;
                  command_index ++;
                }
                else{
                  stepper->step();
                }
              }
              break;

            case 0x02:
              if (commands_state == CommandState::CHOOSING){
                Serial.println("Executing command DOWN");
                stepper->set_speed(stepper->get_steps_by_mm() * command_list[command_index].arg2);
                stepper->set_dir(-1);
                stepper->reset_current_position();
                commands_state =CommandState::EXECUTING;
                target_position = int (command_list[command_index].arg1) * stepper->get_steps_by_mm();
              }
              else if (commands_state == CommandState::EXECUTING){
                if (target_position == abs(stepper->get_current_position())){
                  commands_state = CommandState::CHOOSING;
                  command_index ++;
                }
                else{
                  stepper->step();
                }
              }
              break;
          
            case 0x03:
              if (commands_state == CommandState::CHOOSING){
                Serial.println("Executing command IDLE");
                idle_start = micros();
                stepper->disable();
                commands_state =CommandState::EXECUTING;
              }
              else if (commands_state == CommandState::EXECUTING){
                if (micros() - idle_start >= command_list[command_index].arg1){
                  stepper->enable();
                  commands_state = CommandState::CHOOSING;
                  command_index ++;
                }
              }
          }
          
          
        }
        Serial.print("Program ended as: ");
        Serial.println(micros() - program_start_time);
        stepper->disable();
      
      }
      delete[] command_list;
    }

    void stop(){
      Serial.println("Automatic Mode Stopped");
    }

    String get_name(){
      return this->name;
    }
  private:
    Stepper* stepper;
    CommandState commands_state = CommandState::CHOOSING;
};


