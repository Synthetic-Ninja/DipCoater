class Button{
  public:
    void tick(){
      return;
    }
    
    bool click(){
      return false;
    }
};


class LiquidCrystal_I2C{
  public:
    void print(String a){
      Serial.println(a);
    }
};