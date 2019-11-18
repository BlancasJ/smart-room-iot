#include <16f88.h>
#fuses INTRC,NOWDT,NOPROTECT,NOPUT,NOBROWNOUT,NOMCLR //MCLR PIN A5
#use delay(clock=8M)
#use rs232(uart1, baud=9600)
#include <string.h>
#include <stdlib.h>

#define AC_pin1 pin_a0
#define HC_SET pin_a1

int8 readBuffer = 0.0, readBufferp = 0.0;
int8 operator1 = 0.0;
int16 val1 = 0.0, val1p = 0.0;
char string_cont[20];
int8 decenas = 0.0, unidades = 0.0;
int8 ID1 = 16.0;
int8 IDlsb = 0.0, IDmsb = 0.0, IDread = 0.0;
// Variables para leer el uart
int16 n = 0;
INT1 timeout_error;
INT8 timeout_error_count = 0;    

int8 str_to_int(char string);
void clear_mem(char* str_buff) ;
char timed_getc();
void read_uart(char* uart_buff);

#int_ext // B0
void cruce_zero()
{
   IDmsb = str_to_int(string_cont[0]);
   IDlsb = str_to_int(string_cont[1]);
   decenas = str_to_int(string_cont[2]);
   unidades = str_to_int(string_cont[3]);
   
   if((IDmsb >= 0) && (IDmsb <= 9) && (IDlsb >= 0) && (IDlsb <= 9))
   {
      IDread = IDmsb*10 + IDlsb;
      if((decenas >= 0) && (decenas <= 9) && (unidades >= 0) && (unidades <= 9))
      {
         readBuffer = decenas*10.0 + unidades;
         readBufferp = readBuffer;
      }
      else
      {
         readBuffer = readBufferp;
      }
      if(IDread == ID1)
      {
         operator1 = readBuffer*(255.0/100.0);
         val1 = 28.0*(255.0-operator1);
         val1p = val1; 
      }
      else
      {
         val1 = val1p;
      }
   }
   

   if (val1 <= 0.0) 
   {
      output_low(AC_pin1);
   }
    
   else if (val1 > 0.0 && val1 < 7140.0) 
   {
      delay_us(val1);
      output_high(AC_pin1);
      delay_us(10);
      output_low(AC_pin1);
   }
   clear_interrupt(int_ext);
}

#int_rda
void recibir()
{
   read_uart(string_cont);
   clear_interrupt(int_rda);
}

void main()
{
   enable_interrupts(GLOBAL);
   //setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
   //enable_interrupts(int_timer1);
   //clear_interrupt(int_timer1);
   //set_timer1(63000);
   enable_interrupts(int_rda);
   clear_interrupt(int_rda);
   ext_int_edge(0,L_TO_H); 
   enable_interrupts(int_ext); 
   output_low(AC_pin1);
   output_high(HC_SET);
   while(true)
   {

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
                                                
void clear_mem(char* str_buff) 
{
   for (int16 mem_loop = 0; mem_loop < 20; mem_loop++) 
   {
      if (str_buff[mem_loop] == '\0')
      break;           
      else
      str_buff[mem_loop] = '\0';
   }      
   n = 0;
}

char timed_getc() 
{
   long timeout;
   timeout_error = FALSE;
   timeout = 0;

   while(!kbhit() && (++timeout < 2000)) //0.5 second
   delay_us(10);

   if(kbhit())    
   return(fgetc());                     

   else
   {
      timeout_error = TRUE;
      return(0);
   }
}

void read_uart(char* uart_buff)
{
   int1 OK = False;
   int loop=0;
   do{
      loop++;
      timeout_error_count = 0;
      clear_mem(uart_buff);
      
      long long timeout;
      timeout = 0; 

/*      while(!kbhit() && (++timeout < 3000000)) //30 second         
      {       
         delay_us(10);   
         if (timeout % 3000 == 0) {
            timeout_error_count++;
            //lcd_gotoxy(10, 2);
            int pos = timeout_error_count % 4;
            if (pos == 0){
            printf(".");
            }
            if (pos == 1){                 
            printf(".");
            }
            if (pos == 2){
            printf(".");
            }
            if (pos == 3){
            printf(".\r\n");
            }
         }               
      }*/
      for (int mem_loop = 0; mem_loop < 20; mem_loop++) {
         uart_buff[n] = timed_getc();
         n++;
         if (n > 20) n = 0;
         if (timeout_error)
         break;
      }
      //delay_ms(1);
      break;
   }
   while(OK==FALSE);
}


