/*
 Controlador para los dispositivos:
 *      Sensor de Temperatura y Humedad
 *      Reloj tiempo real
 *      Leds de Advertencia
 *		Bombillas
 *		LCD 16x2
 *		I2C
 *		Virtual terminal
 Se utiliza el Microcontrolador PIC 18F4550
 */

/*!
\file   Controlador.c
\date   2020-12-14
\author Carlos Hoyos <joiroce@unicauca.edu.co> 104617020768
        Eliana Camayo <eacamayo@unicauca.edu.co> 104616021725
\brief  Example Controler .

\par Copyright
Information contained herein is proprietary to and constitutes valuable
confidential trade secrets of unicauca, and
is subject to restrictions on use and disclosure.

\par
Copyright (c) unicauca 2020. All rights reserved.

\par
The copyright notices above do not evidence any actual or
intended publication of this material.
 ******************************************************************************
 */


#include <pic18f4550.h>
#include <xc.h>
#include <stdio.h>
#include "Configuration_Header_File.h"
#include "LCD_16x2_8-bit_Header_File.h"
#include "I2C_Master_File.h"

#define _XTAL_FREQ  8000000L
#define Data_Out LATC2              /* assign Port pin for data*/
#define Data_In PORTCbits.RC2       /* read data from Port pin*/
#define Data_Dir TRISCbits.RC2      /* Port direction */

#define LED_V PORTCbits.RC4         // Definimos el puerto del led verde
#define LED_A PORTCbits.RC5         // Definimos el puerto del led Amarillo
#define LED_R PORTAbits.RA5         // Definimos el puerto del led Rojo

#define device_id_write 0xD0
#define device_id_read 0xD1

//Procedimietos del sensor de temperatura
void DHT11_Start();
void DHT11_CheckResponse();
char DHT11_ReadData();

//Procedimietos 
void onLEDS(char);
void offBombillas();
void imprimirTemHum();
void encenderBombillas();
void offLEDS();
void imprimirBomLed();

// Procedimientos RTC
void mostrarFechaHora();
void verificarHora();

//Caracter especial LED
unsigned char bombillaCar0[8] = {  0x0E,  0x11,  0x11,  0x11,  0x0E,  0x0E,  0x00,  0x00};

//Variables utilizadas por el sistema.
char var_RH_Decimal, var_RH_Integral, var_T_Decimal, var_T_Integral, valor2;
char vec_value[10];
int banAgregar = 0; //Bandera
int acumBom = 0; //Acumulador para encender bombillas

//Variables del RTC
int sec, min, hour;
int Day, Date, Month, Year;
char secs[10], mins[10], hours[10];
char date[10], month[10], year[10];
char Clock_type = 0x06;
char AM_PM = 0x05;
char days[7] = {'S', 'M', 'T', 'W', 't', 'F', 's'};
//Fin variables RTC

char j;

        

//Main

