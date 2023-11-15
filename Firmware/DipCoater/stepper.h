#pragma once

class Stepper{
  public:
    Stepper(
      int step_pin,
      int dir_pin,
      int enable_pin,
      int steps_per_mm,
      uint32_t max_steps_count,
      bool reverse_dir,
      bool reverse_en
      )
      {
        pinMode(step_pin, OUTPUT);
        pinMode(dir_pin, OUTPUT);
        pinMode(enable_pin, OUTPUT);
        this->step_pin = step_pin;
        this->dir_pin = dir_pin;
        this->reverse_dir = reverse_dir;
        this->reverse_en = reverse_en;
        this->enable_pin = enable_pin;
        this->direction = 1;
        this->position = 0;
        this->last_step_time = micros();
        this->steps_per_mm = steps_per_mm;
        this-> max_steps_count = max_steps_count;
    }
    
    int get_steps_by_mm()
    {
      return this->steps_per_mm;
    }
    
    double get_max_speed_by_mm()
    {
      return max_steps_count / steps_per_mm;
    }
    
    void set_speed(int steps_per_s)
    {
      this->driver_delay = 1000000.0 / steps_per_s;
    }


    void set_speed_by_mm(double mm_per_sec)
    {
      set_speed(mm_per_sec * get_steps_by_mm());
    }

    int get_speed()
    {
      return 1000000.0 / this->driver_delay;
    }

    void reset_current_position()
    {
      this->position = 0;
    }

    void set_current_position(int32_t position)
    {
      this->position=position;
    }

    int32_t get_current_position()
    {
      return this->position;
    }

    void step()
    {
      if ((micros() - this->last_step_time) >= this->driver_delay)
      {
        digitalWrite(this->step_pin, HIGH);
        digitalWrite(this->step_pin, LOW);
        this->last_step_time = micros();
        this->position += this->direction;
      }
    }

    void set_dir(int8_t direction)
    {
        this->direction = direction;
        if (direction != -1 && direction != 1)
        {
          return;
        }
        if (this->reverse_dir == true)
        {
          direction = 0 - direction;
        }
        if (direction == 1){
          digitalWrite(this->dir_pin, HIGH);
        }
        else
        {
          digitalWrite(this->dir_pin, LOW);
        }      

    }

    int8_t get_dir()
    {
      return this->direction;
    }

    void enable()
    {
      digitalWrite(this->enable_pin, HIGH ^ this->reverse_en);
    }

    void disable()
    {
      digitalWrite(this->enable_pin, LOW ^ this->reverse_en);
    }

  private:
    int step_pin;
    int dir_pin;
    int enable_pin;
    int steps_per_mm;
    uint32_t max_steps_count;
    bool reverse_dir;
    bool reverse_en;
    uint32_t driver_delay;
    int32_t position;
    int8_t direction;
    uint32_t last_step_time;

};