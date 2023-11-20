#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <GyverButton.h>
#include "stepper.h"
#include "logger.h"
#include "display_modes.h"
#include "components.h"

#define sd_cs 4


enum class ModeState
{
  RUNNING,
  FORCE_STOPPING,
  STOPPED
};


enum class CommandState
{
  CHOOSING,
  EXECUTING,
  FAILURE,
};


struct Command
{
  uint8_t code;
  double arg1;
  double arg2;
  double arg3;
};


class BaseMode
{
  public:
    BaseMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      WorkerModeMenu* tft,
      String name,
      Logger* logger
    )
    {
      this->stop_btn = stop_btn;
      this->accept_btn = accept_btn;
      this->down_btn = down_btn;
      this->up_btn = up_btn;
      this->tft = tft;
      state = ModeState::STOPPED;
      this->name = name;
      this->logger = logger;
    }
    virtual void run() = 0;
    virtual void stop() = 0;
    virtual String get_name() {return name;}
  
  protected:
    GButton* stop_btn;
    GButton* accept_btn;
    GButton* down_btn;
    GButton* up_btn;
    WorkerModeMenu* tft;
    ModeState state;
    String name;
    Logger* logger;
};


class AutomaticMode: public BaseMode
{
  public:
    AutomaticMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      GButton* up_limiter,
      GButton* down_limiter,
      WorkerModeMenu* tft,
      String name,
      Stepper* stepper,
      Logger* logger
    ): BaseMode(stop_btn, accept_btn, down_btn, up_btn, tft, name, logger)
    {
      this->stepper = stepper;
      this->up_limiter = up_limiter;
      this->down_limiter = down_limiter;
    }

    uint8_t get_command(String* command_for_check)
    {
      if (*command_for_check == "UP")
      {
        return 0x01;
      }
      else if (*command_for_check == "DOWN")
      {
        return 0x02;
      }
      else if (*command_for_check == "IDLE_US")
      {
        return 0x03;
      }
      return 0x00;
    }
    
    int parse_json(File* json_file, Command** command_list)
    {
      
      logger->info("Reading from file");
      tft->print_positive("Reading from file");
      
      String json = json_file->readString();
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, json);
      if (error) {
        logger->error("Deserialize Json failed" + String(error.f_str()));
        tft->print_negative("Deserialize Json failed");
        return -1;
      }

      if (!doc.containsKey("version"))
      {
        logger->error("Version not found");
        tft->print_negative("Version not found");

        return -1;
      }

      logger->info("Program version: " + doc["version"].as<String>());
      tft->print_positive("Program version: " + doc["version"].as<String>());

      if (!doc.containsKey("program_body"))
      {
        logger->error("Program body not found");
        tft->print_negative("Program body not found");

        return -1;
      }

      if (!doc["program_body"].containsKey("commands_len"))
      {
        logger->error("'commands_len' paramenter not found in rogram body");
        tft->print_negative("'commands_len' paramenter not found in rogram body");
        return -1;
      }

      int len_commands = doc["program_body"]["commands_len"];

      *command_list = new Command[len_commands];

      for (int i = 0; i < len_commands; i++){
        if (doc["program_body"]["commands_list"][i][0].isNull())
        {
          logger->error("Invalid command" + String(i));
          tft->print_negative("Invalid command" + String(i));
          return -1;
        }
        String name = doc["program_body"]["commands_list"][i][0];
        
        uint8_t code = get_command(&name);

        if (code == 0x00)
        {
          logger->error("Command not found: " + String(i));
          tft->print_negative("Command not found: " + String(i));

          return -1;
        }

        logger->debug("Adding command: " + name);

               
        double arg1;
        double arg2;
        double arg3;
        
        if (doc["program_body"]["commands_list"][i][1].isNull())
        {
          arg1 = 0;
        }
        else
        {
          arg1 = doc["program_body"]["commands_list"][i][1].as<double>();
        }
        
        if (doc["program_body"]["commands_list"][i][2].isNull())
        {
          arg2 = 0;
        }
        else
        {
          arg2 = doc["program_body"]["commands_list"][i][2].as<double>();
        }
        
        if (doc["program_body"]["commands_list"][i][3].isNull())
        {
          arg3 = 0;
        }
        else
        {
          arg3 = doc["program_body"]["commands_list"][i][3].as<double>();
        }

        (*command_list)[i].code = code;
        (*command_list)[i].arg1 = arg1;
        (*command_list)[i].arg2 = arg2;
        (*command_list)[i].arg3 = arg3;

      }

      return len_commands;
    }
    
    
    int get_program(Command** command_list)
    {
      logger->info("Initializing SD card...");
      tft->print_positive("Initializing SD card");
      if (!SD.begin(sd_cs))
      {
        logger->error("Initialization failed!");
        tft->print_negative("Initialization failed!");
        return -1;
      }
      logger->info("Initialization success!");
      tft->print_positive("Initialization success!");

      logger->info("Opening program file");
      tft->print_positive("Opening program file");

      File myFile = SD.open("/program.json");
      if (myFile)
      {
          int commands_count = parse_json(&myFile, command_list);
          if(commands_count < 1)
          {
            myFile.close();
            SD.end();
            return -1;
          }
          myFile.close();
          SD.end();
          return commands_count;
      }
      else
      {
        logger->error("Error opening file");
        tft->print_negative("Error opening file");
        myFile.close();
        SD.end();
        return -1;
      }
    }

    void run(){
      tft->render_main_screen();
      Command *command_list = nullptr;
      int commands_count = get_program(&command_list);

      if (command_list == nullptr)
      {
        logger->error("NullPtr after get_program");
        stop();
        return;
      }

      logger->debug("Commands count: " + String(commands_count));      
      logger->info("Running program...");
      tft->print_positive("Executing Programm...");
      
      up_limiter->resetStates();
      down_limiter->resetStates();
      
      if (commands_count > 0)
      {
        int command_index = 0;
        stepper->enable();
        uint32_t program_start_time = micros();
        commands_state = CommandState::CHOOSING;
        
        //Переменные нужные для работы режимов
        uint32_t idle_start;
        uint32_t target_position;

        while(command_index < commands_count)
        {

  
          if (stop_btn->isHold())
          {
            logger->info("Force stopping");
            stepper->disable();
            tft->print_negative("FORCE STOPPED");
            commands_state = CommandState::FAILURE;
          }

          if (commands_state == CommandState::FAILURE)
          {
              logger->info("Failured");
              break;
          }
      
          logger->debug("switching to command");
          switch (command_list[command_index].code)
          {
            
            case 0x01:
              if (commands_state == CommandState::CHOOSING)
              {
                logger->info("Executing command UP");
                tft->print_positive("EXEC: command [" + String(command_index) + "] - UP");
                stepper->set_speed(stepper->get_steps_by_mm() * command_list[command_index].arg2);
                stepper->set_dir(1);
                stepper->reset_current_position();
                commands_state = CommandState::EXECUTING;
                target_position = int (command_list[command_index].arg1) * stepper->get_steps_by_mm();
              }
              else if (commands_state == CommandState::EXECUTING)
              {
                if (target_position == stepper->get_current_position())
                {
                  commands_state = CommandState::CHOOSING;
                  command_index ++;
                }
                else
                {
                  if (up_limiter->isPress() || up_limiter->isHold())
                  {
                    tft->print_negative("STOPPED BY UP LIMITER");
                    commands_state = CommandState::FAILURE;
                  }
                  else
                  {
                    stepper->step();
                  }
                }
              }
              break;

            case 0x02:
              if (commands_state == CommandState::CHOOSING)
              {
                logger->info("Executing command DOWN");
                tft->print_positive("EXEC: command [" + String(command_index) + "] - DOWN");
                stepper->set_speed(stepper->get_steps_by_mm() * command_list[command_index].arg2);
                stepper->set_dir(-1);
                stepper->reset_current_position();
                commands_state =CommandState::EXECUTING;
                target_position = int (command_list[command_index].arg1) * stepper->get_steps_by_mm();
              }
              else if (commands_state == CommandState::EXECUTING)
              {
                if (target_position == abs(stepper->get_current_position()))
                {
                  commands_state = CommandState::CHOOSING;
                  command_index ++;
                }
                else
                {
                  if (down_limiter->isPress() || down_limiter->isHold())
                  {
                    tft->print_negative("STOPPED BY DOWN LIMITER");
                    commands_state = CommandState::FAILURE;
                  }
                  else
                  {
                    stepper->step();
                  }
                }
              }
              break;
          
            case 0x03:
              if (commands_state == CommandState::CHOOSING)
              {
                logger->info("Executing command IDLE");
                tft->print_positive("EXEC: command [" + String(command_index) + "] - IDLE");
                idle_start = micros();
                stepper->disable();
                commands_state =CommandState::EXECUTING;
              }
              else if (commands_state == CommandState::EXECUTING)
              {
                if (micros() - idle_start >= command_list[command_index].arg1)
                {
                  stepper->enable();
                  commands_state = CommandState::CHOOSING;
                  command_index ++;
                }
              }
          }
          
          
        }

        logger->info("Program ended as: " + String(micros() - program_start_time) + " us.");
        stepper->disable();
        if (commands_state != CommandState::FAILURE)
        {
          tft->print_positive("Program ended as: " + String(micros() - program_start_time) + " us.");
        }
  
      }
      delete[] command_list;
      stop();
    }

    void stop(){
      logger->info("Automatic Mode Stopped");
      while (true)
      {
        if (accept_btn->isClick())
        {
          return;
        }
      }
    }

  private:
    Stepper* stepper;
    GButton* up_limiter;
    GButton* down_limiter;
    CommandState commands_state = CommandState::CHOOSING;
};


