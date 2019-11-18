#include <16f88.h>
#fuses INTRC,NOWDT,NOPROTECT,NOPUT,NOBROWNOUT,NOMCLR //MCLR PIN A5
#use delay(clock=8M)
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
char string_cont[5];
int8 decenas = 0.0, unidades = 0.0;


void send_at_command(char* answer, int16 wait_time, int8 lenghtstr);
int8 str_to_int(char string);

#int_ext // B0
void cruce_zero()
{
   if (lightbuf1 < 1 || lightbuf1 > 100) 
   {
      output_low(AC_pin1);
   }
    
   else if (lightbuf1 > 0 && lightbuf1 < 100) 
   {
      //operator1 = lightbuf1*(255.0/100.0);
      //val1 = 28.0*(255.0-operator1); // 28 es por el pulso de la interruptción
      delay_us(val1);
      output_high(AC_pin1);
      delay_us(100);
      output_low(AC_pin1);
   }
   /*if (lightbuf2 < 1 || lightbuf2 > 100) 
   {
      output_low(AC_pin2);
   }
    
   else if (lightbuf2 > 0 && lightbuf2 < 100) 
   {
      operator2 = lightbuf2*(255.0/100.0);
      val2 = 32.0*(255.0-operator2);
      delay_us(val2);
      output_high(AC_pin2);
      delay_us(100);
      output_low(AC_pin2);
   }
   if (lightbuf3 < 1 || lightbuf3 > 100) 
   {
      output_low(AC_pin3);
   }
    
   else if (lightbuf3 > 0 && lightbuf3 < 100) 
   {
      operator3 = lightbuf3*(255.0/100.0);
      val3 = 32.0*(255.0-operator3);
      delay_us(val3);
      output_high(AC_pin3);
      delay_us(100);
      output_low(AC_pin3);
   }
   */
   clear_interrupt(int_ext);
}
/*
#int_rda
void recibir()
{
   memset(&string_cont,0,sizeof(string_cont));
   send_at_command(string_cont,5,5);
   decenas = str_to_int(string_cont[2]);
   unidades = str_to_int(string_cont[3]);
   operator1 = lightbuf1*(255.0/100.0); 
   val1 = 28.0*(255.0-operator1); // 28 es por el pulso de la interruptción
   printf("%d",unidades);
   clear_interrupt(int_rda);
}
*/
void main()
{
   //enable_interrupts(int_rda);
   //clear_interrupt(int_rda);
   ext_int_edge(0,L_TO_H); 
   enable_interrupts(int_ext); 
   enable_interrupts(GLOBAL);
   output_low(AC_pin1);
   output_low(AC_pin2);
   output_low(AC_pin3);
   
   
   while(true)
   {
      memset(&string_cont,0,sizeof(string_cont));
      send_at_command(string_cont,15,5);
      decenas = str_to_int(string_cont[2]);
      unidades = str_to_int(string_cont[3]);
   
      if((decenas >= 0) && (decenas <= 9) && (unidades >= 0) && (unidades <= 9))
      {
         readBuffer = decenas*10.0 + unidades;
      }
      if(readBuffer < 100 && readBuffer > 0)
      {
         iterador = readBuffer;
         //printf("%d",iterador);
      }
      
      if(string_cont[0]=='1')
      {
         switch (string_cont[1]) 
         {
            case '5': lightbuf1 = iterador;//, printf("%d",lightbuf1);
               break;
         
            case '6': lightbuf2 = iterador;//, printf("%d",lightbuf2);
               break;
            
            case '7': lightbuf3 = iterador;//, printf("%d",lightbuf3);
               break;
         
            default: 
               break; 
         }
      }
      
      // Convertir string_cont[2,3] a int
      //sprintf(string_int,"%c%c",string_cont[2],string_cont[3]);
      //readBuffer = atoi(string_int);
      //printf("%d",decenas);
   }
}


int8 str_to_int(char string)
{
   int8 num = 0;
   switch(string)
   {
      case '0': num=0;
         break;
      case '1': num=1;
         break;
      case '2': num=2;
         break;
      case '3': num=3;
         break;
      case '4': num=4;
         break;
      case '5': num=5;
         break;
      case '6': num=6;
         break;
      case '7': num=7;
         break;
      case '8': num=8;
         break;
      case '9': num=9;
         break;  
      default: num=0;
         break;
   }
   return(num);
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

