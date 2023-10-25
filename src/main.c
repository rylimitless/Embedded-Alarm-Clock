#include <avr/io.h>
#define F_CPU 8000000UL

#include <util/delay.h>
#include <avr/interrupt.h>

#define  LCD_DATA_COMMAND_PORT    PORTB
#define  LCD_DATA_COMMAND_DDR     DDRB
#define  LCD_DATA_COMMAND_PIN     PINB

#define RS PB1
#define RW PB2
#define EN PB0

unsigned char seconds = 0;
unsigned char minutes = 0;
unsigned char hours = 12;
uint8_t state = 0;

ISR(TIMER0_COMPA_vect);
ISR(INT0_vect);
ISR(INT1_vect);

uint8_t  four_bit_mode [] = {
  0x33, // 4 bit mode, 2 line, 5x7 font
  0x32,
  0x28, // 4 bit mode, 2 line, 5x7 font
  0x0E,
  0x01,
  0x06,
}; 

  unsigned char lcdIndex[]={
    0x80,
    0xC0,
    0x94,
    0xD4,
};


void initLCD();
void lcdCommandwriter( unsigned char );
void lcdStringWriter(char *);
void lcdDataWriter(unsigned char );
void setCursor(uint8_t , uint8_t );
void clearDisplay();
void intToString(int , char *);
void initTimer();
void time(unsigned char);
void writeTime();

void delay_ms( uint16_t ms ){
  _delay_ms( ms);
}

void delay_us( uint16_t us ){
  _delay_us( us);
}

void defaultMode(){
  clearDisplay();
  setCursor(1,5);
  lcdStringWriter("12:00:00");

}

int main(){
  _delay_ms(250);
  DDRD = 0x00;
  PORTD = 0xFF;

  initLCD();
  sei();

	MCUCR |= (1 << ISC11 | 1 << ISC01);
	GIMSK |= (1 << INT0 | 1 << INT1);
  while(1){
    // writeTime();
    // delay_ms(1000);
    if(state){
      initTimer();
      writeTime();
    }
    else {
      defaultMode();
    }
    delay_ms(1000);
  }
  return 0;
}

void writeTime(){

  clearDisplay();
  setCursor(1,5);

  if(hours>=10){
    time(hours);
  }
  else{
    lcdStringWriter("0");
    time(hours);
  }

  if(minutes>=10){
    time(minutes);
  }
  else{
    lcdStringWriter("0");
    time(minutes);
  }
  if(seconds>=10){
    char str[10];
    intToString(seconds, str);
    lcdStringWriter(str);
  }
  else{
    lcdStringWriter("0");
    char str[10];
    intToString(seconds, str);
    lcdStringWriter(str);
  }

}

void time(unsigned char time){
  char str[10];
  intToString(time, str);
  lcdStringWriter(str);
  lcdStringWriter(":");
}

void clearDisplay(){
  lcdCommandwriter(0x01);
  delay_us(2000);
}

void initLCD(){
  LCD_DATA_COMMAND_DDR = 0xFF;
  LCD_DATA_COMMAND_PORT &= ~(1<<EN);
  delay_us(2000);
  // for(uint8_t i = 0; i < 3; i++){
  //   lcdCommandwriter( four_bit_mode[i] );
  //   delay_us((i == 4)? 2000: 100);
  // }

  lcdCommandwriter(0x33);
  delay_us(100);
  lcdCommandwriter(0x32);
  delay_us(100);
  lcdCommandwriter(0x28);
  delay_us(100);
  lcdCommandwriter(0x0C);
  delay_us(100);
  lcdCommandwriter(0x01);
  delay_us(2000);
  lcdCommandwriter(0x06);
  delay_us(100);
}

void lcdStringWriter( char * str ){
  while( *str ){
    lcdDataWriter( *str++ );
    delay_us(100);
  }
  delay_us(1000);
}

