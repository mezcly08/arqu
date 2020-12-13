/*
 * DHT11 Interfacing with PIC18F4550
 * http://www.electronicwings.com
 */


#include <pic18f4550.h>
#include <xc.h>
#include <stdio.h>
#include "Configuration_Header_File.h"
#include "LCD_16x2_8-bit_Header_File.h"

#define Data_Out LATC2              /* assign Port pin for data*/
#define Data_In PORTCbits.RC2       /* read data from Port pin*/
#define Data_Dir TRISCbits.RC2      /* Port direction */
#define _XTAL_FREQ 8000000          /* define _XTAL_FREQ for using internal delay */
#define LED_V PORTCbits.RC4         // Definimos el puerto del led verde
#define LED_A PORTCbits.RC5         // Definimos el puerto del led Amarillo
#define LED_R PORTCbits.RC6         // Definimos el puerto del led Rojo
#define Boton PORTBbits.RB4 


void DHT11_Start();
void DHT11_CheckResponse();
char DHT11_ReadData();
void onLEDS(char);




void main() 
{
    int agr= 0;
    int acum=0;
    TRISA = 0;
    Boton=0;
    char RH_Decimal,RH_Integral,T_Decimal,T_Integral, valor2;
    char Checksum;
    char value[10];    
    OSCCON = 0x72;      /* set internal oscillator with frequency 8 MHz*/
    //3TRISAbits.TRI3SA2 = 0;
    
    LCD_Init();         /* initialize LCD16x2 */
    ADCON1=0x0F;        /* this makes all pins as a digital I/O pins */ 
     
    while(1)
{   
    DHT11_Start();                  /* send start pulse to DHT11 module */
    DHT11_CheckResponse();          /* wait for response from DHT11 module */ 
       
    /* read 40-bit data from DHT11 module */
    RH_Integral = DHT11_ReadData(); /* read Relative Humidity's integral value */
    RH_Decimal = DHT11_ReadData();  /* read Relative Humidity's decimal value */
    T_Integral = DHT11_ReadData();    /* read Temperature's integral value */
    T_Decimal = DHT11_ReadData();   /* read Relative Temperature's decimal value */
    Checksum = DHT11_ReadData();    /* read 8-bit checksum value */
            
    while ( agr == 0){  
        onLEDS(T_Integral);//Encender los leds de advertencia
        valor2=T_Decimal;
        if(T_Integral <30 && acum == 0 ){
            PORTAbits.RA0 = 1;
            char buffer_TX[] = "LED 1 ENCENDIDO";

             for (int i = 0; i < 17; i++) {
            //  espera a que el registro de transmisi�n este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviar� a trav�s de TX.
            TXREG = buffer_TX[i];
        }
            T_Integral = T_Integral + 6;
            acum = 6;
        } else if (T_Integral <30 && acum == 6){
            PORTAbits.RA1 = 1;
            T_Integral = T_Integral + 6;
            acum = 12;
        }else if (T_Integral <30 && acum == 12){
            PORTAbits.RA2 = 1;
            T_Integral = T_Integral + 6;
            acum = 18;
        }else if (T_Integral <30 && acum == 18){
            PORTAbits.RA3 = 1;
            T_Integral = T_Integral + 6;
            acum = 24;
        }else if (T_Integral <30 && acum == 24){
            PORTAbits.RA4 = 1;
            T_Integral = T_Integral + 6;            
        } else 
            agr=1;
        /* convert humidity value to ascii and send it to display*/
        sprintf(value,"%d",RH_Integral);
        LCD_String_xy(0,0,value);
        sprintf(value,".%d ",RH_Decimal);
        LCD_String(value);
        LCD_Char('%');
        
        /* convert temperature value to ascii and send it to display*/
        sprintf(value,"%d",T_Integral);
        LCD_String_xy(1,0,value);
        sprintf(value,".%d",T_Decimal);
        LCD_String(value);
        LCD_Char(0xdf);
        LCD_Char('C');

        sprintf(value,"%d  ",Checksum);
        LCD_String_xy(0,8,"Humedad");
        LCD_String_xy(1,8,"Temp");    
              
        MSdelay(2000);
    }
    agr=0;
    MSdelay(500);         
    }      
    
     if (PORTBbits.RB4 == 1) // Si presionamos el boton conectado a RB0 ubicado en el PORTB el led conectado prendera
    {
        DHT11_Start();                  /* send start pulse to DHT11 module */
    DHT11_CheckResponse();          /* wait for response from DHT11 module */ 
       
    /* read 40-bit data from DHT11 module */
    RH_Integral = DHT11_ReadData(); /* read Relative Humidity's integral value */
    RH_Decimal = DHT11_ReadData();  /* read Relative Humidity's decimal value */
    T_Integral = DHT11_ReadData();    /* read Temperature's integral value */
    T_Decimal = DHT11_ReadData();   /* read Relative Temperature's decimal value */
    Checksum = DHT11_ReadData();    /* read 8-bit checksum value */
            
    while ( agr == 0){  
        onLEDS(T_Integral);//Encender los leds de advertencia
        valor2=T_Decimal;
        if(T_Integral <30 && acum == 0 ){
            PORTAbits.RA0 = 1;
            T_Integral = T_Integral + 6;
            acum = 6;
        } else if (T_Integral <30 && acum == 6){
            PORTAbits.RA1 = 1;
            T_Integral = T_Integral + 6;
            acum = 12;
        }else if (T_Integral <30 && acum == 12){
            PORTAbits.RA2 = 1;
            T_Integral = T_Integral + 6;
            acum = 18;
        }else if (T_Integral <30 && acum == 18){
            PORTAbits.RA3 = 1;
            T_Integral = T_Integral + 6;
            acum = 24;
        }else if (T_Integral <30 && acum == 24){
            PORTAbits.RA4 = 1;
            T_Integral = T_Integral + 6;            
        } else 
            agr=1;
        /* convert humidity value to ascii and send it to display*/
        sprintf(value,"%d",RH_Integral);
        LCD_String_xy(0,0,value);
        sprintf(value,".%d ",RH_Decimal);
        LCD_String(value);
        LCD_Char('%');
        
        /* convert temperature value to ascii and send it to display*/
        sprintf(value,"%d",T_Integral);
        LCD_String_xy(1,0,value);
        sprintf(value,".%d",T_Decimal);
        LCD_String(value);
        LCD_Char(0xdf);
        LCD_Char('C');

        sprintf(value,"%d  ",Checksum);
        LCD_String_xy(0,8,"Humedad");
        LCD_String_xy(1,8,"Temp");    
              
        MSdelay(500);
    }
    MSdelay(500);         
    }    
     
     
   
}        
        
    
    
    
  
    


