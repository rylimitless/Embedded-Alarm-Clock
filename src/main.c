/*
Author: Ryheeme Donegan
Student ID: 620157506
Date: 25/10/2023
*/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define  LCD_DATA_PORT    PORTB
#define  LCD_DDR     DDRB
#define  LCD_PIN     PINB

#define RS PB1
#define RW PB2
#define EN PB0

#define BUZZER PB3

typedef struct {
  volatile uint8_t hours;
  volatile uint8_t minutes;
  volatile uint8_t seconds;
}ClockTime;

uint8_t twelve_hour_mode = 0;

ClockTime clockTime, alarmTime;

uint8_t buzzer_time = 0;


//Alarm state
uint8_t alarmState = 0;
uint8_t defaultState = 1;
uint8_t state = 0;

ISR(TIMER0_COMPA_vect);
ISR(INT0_vect);
ISR(INT1_vect);

uint8_t  four_bit_mode [] = {
  0x33, 
  0x32,
  0x28,
  0x0C,
  0x01,
  0x06,
}; 

uint8_t lcdIndex[]={
    0x80,
    0xC0,
    0x94,
    0xD4,
};

//Function prototypes

uint8_t checkTime();
void initLCD();
void lcdCommandwriter( unsigned char );
void lcdStringWriter(char *);
void lcdDataWriter(unsigned char );
void setCursor(uint8_t , uint8_t );
void clearDisplay();
void convertToString(int , char *);
void startClock();
void time(unsigned char);
void updateLCD();
void updateLCDAlarm();
void updateAlarmTime();

/**
 * The function setTime sets the hours, minutes, and seconds variables to the specified values and sets
 * the state variable to 1.
 * 
 * @param hrs The parameter "hrs" is of type uint8_t, which stands for unsigned 8-bit integer. It
 * represents the hours value that you want to set.
 * @param mins The "mins" parameter is of type uint8_t, which stands for unsigned 8-bit integer. It
 * represents the minutes value that you want to set for the time.
 * @param secs The "secs" parameter is of type uint8_t, which stands for unsigned 8-bit integer. It
 * represents the seconds value that you want to set for the time.
 * 
 * *Note : The fuction updates the global variables hours, minutes and seconds.
 * 
 */
void setTime(uint8_t hrs , uint8_t mins  , uint8_t secs , ClockTime * time){
  hours = hrs;
  minutes = mins;
  seconds = secs;
}

void defaultMode(){
  clearDisplay();
  setCursor(1, 5);
  // lcdStringWriter("12:00:00");
  updateLCD();

} 

void init(){
  _delay_ms(250);
  DDRD = 0x00;
  PORTD = 0xFF;
  
  initLCD();
  sei(); // enable global interrupts 
 
  MCUCR |= (1 << ISC11 | 1 << ISC01);
  GIMSK |= (1 << INT0 | 1 << INT1); // enable external interrupts
}

int main(){
  init();
  while(1){
    if(state){
      // startClock();
      updateLCD();
    }
    if(alarmState){
      updateLCDAlarm();
    }
    if(defaultState){
      defaultMode();
      updateLCDAlarm();
    }

    if(PIND & 1<<PD0){
      PORTB &= ~(1<<BUZZER);
      //Todo : turn on the buzzer
    }

    //Todo : turn off the buzzer
    _delay_ms(1000);
  }
  return 0;
}

/**
 * The function "updateLCD" displays the current time on an LCD display, with leading zeros if
 * necessary. The time is displayed in the format HH:MM:SS.
 */

void updateLCD(){
  // clearDisplay();
  setCursor(1,1);
  lcdStringWriter("Time");
  setCursor(1,7);

  time(clockTime.hours);
  lcdStringWriter(":");

  time(clockTime.minutes);
  lcdStringWriter(":");

  time(clockTime.seconds);

}

void updateLCDAlarm(){
  // clearDisplay();
  setCursor(2,1);
  lcdStringWriter("Alarm");
  setCursor(2,7);

  time(alarmTime.hours);
  lcdStringWriter(":");

  time(alarmTime.minutes);
  lcdStringWriter(":");

  time(alarmTime.seconds);
}

