
/* ELECTRONOOBS portable soldering iron version 1.0
 *  Tutorial: http://www.electronoobs.com/eng_arduino_tut32.php
 *  Read all the comments in this code!
*/
#include <Arduino.h>
#include <max6675.h> //http://www.ELECTRONOOBS.com/eng_arduino_max6675.php
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>  //http://www.ELECTRONOOBS.com/eng_arduino_Adafruit_GFX.php
#include <Adafruit_SSD1306.h> //http://www.ELECTRONOOBS.com/eng_arduino_Adafruit_SSD1306.php
#include "logo.h" // get the logo botmaps from the other file, keeps things clear!


///////////////

//OLED setup
#define OLED_RESET 8
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 5
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
///////////////

//  Inputs and outputs
int butL = 4;     //left button
int butR = 7;     //right button
int ktcSO = 9;    //SPI pins for MAX6675 sensor
int ktcCS = 8;
int ktcCLK = 13;
int mosfet = 3;   //MOSFET pwm pin

MAX6675 ktc(ktcCLK, ktcCS, ktcSO);


//Variables
float temperature_read = 0;
int logo_slide =128;
unsigned long previousMillis = 0;
float Delay=300;
int sleeplogo_count = 0;
bool sleepmode = true;
bool temp_change = false;
int sleep_out_count = 0;
float set_value = 250;
int temp_change_count = 0;
float min_temp = 200;
float max_temp = 480;
int both_pressed_count = 0;

//PID values
float P_pid = 0;
float I_pid = 0;
float D_pid = 0;
/////////////////////////////////
float kp = 18.8;
float ki = 0.02;
float kd = 1.4;
//////////////////////////////////
float kp_init = 18.8;
float ki_init = 0.02;
float kd_init = 1.2;
//////////////////////////////////
float kp_fine= 8.8;
float ki_fine = 0.42;
float kd_fine = 4.1;
//////////////////////////////////
float error = 0;
float error_percent = 0;
float previous_error = 0;
float PID_value = 0;

void setup() {
  Serial.begin(9600);
  pinMode(mosfet,OUTPUT);
  digitalWrite(mosfet,LOW);
  pinMode(butL,INPUT);
  pinMode(butR,INPUT);//Begin the display i2c omunication
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  delay(200);
  display.clearDisplay();

  while(logo_slide >= 0)
  {
    display.clearDisplay();
    display.drawBitmap(logo_slide, 0,  logo_EN, 128, 32, 1);
    display.display();
    logo_slide = logo_slide-2;
    delayMicroseconds(700);
  }
  display.drawBitmap(0, 0,  logo_EN, 128, 32, 1);
  display.display();
  delay(500);
  display.clearDisplay();
  display.drawBitmap(0, 0,  no_press_logo, 128, 32, 1);
  display.display();
}

void loop() {
  //Get out of sleepmode
  if((digitalRead(butL) || digitalRead(butR)) &&  sleepmode)
  {
    if(sleep_out_count > 1300)
    {
      sleepmode = false;
      sleep_out_count = 0;
      delay(100);
    }
    sleep_out_count = sleep_out_count + 1;
  }//end of dig read of buttons and SLEEPMODE activated


  if(digitalRead(butR)  &&  !digitalRead(butL))
  {
    if(sleep_out_count > 6500)
    {
      temp_change = true;
      digitalWrite(mosfet,LOW);
      sleep_out_count = 0;
      set_value = set_value + 10;
       if (set_value > max_temp)
      {
        set_value = max_temp;
      }

      display.clearDisplay();
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.setCursor(10,4);
      display.print(set_value);
      display.display();
    }
    sleep_out_count = sleep_out_count + 1;
  }//increase temp

  if(digitalRead(butL)  && !digitalRead(butR))
  {
    if(sleep_out_count > 6500)
    {
      temp_change = true;
      digitalWrite(mosfet,LOW);
      sleep_out_count = 0;
      set_value = set_value - 10;

      if (set_value < min_temp)
      {
        set_value = min_temp;
      }
      display.clearDisplay();
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.setCursor(10,4);
      display.print(set_value);
      display.display();
    }
    sleep_out_count = sleep_out_count + 1;
  }//decrease temp


  if(!digitalRead(butL) && !digitalRead(butR) && temp_change)
  {
    if(temp_change_count > 28000)
    {
      temp_change = false;
      temp_change_count = 0;
    }
    temp_change_count = temp_change_count + 1;
  }


  if(digitalRead(butL) && digitalRead(butR) && !sleepmode)
  {
    if(both_pressed_count > 30000)
    {
      sleepmode=true;
      digitalWrite(mosfet,LOW);
      display.clearDisplay();
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.setCursor(20,4);
      display.print("SLEEP");
      display.display();
      delay(2000);
      both_pressed_count=0;
    }
    both_pressed_count = both_pressed_count +1;
  }


  if(sleepmode)
  {
    if(sleeplogo_count > 3000 && sleeplogo_count < 3005)
    {
      display.clearDisplay();
      display.drawBitmap(0, 0,  press_logo, 128, 32, 1);
      display.display();
    }
    if(sleeplogo_count > 6000 && sleeplogo_count < 6005)
    {
      display.clearDisplay();
      display.drawBitmap(0, 0,  no_press_logo, 128, 32, 1);
      display.display();
      sleeplogo_count = 0;
    }
    sleeplogo_count = sleeplogo_count + 1;
  }//end sleepmode



  //Start PID code
  unsigned long currentMillis = millis();
  if(  (currentMillis - previousMillis >= Delay) &&  (!sleepmode && !temp_change)){
    previousMillis += Delay;
    if(!sleepmode)
    {
      temperature_read = (ktc.readCelsius());
      error = set_value - temperature_read + 3;
      error_percent = (error/set_value)*100.0;

      P_pid = kp * error;
      I_pid = I_pid + (ki * error);
      D_pid =  kd*((error - previous_error)/(Delay/1000));
      PID_value = P_pid + I_pid + D_pid;




      if(PID_value > 255)
      {
        PID_value = 255;
      }

       if(PID_value < 1)
      {
        PID_value = 1;
      }
/*
      if(error_percent > 20)
      {
        analogWrite(mosfet,250);
      }
      */

        analogWrite(mosfet,PID_value);


      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.print("Set: ");
      display.setCursor(60,0);
      display.print(set_value,1);
      display.setCursor(0,18);
      display.print("Temp: ");
      display.setCursor(60,18);
      display.print(temperature_read- 5,1);
      display.display();    //This functions will display the data

      previous_error = error;
    }



  }//end current milis




}//end void loop
