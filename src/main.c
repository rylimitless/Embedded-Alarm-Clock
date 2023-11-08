#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define  LCD_PORT    PORTB
#define  LCD_DDR     DDRB
#define  LCD_PIN     PINB

#define RS PB1
#define RW PB2
#define EN PB0

#define BUZZER PB3

typedef struct {
  char * identifier;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t row;
  uint8_t isPM;
}ClockTime;


ClockTime clockTime={"T",12,0,0,1,0}, 
          alarmTime={"A",0,0,0,2,0};


volatile uint8_t twelve_hour_mode = 0;
volatile uint8_t hourflag = 24;
volatile uint8_t alarmState = 0;


volatile uint8_t setClockTimeFlag = 0;
volatile uint8_t setAlarmTimeFlag = 0;

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
void displayTime(unsigned char);
void paint(ClockTime *);
void setInternalTime(ClockTime *);

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
// void setTime(uint8_t hrs , uint8_t mins  , uint8_t secs , ClockTime * time){
//   hours = hrs;
//   minutes = mins;
//   seconds = secs;
// }

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
  startClock();

  while(1){

     if(setClockTimeFlag) {
            setInternalTime(&clockTime);
            setClockTimeFlag = 0;
        }
        if(setAlarmTimeFlag) {
            setInternalTime(&alarmTime);
            setAlarmTimeFlag = 0;
        }
    
      paint(&clockTime);
      paint(&alarmTime);

    if(checkTime()){
      PORTB |= (1<<BUZZER);
    }

    //Todo : turn off the buzzer
    _delay_ms(1000);
  }
  return 0;
}


void paint(ClockTime *t_time){

  uint8_t row = t_time->row;


  setCursor(row,1);
  lcdStringWriter(t_time->identifier);

  setCursor(row,3);

  //ggg

  displayTime(t_time->hours);
  lcdStringWriter(":");
  displayTime(t_time->minutes);
  lcdStringWriter(":");
  displayTime(t_time->seconds);

  if(twelve_hour_mode){
    if(t_time->isPM){
      setCursor(row,13);
      lcdStringWriter("PM");
  }else{
    setCursor(row,13);
    lcdStringWriter("AM");
  }
  }else{
    setCursor(row,13);
    lcdStringWriter("  ");
  }
  if(alarmState && t_time->row == 2){
    setCursor(row,16);
    lcdStringWriter("*");
  }else if(!alarmState && t_time->row == 2){
    setCursor(row,16);
    lcdStringWriter("-");

  }

  // paint(&clockTime);
}
void displayTime(unsigned char time){
  char str[3];
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
  LCD_PORT &= ~(1<<EN);

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
  LCD_PORT = (LCD_PORT & 0x0F) | (cmd & 0xF0);
  LCD_PORT &= ~(1<<RS);
  LCD_PORT &= ~(1<<RW);

  LCD_PORT |= (1<<EN);
  _delay_us(1);
  LCD_PORT &= ~(1<<EN);

  _delay_us(20);

  LCD_PORT = (LCD_PORT & 0x0F) | (cmd << 4);
  LCD_PORT |= (1<<EN);
  _delay_us(1);
  LCD_PORT &= ~(1<<EN);
}

/**
 * The function `lcdDataWriter` is used to write data to an LCD display.
 * 
 * @param data The parameter "data" is an unsigned char, which means it can hold values from 0 to 255.
 * It represents the data that needs to be written to the LCD.
 */
void lcdDataWriter(unsigned char data){
  LCD_PORT = (LCD_PORT & 0x0F) | (data & 0xF0);
  LCD_PORT |= (1<<RS);
  LCD_PORT &= ~(1<<RW);
  LCD_PORT |= (1<<EN);
  _delay_us(1);
  LCD_PORT &= ~(1<<EN);
  LCD_PORT = (LCD_PORT & 0x0F) | (data << 4);
  LCD_PORT |= (1<<EN);
  _delay_us(1);
  LCD_PORT &= ~(1<<EN);
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
  if(clockTime.hours > hourflag){
    if(twelve_hour_mode){
      
      clockTime.isPM = !clockTime.isPM;
      if(clockTime.hours>=13){
        clockTime.hours -= 12;
      }
    }
    else {
      clockTime.hours = 0;
    }
    clockTime.minutes = 0;
    clockTime.seconds = 0;
  }
}

uint8_t checkTime(){
  return ((alarmTime.hours==clockTime.hours )
    &&( alarmTime.minutes ==clockTime.minutes )
      && (alarmTime.isPM == clockTime.isPM) && alarmState==1);
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


void convertToString(int num, char * str) {
  str[0] = num / 10 + '0';
  str[1] = num % 10 + '0';
  str[2] = '\0';
}



ISR(INT0_vect){
  setClockTimeFlag = 1;
  return;
}

ISR(INT1_vect){
  while(1){
    PORTB &= ~(1<<BUZZER);
    if(!(PIND & (1<<PD0))){
      break;
    }
    if(!(PIND & (1<<PD6))){
      setAlarmTimeFlag = 1;
      break;
    }
  }
 return;
}

void setInternalTime(ClockTime * time){
  uint8_t i = 3;
  uint8_t j = 0;
  lcdCommandwriter(0x0F);
  
  while(1){
    setCursor(time->row,i);
    _delay_ms(100);
    if(i==3){
      if(!(PIND & (1<<PD0))){
        time->hours+=1;
        if(time->hours>hourflag){
          time->hours = 0;
        }

      }
      if(!(PIND & (1<<PD1))){
        j++;
        switch(j){
          case 1:{
            twelve_hour_mode = !twelve_hour_mode;
            hourflag = 12;
          } break;
   
          case 2:{
            time->isPM = !time->isPM;
          } break;

          case 3: {
            if(time->row==2){
              alarmState = !alarmState;
            } break;
            
          }
          case 4:{
            j=0;
            hourflag = 24;
          } break;
        }

    }

    }

    if(!(PIND & 1<<PD4)){
      i = 7;
      setCursor(time->row,i);
    }

    if(i==7){
      if(!(PIND & (1<<PD0))){
        time->minutes+=1;
        if(time->minutes>=60){
          time->minutes = 0;
        }
      }
      if(!(PIND & (1<<PD1))){
        time->minutes+=10;
        if(time->minutes>=60){
          time->minutes = 0;
        }
      }

    }

    if(!(PIND & 1<<PD5)){
      i=10;
      setCursor(time->row,i);
    }

    if(i==10){
      if(!(PIND & (1<<PD0))){
        time->seconds+=1;
        if(time->seconds>=60){
          time->seconds = 0;
        }
      }
      if(!(PIND & (1<<PD1))){
        time->seconds+=10;
        if(time->seconds>=60){
          time->seconds = 0;
        }
      }
    }

    if(!(PIND & 1<<PD6)){
      lcdCommandwriter(0x0C);

      break;
    }
    paint(time); 
  }
  return;  

}