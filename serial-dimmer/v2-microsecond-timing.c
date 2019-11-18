#include <16f88.h>
#fuses XT,NOWDT,NOPROTECT,NOPUT,NOBROWNOUT,NOMCLR //MCLR PIN A5
#use delay(clock=4M)
#use rs232(uart1, baud=9600)
#include <string.h>
#include <stdlib.h>

#define AC_pin1 pin_a0
#define AC_pin2 pin_a1
#define AC_pin3 pin_a2

int8 iterador=0.0;
int8 readBuffer = 0.0;
int8 lightbuf1 = 0.0, lightbuf2 = 0.0, lightbuf3 = 0.0;
int8 operator1 = 0.0, operator2 = 0.0, operator3 = 0.0;
int16 val1 = 0.0, val2 = 0.0, val3 = 0.0;
char string_cont[5],string_int[2];
int16 micros = 0.0, pmicros1 = 0.0, pmicros2 = 0.0, pmicros3 = 0.0;
int16 resta=0.0;

void send_at_command(char* answer, int16 wait_time, int8 lenghtstr);

#int_timer0
void micro()
{
   micros = micros+40;
   //output_toggle(AC_pin2);
   set_timer0(245);
}

#int_ext // B0
void cruce_zero()
{
   set_timer0(245);
   clear_interrupt(int_ext);
}

void main()
{
   setup_timer_0(T0_INTERNAL | T0_DIV_1);
   set_timer0(245);
   enable_interrupts(int_timer0);
   ext_int_edge(0,L_TO_H); 
   enable_interrupts(int_ext); 
   enable_interrupts(GLOBAL);
   output_low(AC_pin1);
   output_low(AC_pin2);
   output_low(AC_pin3);
   
   while(true)
   {
      memset(string_cont,0,sizeof(string_cont));
      send_at_command(string_cont,15,5);
      sprintf(string_int,"%c%c",string_cont[2],string_cont[3]);
      readBuffer = atoi(string_int);
      
      if(readBuffer < 100 && readBuffer > 0)
      {
         iterador = readBuffer;
         printf("%d",iterador);
      }
      
      if(string_cont[0]=='1')
      {
         switch (string_cont[1]) 
         {
            case '5': lightbuf1 = iterador, printf("%d",lightbuf1), operator1 = lightbuf1*(255.0/100.0), val1 = 32.0*(255.0-operator1);
               break;
         
            case '6': lightbuf2 = iterador, printf("%d",lightbuf2), operator2 = lightbuf2*(255.0/100.0), val2 = 32.0*(255.0-operator2);
               break;
            
            case '7': lightbuf3 = iterador, printf("%d",lightbuf3), operator3 = lightbuf3*(255.0/100.0), val3 = 32.0*(255.0-operator3);
               break;
         
            default: 
               break; 
         }
      }
      
      if(resta >= (micros-pmicros1))
      {
         output_high(AC_pin1);
         delay_us(10);
         output_low(AC_pin1);
         pmicros1 = micros;
      }
      if(val2 >= (micros - pmicros2))
      {
         output_high(AC_pin2);
         delay_us(10);
         output_low(AC_pin2);
         pmicros2 = micros;
      }
      if(val3 >= (micros - pmicros3))
      {
         output_high(AC_pin3);
         delay_us(10);
         output_low(AC_pin3);
         pmicros3 = micros;
      }
   }
}

// Esta funcion recibe como parametros el comando AT, ciertos valores para sacar un tiempo de espera, el pic esperara cierto tiempo la respuesta del ESP y regresa la respuesta de este
void send_at_command(char* answer, int16 wait_time, int8 lenghtstr)
{
   int8 i=0;
   int16 t1=0,t2=0,t_cont=0;

   //printf(at_command);
   //Lee la cadena de caracteres hasta que se acabe el tiempo o el número de caracteres de la cadena receptora
   //Para calcular el tiempo de espera se considera:
   //un reloj de 48MHz=> tc=20.8333ns;   4 pulsos de reloj por instrucción => ti=tc * 4 = 83,33ns
   //y que un loop del while utiliza aproximadamente 20 instrucciones de ensamblador => tl = ti * 20 = 1.66 us
   //para llegar a una decima de segundo se requieren: 0.1s/1.66us = 60,000 cuentas.
   while((t_cont < wait_time) && (i<(lenghtstr-1)))
   {
      if (kbhit()) 
      {
         answer[i]= getc(); 
         i++; 
      }
      t1++;
      IF (t1>5000)
      {
         t2++; t1=0;
         IF (t2>10) 
         {
            t_cont ++; 
            t2=0;
         }
      }
   }
   //Rellena el resto de la cadena con 0s para el fin de línea
   answer=0;
   for (i=i ;i<lenghtstr;i++)
   {
      answer[i]=0;
   }
}