void time(unsigned char time){
  char str[10];
  convertToString(time, str);
  lcdStringWriter(str);
}

void clearDisplay(){
  lcdCommandwriter(0x01);
  _delay_us(2000);
}

/**
 * The function initializes the LCD by setting the data and command pins as outputs, sending
 * initialization commands to the LCD, and adding delays.
 */
void initLCD(){
  LCD_DDR = 0xFF;
  LCD_DATA_PORT &= ~(1<<EN);

  _delay_us(2000);
  for(uint8_t i = 0; i <= 4; i++){
    lcdCommandwriter( four_bit_mode[i] );
    _delay_us(100);
  }
  _delay_us(2000);
  lcdCommandwriter(0x06);
  _delay_us(100);
}

/**
 * The function lcdStringWriter writes a string of characters to an LCD display.
 * 
 * @param str A pointer to a character array (string) that contains the characters to be written to the
 * LCD.
 */

void lcdStringWriter( char * str ){
  while( *str ){
    lcdDataWriter( *str++ );
    _delay_us(100);
  }
  _delay_us(1000);
}

/**
 * Writes a command to the LCD display.
 * This is done in 4bit mode so only 4 bits are written at a time.
 * 
 * @param cmd The command to write.
 */

void lcdCommandwriter( unsigned char cmd ){
  LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmd & 0xF0);
  LCD_DATA_PORT &= ~(1<<RS);
  LCD_DATA_PORT &= ~(1<<RW);

  LCD_DATA_PORT |= (1<<EN);
  _delay_us(1);
  LCD_DATA_PORT &= ~(1<<EN);

  _delay_us(20);

  LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (cmd << 4);
  LCD_DATA_PORT |= (1<<EN);
  _delay_us(1);
  LCD_DATA_PORT &= ~(1<<EN);
}

/**
 * The function `lcdDataWriter` is used to write data to an LCD display.
 * 
 * @param data The parameter "data" is an unsigned char, which means it can hold values from 0 to 255.
 * It represents the data that needs to be written to the LCD.
 */
void lcdDataWriter(unsigned char data){
  LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data & 0xF0);
  LCD_DATA_PORT |= (1<<RS);
  LCD_DATA_PORT &= ~(1<<RW);
  LCD_DATA_PORT |= (1<<EN);
  _delay_us(1);
  LCD_DATA_PORT &= ~(1<<EN);
  LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data << 4);
  LCD_DATA_PORT |= (1<<EN);
  _delay_us(1);
  LCD_DATA_PORT &= ~(1<<EN);
}

void setCursor(uint8_t row , uint8_t col){
  unsigned char pos = lcdIndex[row-1] + col - 1;
lcdCommandwriter(pos);
_delay_us(100);
}

/**
 * The ISR (Interrupt Service Routine) increments the seconds, minutes, and hours variables, and resets
 * them to 0 when they reach their maximum values.
 * When hour is 24, the clock is reset to 00:00:00. Meaning start 
 * of new day.
 */

ISR(TIMER1_COMPA_vect){
  
  clockTime.seconds++;
  if(clockTime.seconds == 60){
    clockTime.seconds = 0;
    clockTime.minutes++;
  }
  if(clockTime.minutes == 60){
    clockTime.minutes = 0;
    clockTime.hours++;
  }
  if(clockTime.hours == 24){
    clockTime.hours = 0;
    clockTime.minutes = 0;
    clockTime.seconds = 0;
  }

  if(alarmState){
   if(checkTime()){

   }
  }

}

uint8_t checkTime(){
  return (alarmTime.hours==hours && alarmTime.minutes ==minutes
    && alarmTime.seconds == seconds);
}

/**
 * The function "startClock" sets up a timer to generate an interrupt every 1 second.
 * This is the basis of the clock 
 *
 */
void startClock(){
  TCCR1B = (1<<CS12|1<<CS10|1<<WGM12);
  OCR1A = 7812;
  TIMSK |= (1<<OCIE1A);
}

void startAlarmClock(){
  alarmState =1;
}


