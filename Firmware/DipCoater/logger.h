#pragma once
#include <Arduino.h>

enum class LoggerLevel
{
    DEBUG,
    INFO,
    NO_LOG
};


class Logger
{
    public:
        Logger(LoggerLevel log_level)
        {
            this->log_level = log_level;
        }
    
    void debug(String message)
    {
        switch (log_level)
        {
            case LoggerLevel::NO_LOG:
                break;
        
            case LoggerLevel::INFO:
                break;

            case LoggerLevel::DEBUG:
                
                Serial.println("__DEBUG__: " + message);
                
            default:
                break;
        }
    }
    void info(String message)
    {
         switch (log_level)
        {
            case LoggerLevel::NO_LOG:
                break;
        
            case LoggerLevel::INFO:
                Serial.println("__INFO__: " + message);

            case LoggerLevel::DEBUG:
                
                Serial.println("__INFO__: " + message);
                
            default:
                break;
        }
    }

    void error(String message)
    {
         switch (log_level)
        {
            case LoggerLevel::NO_LOG:
                break;
        
            case LoggerLevel::INFO:
                Serial.println("__ERROR__: " + message);

            case LoggerLevel::DEBUG:
                
                Serial.println("__ERROR__: " + message);
                
            default:
                break;
        }
    }

    LoggerLevel get_log_level()
    {
        return log_level;
    }
     

    private:
       LoggerLevel log_level;

};