void lcdCommandwriter( unsigned char cmd ){
  LCD_DATA_COMMAND_PORT = (LCD_DATA_COMMAND_PORT & 0x0F) | (cmd & 0xF0);
  LCD_DATA_COMMAND_PORT &= ~(1<<RS);
  LCD_DATA_COMMAND_PORT &= ~(1<<RW);

  LCD_DATA_COMMAND_PORT |= (1<<EN);
  delay_us(1);
  LCD_DATA_COMMAND_PORT &= ~(1<<EN);

  delay_us(20);

  LCD_DATA_COMMAND_PORT = (LCD_DATA_COMMAND_PORT & 0x0F) | (cmd << 4);
  LCD_DATA_COMMAND_PORT |= (1<<EN);
  delay_us(1);
  LCD_DATA_COMMAND_PORT &= ~(1<<EN);
}

void lcdDataWriter(unsigned char data){
  LCD_DATA_COMMAND_PORT = (LCD_DATA_COMMAND_PORT & 0x0F) | (data & 0xF0);
  LCD_DATA_COMMAND_PORT |= (1<<RS);
  LCD_DATA_COMMAND_PORT &= ~(1<<RW);
  LCD_DATA_COMMAND_PORT |= (1<<EN);
  delay_us(1);
  LCD_DATA_COMMAND_PORT &= ~(1<<EN);
  LCD_DATA_COMMAND_PORT = (LCD_DATA_COMMAND_PORT & 0x0F) | (data << 4);
  LCD_DATA_COMMAND_PORT |= (1<<EN);
  delay_us(1);
  LCD_DATA_COMMAND_PORT &= ~(1<<EN);
}

void setCursor(uint8_t row , uint8_t col){
  unsigned char pos = lcdIndex[row-1] + col - 1;
lcdCommandwriter(pos);
delay_us(100);
}

ISR(TIMER1_COMPA_vect){
  seconds++;
  if(seconds == 60){
    seconds = 0;
    minutes++;
  }
  if(minutes == 60){
    minutes = 0;
    hours++;
  }
  if(hours == 24){
    hours = 0;
    minutes = 0;
    seconds = 0;
  }

}

void initTimer(){
  TCCR1B = (1<<CS12|1<<CS10|1<<WGM12);
	//Put appropriate value for OCR1A
	OCR1A = 7812;
	//Output Compare Interrupt Enabled
	TIMSK = (1<<OCIE1A);
}



void intToString(int num, char* str) {
    int i = 0;
    int sign = 0;

    // Handle negative numbers
    if (num < 0) {
        sign = 1;
        num = -num;
    }

    // Extract digits from the number
    do {
        str[i++] = num % 10 + '0';
        num /= 10;
    } while (num > 0);

    // Add sign character if necessary
    if (sign) {
        str[i++] = '-';
    }

    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }

    // Add null terminator
    str[i] = '\0';
}

ISR(INT0_vect){
  uint8_t i = 5;
  TCCR1B = 0;
  lcdCommandwriter(0x0F);
  
  while(1){
    setCursor(1,i);
    _delay_ms(100);
    if(i==5){
      if(!(PIND & (1<<PD0))){
        hours+=1;
        if(hours>=24){
          hours = 0;
        }
        writeTime();
      }
      if(!(PIND & (1<<PD1))){
        hours+=10;
        if(hours>=24){
          hours = 0;
        }
        writeTime();
      }
    }

    if(!(PIND & 1<<PD4)){
      i = 8;
      setCursor(1,i);
    }

    if(i==8){
      if(!(PIND & (1<<PD0))){
        minutes+=1;
        if(minutes>=60){
          minutes = 0;
        }
        writeTime();
      }
      if(!(PIND & (1<<PD1))){
        minutes+=10;
        if(minutes>=60){
          minutes = 0;
        }
        writeTime();
      }
    }

    if(!(PIND & 1<<PD5)){
      i=11;
      setCursor(1,i);
    }

    if(i==11){
      if(!(PIND & (1<<PD0))){
        seconds+=1;
        if(seconds>=60){
          seconds = 0;
        }
        writeTime();
      }
      if(!(PIND & (1<<PD1))){
        seconds+=10;
        if(seconds>=60){
          seconds = 0;
        }
        writeTime();
      }
    }

    if(!(PIND & 1<<PD6)){
      initTimer();
      lcdCommandwriter(0x0C);
      state = 1;
      break;
    }
    
  }

}

ISR(INT1_vect){
    initTimer();
    state = 1;

}