enum class ManualModeState{
  MOVING_UP,
  MOVING_DOWN,
  SPEED_SELECT,
  IDLING,
  LIMITED_UP,
  LIMITED_DOWN
};


class ManualMode: public BaseMode{
  public:
    ManualMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      GButton* up_limiter,
      GButton* down_limiter,
      WorkerModeMenu* tft,
      String name,
      Stepper* stepper,
      Logger* logger
    ): BaseMode(stop_btn, accept_btn, down_btn, up_btn, tft, name, logger){
      this->up_limiter = up_limiter;
      this->down_limiter = down_limiter;
      this->stepper = stepper;
      max_stepper_speed = stepper->get_max_speed_by_mm();
    }
  void run(){
    up_limiter->resetStates();
    down_limiter->resetStates();
    stepper->enable();
    double speed = 1;
    double speed_pred = speed;
    stepper->set_speed_by_mm(speed);
    last_speed_set = millis();
    logger->info("Manual mode running");
    tft->render_main_screen();
    tft->print_positive("IDLING");
    ManualModeState modestate = ManualModeState::IDLING;
    while(true)
    {
      // Обработка событий кнопок
      
      if (stop_btn->isHold())
      {
          stepper->disable();
          logger->info("Force stopping");
          break;
            
      }
      
      if (stop_btn->isPress())
      {  
        if (modestate == ManualModeState::SPEED_SELECT)
        {
          modestate = ManualModeState::IDLING;
          speed = speed_pred;
          logger->info("SPEED ABORT");
          logger->debug("ABORTING SPEED: " + String(stepper->get_speed()));
          tft->print_positive("IDLING");
        } 
      }
      
      if ((up_limiter->isPress() || up_limiter->isHold()) && modestate == ManualModeState::MOVING_UP)
      {
        modestate = ManualModeState::LIMITED_UP;
        tft->print_negative("LIMITED UP");
      }

      if ((down_limiter->isPress() || down_limiter->isHold()) && modestate == ManualModeState::MOVING_DOWN)
      {
        modestate = ManualModeState::LIMITED_DOWN;
        tft->print_negative("LIMITED DOWN");
      }
      
      if (up_btn->isClick())
      {
        if (modestate == ManualModeState::SPEED_SELECT)
        {
          speed += 0.1;
          if (speed > max_stepper_speed)
          {
            speed = max_stepper_speed;
          }
          logger->info("Speed: " + String(speed));
          tft->print_positive("SPEED: " + String(speed));
        }
      }
      
      if (up_btn->isHold())
      {
        if (modestate == ManualModeState::SPEED_SELECT)
        {
          speed = calculate_speed(speed, 1);
        }

        else if (modestate == ManualModeState::IDLING || modestate == ManualModeState::LIMITED_DOWN)
        {
          stepper->set_dir(1);
          modestate = ManualModeState::MOVING_UP;
          tft->print_positive("MOVING UP");
        }  
        else if (modestate == ManualModeState::MOVING_UP)
        {
          stepper->step();
          logger->debug("Moving UP");
        }
      }
      
      if (down_btn->isClick())
      {
        if (modestate == ManualModeState::SPEED_SELECT)
        {
          speed -= 0.1;
          if (speed < 0)
          {
            speed = 0;
          }
          logger->info("Speed: " + String(speed));
          tft->print_positive("SPEED: " + String(speed));
        }
      }

      if (down_btn->isHold())
      {
        if (modestate == ManualModeState::SPEED_SELECT)
        {
          speed = calculate_speed(speed, -1);
          if (speed < 0)
          {
            speed = 0;
          }
        }
        else if (modestate == ManualModeState::IDLING || modestate == ManualModeState::LIMITED_UP)
        {
          stepper->set_dir(-1);
          modestate = ManualModeState::MOVING_DOWN;
          tft->print_positive("MOVING DOWN");
        }
        else if (modestate == ManualModeState::MOVING_DOWN)
        {
          stepper->step();
          logger->debug("Moving DOWN");
        }
      }

      if (accept_btn->isClick() && modestate == ManualModeState::SPEED_SELECT)
      {
        stepper->set_speed_by_mm(speed);
        logger->info("SPEED SAVING");
        logger->debug("ACCEPTING SPEED: " + String(stepper->get_speed()));
        modestate = ManualModeState::IDLING;
        tft->print_positive("IDLING");
      }
     
      if (accept_btn->isHolded() && modestate == ManualModeState::IDLING)
      {
        modestate = ManualModeState::SPEED_SELECT;
        speed_pred = speed;
        logger->info("SET_SPEED MODE");
        tft->print_positive("SPEED: " + String(speed));
      }

      if (
          (up_btn->isRelease() && modestate == ManualModeState::MOVING_UP) || 
          (down_btn->isRelease() && modestate == ManualModeState::MOVING_DOWN)
         )
      {
        modestate = ManualModeState::IDLING;
        tft->print_positive("IDLING");
        logger->debug("IDLING");
      }
    
    }

  }

  void stop(){}

  private:
    Stepper* stepper;
    ManualModeState modestate = ManualModeState::IDLING;
    uint32_t last_speed_set;
    GButton* up_limiter;
    GButton* down_limiter;
    double max_stepper_speed;

    double calculate_speed(double speed, double coef)
    {
      if (millis() - last_speed_set >= 500)
      {
        last_speed_set = millis();
        double calculated_speed = speed + coef;
        if (calculated_speed < 0)
        {
          calculated_speed = 0;
        }
        if (calculated_speed > max_stepper_speed)
        {
          calculated_speed = max_stepper_speed;
        }
        logger->info("Speed: " + String(calculated_speed));
        tft->print_positive("SPEED: " + String(calculated_speed));

        return calculated_speed;
      }
      return speed;
    }

};