void main() {
    TRISA = 0;

    //Inicializar las bombillas como apagadas
    PORTAbits.RA0 = 0;
    PORTAbits.RA1 = 0;
    PORTAbits.RA2 = 0;
    PORTAbits.RA3 = 0;
    PORTAbits.RA4 = 0;

    //Configura UART a 9600 baudios
    TRISCbits.RC6 = 0; //  Pin RC6 como salida digital para TX.
    TXSTAbits.TX9 = 0; //  Modo-8bits.
    TXSTAbits.TXEN = 1; //  Habilita Transmisión.
    TXSTAbits.SYNC = 0; //  Modo-Asíncrono.
    TXSTAbits.BRGH = 0; //  Modo-Baja Velocidad de Baudios.
    BAUDCONbits.BRG16 = 0; //  Baudios modo-8bits.
    RCSTAbits.SPEN = 1; //  Hbilita el Módulo SSP como UART.
    SPBRG = (unsigned char) (((_XTAL_FREQ / 9600) / 64) - 1); //baudios  = 9600

    OSCCON = 0x72; /* set internal oscillator with frequency 8 MHz*/
    //3TRISAbits.TRI3SA2 = 0;


    ADCON1 = 0x0F; /* this makes all pins as a digital I/O pins */


    while (1) {
        LCD_Init(); /* initialize LCD16x2 */
        I2C_Init(); /*initialize I2C protocol*/
        MSdelay(10);
        
              
        verificarHora();

        if ( hour >= 0x18 || hour <=6 ) { //Compara si la hora es nocturna
            DHT11_Start(); /* send start pulse to DHT11 module */
            DHT11_CheckResponse(); /* wait for response from DHT11 module */

            /* read 40-bit data from DHT11 module */
            var_RH_Integral = DHT11_ReadData(); /* read Relative Humidity's integral value */
            var_RH_Decimal = DHT11_ReadData(); /* read Relative Humidity's decimal value */
            var_T_Integral = DHT11_ReadData(); /* read Temperature's integral value */
            var_T_Decimal = DHT11_ReadData(); /* read Relative Temperature's decimal value */

            imprimirTemHum();

            char buffer_TX[] = "No se ha prendido ninguna bombilla\r";
            for (int i = 0; i < 35; i++) {
                //  espera a que el registro de transmisión este disponible o vacio.
                while (!TXSTAbits.TRMT) {
                }
                // escribe el dato que se enviará a través de TX.
                TXREG = buffer_TX[i];
            }
            MSdelay(1000); // 1 seg de espera para aparecer la tem leida

            while (banAgregar == 0) {

                encenderBombillas();
                imprimirTemHum();

                MSdelay(1000);
            }
            mostrarFechaHora();
            MSdelay(1000);
            banAgregar = 0;
            acumBom = 0;
            offBombillas();
            offLEDS();
            MSdelay(2000);

        } else {
            LCD_String_xy(0, 0, "ERROR HORA ");
            LCD_String_xy(1, 0, "DIURNA");
            char buffer_TX[] = "La hora nocturna empieza a las 18:00\r";
            for (int i = 0; i < 37; i++) {
                //  espera a que el registro de transmisión este disponible o vacio.
                while (!TXSTAbits.TRMT) {
                }
                // escribe el dato que se enviará a través de TX.
                TXREG = buffer_TX[i];
            }
        }
        MSdelay(2000);

    }
}

// Metodos del sensor DHT11 (SENSOR DE TEMPERATURA Y HUMEDAD)

char DHT11_ReadData() {
    char i, data = 0;
    for (i = 0; i < 8; i++) {
        while (!(Data_In & 1)); /* wait till 0 pulse, this is start of data pulse */
        __delay_us(30);
        if (Data_In & 1) /* check whether data is 1 or 0 */
            data = ((data << 1) | 1);
        else
            data = (data << 1);
        while (Data_In & 1);

    }
    return data;
}

void DHT11_Start() {
    Data_Dir = 0; /* set as output port */
    Data_Out = 0; /* send low pulse of min. 18 ms width */
    __delay_ms(18);
    Data_Out = 1; /* pull data bus high */
    __delay_us(20);
    Data_Dir = 1; /* set as input port */

}

void DHT11_CheckResponse() {
    while (Data_In & 1); /* wait till bus is High */
    while (!(Data_In & 1)); /* wait till bus is Low */
    while (Data_In & 1); /* wait till bus is High */
}

// Método para apagar LEDS

void offLEDS() {
    LED_A = 0;
    LED_V = 0;
    LED_R = 0;

    char buffer_TX[] = "Apagando LEDS\r";
    for (int i = 0; i < 15; i++) {
        //  espera a que el registro de transmisión este disponible o vacio.
        while (!TXSTAbits.TRMT) {
        }
        //  escribe el dato que se enviará a través de TX.
        TXREG = buffer_TX[i];
    }
}
// Método para encender LEDS

void onLEDS(char T_Integral) {
    int valor = T_Integral;
    if (valor < 30) {
        LED_A = 1;
        LED_V = 0;
        LED_R = 0;
    } else if (valor >= 30 && valor <= 35) { //Temperatura ideal. Entre los 30 y 35 grados
        LED_A = 0;
        LED_V = 1;
        LED_R = 0;
        char buffer_TX[] = "Temperatura Ideal\r";
        for (int i = 0; i < 19; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }
    } else {
        LED_A = 0;
        LED_V = 0;
        LED_R = 1;

    }

}

void offBombillas() {
    PORTAbits.RA0 = 0;
    PORTAbits.RA1 = 0;
    PORTAbits.RA2 = 0;
    PORTAbits.RA3 = 0;
    PORTAbits.RA4 = 0;
    char buffer_TX[] = "Bombillas Apagadas \r";
    for (int i = 0; i < 20; i++) {
        //  espera a que el registro de transmisión este disponible o vacio.
        while (!TXSTAbits.TRMT) {
        }
        //  escribe el dato que se enviará a través de TX.
        TXREG = buffer_TX[i];
    }
}

