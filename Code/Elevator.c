#include <16F887.h>
#device adc = 10
#fuses XT,NOWDT,NOPROTECT,NOBROWNOUT,NOLVP,NOPUT,NOWRT,NODEBUG,NOCPD
#use delay(clock = 4M)
#use i2c(master, scl=pin_c3, sda=pin_c4, fast=400000, stream=SSD1306_STREAM)
#use rs232(UART1, BAUD=9600)
#use fast_io(B)
#use fast_io(C)
#include <SSD1306.c>

char received = 0;

#INT_RDA
void receive_command()
{
   received = getc();
}

// Driving in full step mode
// Dir = 1 AB, A'B, A'B', AB'
// Dir = 0 AB, AB', A'B', A'B

#DEFINE CCW          1
#DEFINE CW           0
#DEFINE CABIN_UP     1
#DEFINE CABIN_DOWN   0
#DEFINE DOOR_OPEN    1
#DEFINE DOOR_CLOSE   0

int step_index_door = 0;
int wave_mode[4] = {0b1000, 0b0100, 0b0010, 0b0001};

int full_step_mode[4] = {0b1010, 0b0110, 0b0101, 0b1001};
int step_index_cabin = 0;

void step_door(int dir)
{
   if (dir && step_index_door != 3)
      step_index_door++;
   else if (dir)
      step_index_door = 0;
   else if (!dir && step_index_door != 0)
      step_index_door--;
   else if (!dir)
      step_index_door = 3;

   output_b((full_step_mode[step_index_cabin] << 4) | wave_mode[step_index_door]);
}

void step_cabin(int dir)
{
   if (dir && step_index_cabin != 3)
      step_index_cabin++;
   else if (dir)
      step_index_cabin = 0;
   else if (!dir && step_index_cabin != 0)
      step_index_cabin--;
   else if (!dir)
      step_index_cabin = 3;
      
   output_b((full_step_mode[step_index_cabin] << 4) | wave_mode[step_index_door]);
}

// every step is about 0.4mm,
// distance between floors are like this:
// 4         3         2         1
//  -223.5mm- -222.0mm- -224.0mm-
//

#define POS_TOP (0xffff/2)

int16 position = POS_TOP;
int16 floor_pos[4] = {POS_TOP - (6696/4), POS_TOP - (4455/4), POS_TOP - (2235/4), POS_TOP};

void go_to_pos(int16 pos)
{
   if (pos != position)
   {
      int dir = pos > position ? CABIN_UP : CABIN_DOWN;
      if (dir == CABIN_UP)
      {
         step_cabin(dir);
         position++;
         delay_ms(50);
      }
      if (dir == CABIN_DOWN)
      {
         step_cabin(dir);
         position--;
         delay_ms(50);
      }
   }
   else
   {
      received = '!';
   }
}

void main()
{
   // I/O, UART and Display setup 
   set_tris_b(0x00);
   set_tris_c(0xff);
   
   SSD1306_Init(SSD1306_SWITCHCAPVCC, 0x78);
   SSD1306_ClearDisplay();
   
   setup_uart(9600);
   enable_interrupts(GLOBAL);
   enable_interrupts(INT_RDA);
   
   while (1)
   {
      if (received == 'U')    // Up
      {
         step_cabin(CABIN_UP);
         position++;
         delay_ms(35);
      }
      if (received == 'D')    // Down
      {
         step_cabin(CABIN_DOWN);
         position--;
         delay_ms(35);
      }
      if (received == 'O')   // Open
      {
         step_door(DOOR_OPEN);
         delay_us(2200);
      }  
      if (received == 'C')   // Close
      {
         step_door(DOOR_CLOSE);
         delay_us(2200);
      }
      if (received == 'P')
      {
         position = 0xffff / 2;
      }
      if (received == '1')
      {
         go_to_pos(floor_pos[0]);
      }
      if (received == '2')
      {
         go_to_pos(floor_pos[1]);
      }
      if (received == '3')
      {
         go_to_pos(floor_pos[2]);
      }
      if (received == '4')
      {
         go_to_pos(floor_pos[3]);
      }
      if (received == '!')
      {
         SSD1306_ClearDisplay();
         SSD1306_GotoXY(1, 1);
         printf(SSD1306_PutC, "Position %lu", position);
         received = '0';
      }
   }
}
