#pragma once

#include <EEPROM.h>

#define EEPROM_CHECK_BYTE 0
#define EEPROM_DEFAULT_VALUE 0xFA
#define EEPROM_CUSTOM_VALUE 0xFB
#define EEPROM_CHECK_BYTE 0
#define EEPROM_SETTINGS_BYTE 1
#define EEPROM_MAX_SIZE 4096 // Для esp32 4096


struct Settings
{
  uint32_t steps_per_mm;
  uint32_t max_steps_count;
  uint8_t driver_steps_division;
  uint8_t log_level;
};

class EepromSettings
{

    public:
        EepromSettings()
        {
            default_settings = Settings{300, 700, 1, 0};
        }
        
        bool is_default(){
            char checking_byte_value;
            EEPROM.begin(EEPROM_MAX_SIZE);
            EEPROM.get(EEPROM_CHECK_BYTE, checking_byte_value);
            return checking_byte_value == EEPROM_DEFAULT_VALUE; 
        }

        bool is_configured()
        {
            char checking_byte_value;
            EEPROM.begin(EEPROM_MAX_SIZE);
            EEPROM.get(EEPROM_CHECK_BYTE, checking_byte_value);
            return checking_byte_value == EEPROM_DEFAULT_VALUE || checking_byte_value == EEPROM_CUSTOM_VALUE;
        }
        
        Settings get()
        {
            Settings settings;
            EEPROM.begin(4096);
            EEPROM.get(EEPROM_SETTINGS_BYTE, settings);
            return settings;
        }

        void set(Settings input_settings)
        {
            EEPROM.begin(EEPROM_MAX_SIZE);
            EEPROM.put(EEPROM_CHECK_BYTE, EEPROM_CUSTOM_VALUE);
            EEPROM.put(EEPROM_SETTINGS_BYTE, input_settings);
            EEPROM.commit();
        }

        void set_default_settings()
        {
            EEPROM.begin(EEPROM_MAX_SIZE);
            EEPROM.put(EEPROM_CHECK_BYTE, EEPROM_DEFAULT_VALUE);
            EEPROM.put(EEPROM_SETTINGS_BYTE, default_settings);
            EEPROM.commit();
        }

    private:
        Settings default_settings;
};