char DHT11_ReadData()
{
  char i,data = 0;  
    for(i=0;i<8;i++)
    {
        while(!(Data_In & 1));      /* wait till 0 pulse, this is start of data pulse */
        __delay_us(30);         
        if(Data_In & 1)             /* check whether data is 1 or 0 */    
          data = ((data<<1) | 1); 
        else
          data = (data<<1);  
        while(Data_In & 1);
        
    }
  return data;
}

void DHT11_Start()
{    
    Data_Dir = 0;       /* set as output port */
    Data_Out = 0;       /* send low pulse of min. 18 ms width */
    __delay_ms(18);
    Data_Out = 1;       /* pull data bus high */
    __delay_us(20);
    Data_Dir = 1;       /* set as input port */ 
    
}

void DHT11_CheckResponse()
{
    while(Data_In & 1);     /* wait till bus is High */     
    while(!(Data_In & 1));  /* wait till bus is Low */
    while(Data_In & 1);     /* wait till bus is High */
}

void onLEDS( char T_Integral)
{
    int valor =T_Integral ;//- '0';
    if(valor < 30) //&& (T_Integral>20)
    {
        LED_A = 1;
        LED_V = 0;
        LED_R = 0;
    }
    else if( valor >=30 && valor<=35) { //Temperatura ideal entre los 20 y 29
        LED_A = 0;
        LED_V = 1;
        LED_R = 0;
    }
    else {
        LED_A = 0;
        LED_V = 0;
        LED_R = 1;
    }
    
}