/**
 * The function converts an integer to a string representation.
 * 
 * @param num The num parameter is an the integer that needs to be converterd to a string.
 * @param str The `str` parameter is a pointer to a character array where the converted integer will be
 * stored as a string.
 * 
 * !Note : The function is designed to work with 2 digit numbers only. 
 */

void convertToString(int num, char * str) {
  str[0] = num / 10 + '0';
  str[1] = num % 10 + '0';
  str[2] = '\0';
}


void changetime(uint8_t mode){
  if(mode /*Clock*/){
  }
}

//alarm time
ISR(INT0_vect){
  uint8_t i = 7;
  TCCR1B = 0;
  lcdCommandwriter(0x0F);
  
  while(1){
    setCursor(1,i);
    _delay_ms(100);
    if(i==7){
      if(!(PIND & (1<<PD0))){
        hours+=1;
        if(hours>=24){
          hours = 0;
        }
        updateLCD();
      }
      if(!(PIND & (1<<PD1))){
        hours+=10;
        if(hours>=24){
          hours = 0;
        }
        updateLCD();
      }
    }

    if(!(PIND & 1<<PD4)){
      i = 10;
      setCursor(1,i);
    }

    if(i==10){
      if(!(PIND & (1<<PD0))){
        minutes+=1;
        if(minutes>=60){
          minutes = 0;
        }
        updateLCD();
      }
      if(!(PIND & (1<<PD1))){
        minutes+=10;
        if(minutes>=60){
          minutes = 0;
        }
        updateLCD();
      }
    }

    if(!(PIND & 1<<PD5)){
      i=13;
      setCursor(1,i);
    }

    if(i==13){
      if(!(PIND & (1<<PD0))){
        seconds+=1;
        if(seconds>=60){
          seconds = 0;
        }
        updateLCD();
      }
      if(!(PIND & (1<<PD1))){
        seconds+=10;
        if(seconds>=60){
          seconds = 0;
        }
        updateLCD();
      }
    }

    if(!(PIND & 1<<PD6)){
      startClock();
      lcdCommandwriter(0x0C);
      state = 1;
      break;
    }
    
  }
}


//clocktime
ISR(INT1_vect){
  uint8_t i = 7;
  // TCCR0B = 0;
  lcdCommandwriter(0x0F);
  
  while(1){
    setCursor(2,i);
    _delay_ms(100);
    if(i==7){
      if(!(PIND & (1<<PD0))){
        alarmTime.hours+=1;
        if(alarmTime.hours>=24){
          alarmTime.hours = 0;
        }
        updateLCDAlarm();
      }
      if(!(PIND & (1<<PD1))){
        alarmTime.hours+=10;
        if(alarmTime.hours>=24){
          alarmTime.hours = 0;
        }
        updateLCDAlarm();
      }
    }

    if(!(PIND & 1<<PD4)){
      i = 10;
      setCursor(2,i);
    }

    if(i==8){
      if(!(PIND & (1<<PD0))){
        alarmTime.minutes+=1;
        if(alarmTime.minutes>=60){
          alarmTime.minutes = 0;
        }
        updateLCDAlarm();
      }
      if(!(PIND & (1<<PD1))){
        alarmTime.minutes+=10;
        if(alarmTime.minutes>=60){
          alarmTime.minutes = 0;
        }
        updateLCDAlarm();
      }
    }

    if(!(PIND & 1<<PD5)){
      i=13;
      setCursor(2,i);
    }

    if(i==13){
      if(!(PIND & (1<<PD0))){
        alarmTime.seconds+=1;
        if(alarmTime.seconds>=60){
          alarmTime.seconds = 0;
        }
        updateLCDAlarm();
      }
      if(!(PIND & (1<<PD1))){
        alarmTime.seconds+=10;
        if(alarmTime.seconds>=60){
          alarmTime.seconds = 0;
        }
        updateLCDAlarm();
      }
    }

    if(!(PIND & 1<<PD6)){
      startAlarmClock();
      lcdCommandwriter(0x0C);
      alarmState = 1;
      break;
    } 
  }
}