// Imrimir en el LCD los datos de Temperatura y Humedad
void imprimirTemHum() {
    /* convert humidity value to ascii and send it to display*/
    sprintf(vec_value, "%d", var_RH_Integral);
    LCD_String_xy(0, 9, vec_value);
    sprintf(vec_value, ".%d ", var_RH_Decimal);
    LCD_String(vec_value);
    LCD_Char('%');

    /* convert temperature value to ascii and send it to display*/
    sprintf(vec_value, "%d", var_T_Integral);
    LCD_String_xy(0, 0, vec_value);
    sprintf(vec_value, ".%d", var_T_Decimal);
    LCD_String(vec_value);
    LCD_Char(0xdf);
    LCD_Char('C');

    LCD_String_xy(0, 15, "H");
    LCD_String_xy(0, 6, "T");
}


void encenderBombillas() {
    LCD_Custom_Char(0,bombillaCar0);
    LCD_Custom_Char(1,bombillaCar0);
    LCD_Custom_Char(2,bombillaCar0);
    LCD_Custom_Char(3,bombillaCar0);
    LCD_Custom_Char(4,bombillaCar0);
    
    onLEDS(var_T_Integral); //Encender los leds de advertencia
    if (var_T_Integral < 30 && acumBom == 0) {
        PORTAbits.RA0 = 1;
        char buffer_TX[] = "Se encendio la bombilla 1\r";
        for (int i = 0; i < 27; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }      
        
                
        //Parte para Imprimir el Caracter
                           
        LCD_Command(0xc0|(0));     /*Display characters from c0(2nd row) location */
        LCD_Char(0);                 /*To display custom character send address as data to point stored 
                                       character */  
        
        
//        LCD_String_xy(1, 0, "ON B.");
//        LCD_String_xy(1, 6, "1");
            
        var_T_Integral = var_T_Integral + 6;
       acumBom = 6;

    } else if (var_T_Integral < 30 && acumBom == 6) {
        PORTAbits.RA1 = 1;        
        
        char buffer_TX[] = "Se encendio la bombilla 2\r";
        for (int i = 0; i < 27; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }
        //Parte para Imprimir el Caracter
        LCD_Command(0xc0|(3));     /*Display characters from c0(2nd row) location */
        LCD_Char(1);                 /*To display custom character send address as data to point stored character */     
        
        var_T_Integral = var_T_Integral + 6;
        acumBom = 12;
    } else if (var_T_Integral < 30 && acumBom == 12) {
        PORTAbits.RA2 = 1;        
        
        char buffer_TX[] = "Se encendio la bombilla 3\r";
        for (int i = 0; i < 27; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }

        //Parte para Imprimir el Caracter
        LCD_Command(0xc0|(6));     /*Display characters from c0(2nd row) location */
        LCD_Char(2);                 /*To display custom character send address as data to point stored character */     
        
        var_T_Integral = var_T_Integral + 6;
        acumBom = 18;
    } else if (var_T_Integral < 30 && acumBom == 18) {
        PORTAbits.RA3 = 1;
        
        char buffer_TX[] = "Se encendio la bombilla 4\r";
        for (int i = 0; i < 27; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }
        
        //Parte para Imprimir el Caracter
        LCD_Command(0xc0|(9));     /*Display characters from c0(2nd row) location */
        LCD_Char(3);                 /*To display custom character send address as data to point stored character */     
        
        var_T_Integral = var_T_Integral + 6;
        acumBom = 24;
    } else if (var_T_Integral < 30 && acumBom == 24) {
        PORTAbits.RA4 = 1;

        LCD_Custom_Char(4,bombillaCar0);
        for(char i=4;i<5;i++)
            {    
                LCD_Command(0xc0|(i*3));     /*Display characters from c0(2nd row) location */
                LCD_Char(i);                 /*To display custom character send address as data to point stored 
                                               character */
            } 
        char buffer_TX[] = "Se encendio la bombilla 5\r";
        for (int i = 0; i < 27; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }
        
        //Parte para Imprimir el Caracter
        LCD_Command(0xc0|(12));     /*Display characters from c0(2nd row) location */
        LCD_Char(4);                 /*To display custom character send address as data to point stored character */     
        
        var_T_Integral = var_T_Integral + 6;
    } else if (var_T_Integral > 35) {
        banAgregar = 1;
        char buffer_TX[] = "Temperatura Alta \r";
        for (int i = 0; i < 19; i++) {
            //  espera a que el registro de transmisión este disponible o vacio.
            while (!TXSTAbits.TRMT) {
            }
            //  escribe el dato que se enviará a través de TX.
            TXREG = buffer_TX[i];
        }
        MSdelay(100);
    } else
        banAgregar = 1;
    MSdelay(1000);

}

