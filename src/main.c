#include <avr/io.h>
#define F_CPU 8000000UL

#include <util/delay.h>

#define  LCD_DATA_COMMAND_PORT    PORTB
#define  LCD_DATA_COMMAND_DDR     DDRB
#define  LCD_DATA_COMMAND_PIN     PINB

#define RS PB1
#define RW PB2
#define EN PB0

uint8_t  four_bit_mode [] = {
  0x33, // 4 bit mode, 2 line, 5x7 font
  0x32,
  0x28, // 4 bit mode, 2 line, 5x7 font
  0x0E,
  0x01,
  0x06,
}; 

void initLCD();
void lcdCommandwriter( unsigned char );
void lcdStringWriter(char *);
void lcdDataWriter(unsigned char );
void setCursor(uint8_t , uint8_t );
void clearDisplay();

void delay_ms( uint16_t ms ){
  _delay_ms( ms);
}

void delay_us( uint16_t us ){
  _delay_us( us);
}

int main(){

  initLCD();
  
  while(1){
    clearDisplay();
    setCursor(1,1);
  lcdStringWriter("Hello ");
  delay_ms(1000);
  clearDisplay();
  lcdStringWriter("Rachelle");
  delay_ms(1000);
  clearDisplay();
  lcdStringWriter("From Ry :)");
  delay_ms(1000);
  }
  return 0;
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
  lcdCommandwriter(0x0E);
  delay_us(100);
  lcdCommandwriter(0x01);
  delay_us(2000);
  lcdCommandwriter(0x06);
  delay_us(100);
}

void lcdStringWriter( char * str ){
  unsigned char i = 0;
while(str[i]!=0)
{
lcdDataWriter(str[i]);
i++;
}
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
  unsigned char firstCharAdr[]={0x80,0xC0,0x94,0xD4};
lcdCommandwriter(firstCharAdr[row-1] + col - 1);
delay_us(100);
}