enum class ConnetionModeState
{
  WAITING_CONNECTION_REQUEST,
  WAITING_ACK_BYTE,
  CONNETION_ESTABLISHED,
  WAITING_FIN_BYTE,
  CONNECTION_CLOSED,
  READY_TO_UPDATE_SETTINGS
};

class ConnetionMode: public BaseMode
{
  public:
    ConnetionMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      WorkerModeMenu* tft,
      String name,
      Logger* logger,
      EepromSettings* eeprom_settings
    ): BaseMode(stop_btn, accept_btn, down_btn, up_btn, tft, name, logger){
      state = ConnetionModeState::WAITING_CONNECTION_REQUEST;
      this->eeprom_settings = eeprom_settings;
    }
  
    void run()
    
    {
      tft->render_main_screen();
      tft->print_positive("Wait connection..");
      state = ConnetionModeState::WAITING_CONNECTION_REQUEST;
      uint8_t request_byte;
      Settings settings = Settings();
      int a;     
      while (true)
      {
        if (state == ConnetionModeState::CONNECTION_CLOSED)
        {
          break;
        }

        a = Serial.available();
        if (Serial.available() > 0)
        {
          switch (state)
          {
            case ConnetionModeState::WAITING_CONNECTION_REQUEST:
              request_byte = Serial.read();
              switch (request_byte)
              {
                case 0xF0:
                Serial.write(0xF1);
                Serial.flush();
                tft->print_positive("ACK BYTE SENDED");
                state = ConnetionModeState::WAITING_ACK_BYTE;
                break;

                default:
                  Serial.write(0x00);
                  Serial.flush();
                  tft->print_negative("WRONG REQUEST BYTE");
                  break;
              }
              break;
            
            case ConnetionModeState::WAITING_ACK_BYTE:
              request_byte = Serial.read();
              switch (request_byte)
              {
                case 0xF2:
                  tft->print_positive("CONNETION_ESTABLISHED");
                  state = ConnetionModeState::CONNETION_ESTABLISHED;
                  break;

                default:
                  Serial.write(0x00);
                  Serial.flush();
                  tft->print_negative("WRONG ACK BYTE");
                  break;
              }
              break;

            case ConnetionModeState::CONNETION_ESTABLISHED:
              request_byte = Serial.read();
              switch (request_byte)
              {
              case 0x11:
                tft->print_positive("UPDATING SETTINGS..>" + String(sizeof(Settings)));
                state = ConnetionModeState::READY_TO_UPDATE_SETTINGS;
                break;
              
              case 0x12:
                tft->print_negative("CONNECTION CLOSED");
                state = ConnetionModeState::CONNECTION_CLOSED;
                break;

              default:
                tft->print_negative("UNKNOWN COMMAND");
                break;
              }
              break;
            
            case ConnetionModeState::READY_TO_UPDATE_SETTINGS:
              if (receive(&settings))
              {
                eeprom_settings->set(settings);
                tft->print_positive("Params updated. Please restart device.");
              }
              else
              {
                tft->print_negative("INVALID STRUCT");
              }
              state = ConnetionModeState::CONNETION_ESTABLISHED;
              break;

            default:
              break;
          }
        }
      }
    stop();
    }

    void stop(){
      logger->info("Automatic Mode Stopped");
      while (true)
      {
        if (accept_btn->isClick())
        {
          return;
        }
      }
    }

    private:
      ConnetionModeState state;
      EepromSettings* eeprom_settings;

      bool receive(Settings* settings)
      {
        return (Serial.readBytes((char*)settings, sizeof(Settings)) == sizeof(Settings));
      }

};