//Procedimeintos utilizados en RTC

void RTC_Read_Clock(char read_clock_address) {
    I2C_Start(device_id_write);
    I2C_Write(read_clock_address); /* address from where time needs to be read*/
    I2C_Repeated_Start(device_id_read);
    sec = I2C_Read(0); /*read data and send ack for continuous reading*/
    min = I2C_Read(0); /*read data and send ack for continuous reading*/
    hour = I2C_Read(1); /*read data and send nack for indicating stop reading*/

}

void RTC_Read_Calendar(char read_calendar_address) {
    I2C_Start(device_id_write);
    I2C_Write(read_calendar_address); /* address from where time needs to be read*/
    I2C_Repeated_Start(device_id_read);
    Day = I2C_Read(0); /*read data and send ack for continuous reading*/
    Date = I2C_Read(0); /*read data and send ack for continuous reading*/
    Month = I2C_Read(0); /*read data and send ack for continuous reading*/
    Year = I2C_Read(1); /*read data and send nack for indicating stop reading*/
    I2C_Stop();
}

void mostrarFechaHora() {
    LCD_Init();
    RTC_Read_Clock(0); /*gives second,minute and hour*/
    I2C_Stop();
    MSdelay(1000);
    if (hour & (1 << Clock_type)) { /* check clock is 12hr or 24hr */

        if (hour & (1 << AM_PM)) { /* check AM or PM */
            LCD_String_xy(1, 14, "PM");
        } else {
            LCD_String_xy(1, 14, "AM");
        }

        hour = hour & (0x1f);
        sprintf(secs, "%x ", sec); /*%x for reading BCD format from RTC DS1307*/
        sprintf(mins, "%x:", min);
        sprintf(hours, "Tim:%x:", hour);
        LCD_String_xy(0, 0, hours);
        LCD_String(mins);
        LCD_String(secs);
    } else {

        hour = hour & (0x3f);
        sprintf(secs, "%x ", sec); /*%x for reading BCD format from RTC DS1307*/
        sprintf(mins, "%x:", min);
        sprintf(hours, "Tim:%x:", hour);
        LCD_String_xy(0, 0, hours);
        LCD_String(mins);
        LCD_String(secs);
    }

    RTC_Read_Calendar(3); /*gives day, date, month, year*/
    I2C_Stop();
    sprintf(date, "Cal %x-", Date);
    sprintf(month, "%x-", Month);
    sprintf(year, "%x ", Year);
    LCD_String_xy(2, 0, date);
    LCD_String(month);
    LCD_String(year);

    /* find day */
    switch (days[Day]) {
        case 'S':
            LCD_String("Sun");
            break;
        case 'M':
            LCD_String("Mon");
            break;
        case 'T':
            LCD_String("Tue");
            break;
        case 'W':
            LCD_String("Wed");
            break;
        case 't':
            LCD_String("Thu");
            break;
        case 'F':
            LCD_String("Fri");
            break;
        case 's':
            LCD_String("Sat");
            break;
        default:
            break;

    }
    char buffer_TX[] = "Registrando Hora y Fecha\r";
    for (int i = 0; i < 25; i++) {
        //  espera a que el registro de transmisión este disponible o vacio.
        while (!TXSTAbits.TRMT) {
        }
        // escribe el dato que se enviará a través de TX.
        TXREG = buffer_TX[i];
    }

}

void verificarHora() {
    RTC_Read_Clock(0); /*gives second,minute and hour*/
    I2C_Stop();
    MSdelay(1000);
    if (hour & (1 << Clock_type)) { /* check clock is 12hr or 24hr */
        if (hour & (1 << AM_PM)) { /* check AM or PM */
            LCD_String_xy(1, 14, "PM");
        } else {
            LCD_String_xy(1, 14, "AM");
        }
    }

}
