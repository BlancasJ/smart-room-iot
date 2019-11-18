#include <16f88.h>
#fuses XT,NOWDT,NOPROTECT,NOPUT,NOBROWNOUT,NOMCLR //MCLR PIN A5
#use delay(clock=4M)
#use rs232(uart1, baud=9600)

float micro = 0.0, premicro = 0.0, resta = 0.0, point = 1000000;

#int_timer0
void micros()
{
   micro = micro + 800;
   set_timer0(155);
}

void main()
{
   enable_interrupts(GLOBAL);
   setup_timer_0(T0_INTERNAL | T0_DIV_1);
   enable_interrupts(int_timer0);
   set_timer0(155);
   output_low(pin_a1);
   while(true)
   {
      resta = micro-premicro;
      if(resta >= 1000000)
      {
         output_toggle(pin_a1);
         premicro = micro;
      }
   }
}

