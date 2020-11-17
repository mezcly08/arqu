/* 
 * File:   pinout.h
 * Author: ASUS
 *
 * Created on 17 de noviembre de 2020, 08:24 AM
 */

#ifndef PINOUT_H
#define	PINOUT_H

#ifdef	__cplusplus
extern "C" {
#endif

#define efecto_pin TRISBbits.TRISB7
#define efecto_value PORTBbits.RB7

#define velocidad_pin TRISBbits.TRISB6
#define velocidad_value PORTBbits.RB6
    

#ifdef	__cplusplus
}
#endif

#endif	/* PINOUT_H */