class InformationMode: public BaseMode
{
  public:
    InformationMode(
      GButton* stop_btn,
      GButton* accept_btn,
      GButton* down_btn,
      GButton* up_btn,
      InfoModeMenu* tft,
      String name,
      Logger* logger,
      EepromSettings* eeprom_settings
    ): BaseMode(stop_btn, accept_btn, down_btn, up_btn, tft, name, logger){
      this->tft = tft;
      this->eeprom_settings = eeprom_settings;
    }

    void run()
    {
      tft->render_main_screen();
      
      Settings settings = eeprom_settings->get();
      //logger->info("SET: " + String(set1.stepper_mode));
      //Settings settings = Settings{200, 1400, 1, 0};
      uint8_t max_speed_mm = settings.max_steps_count / (settings.steps_per_mm * settings.driver_steps_division);
      String log_level;
      switch (logger->get_log_level())
      {
      case LoggerLevel::NO_LOG:
        log_level = "NO_LOG";
        break;
      
      case LoggerLevel::INFO:
        log_level = "INFO";
        break;

      default:
        log_level = "DEBUG";
        break;
      }

      tft->render_info(settings.steps_per_mm, max_speed_mm, settings.driver_steps_division, log_level);

      stop();
    }

    void stop()
    {
      logger->info("Information Mode Stopped");
      while (true)
      {
        if (accept_btn->isClick())
        {
          return;
        }
      }

    }


  private:
    InfoModeMenu* tft;
    EepromSettings* eeprom_settings;

};