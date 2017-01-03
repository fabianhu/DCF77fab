
#include <avr/interrupt.h>
#include "dcf77fab.h"

#define CFG_ISR0_rise MCUCR |=  (1<< ISC00)
#define CFG_ISR0_fall MCUCR &= ~(1<< ISC00) // only after configured

extern uint32_t un100thSecTimer;
extern uint8_t gTimeOK;

volatile dcf_format_t	dcf;	//!< Controllstruktur für DCF

void dcf_resync(void)
{
	//crap all we know
	dcf.ok =0;
	for(uint8_t i=0; i < 8; i++)
	{
		dcf.data[i] = 0;
	}
	dcf.bitcnt=0;
	PORTB |= (1<<PB0); // LED on
}

ISR(INT0_vect)
{
	static uint32_t LastRiseTime;
	uint8_t RiseDiff; 

	RiseDiff = un100thSecTimer-LastRiseTime; // is ISR safe, as we are in ISR.

	if(MCUCR & (1<< ISC00))
	{
		// must be rising edge isr
		//if(RiseDiff < 90) break; // throw out too short pulses
		if(RiseDiff >= 95 && RiseDiff < 105) // about 1 s ago...
		{
			/*if(dcf.bitcnt >= 35)
			{
				// debug got everything except date.
				// copy result
				if (check_dcf() == 0)
				{
					bGetTime = 1;
				}
			}*/
			
			CFG_ISR0_fall; // config for falling
		}
		else if (RiseDiff > 195 && RiseDiff < 205) // about 2 s ago..
		{
			// sync
			dcf_resync();
			CFG_ISR0_fall; // config for falling
		}
		else
		{
			dcf_resync();  // not a valid interval of 1 or 2 seconds
		}
		
		LastRiseTime = un100thSecTimer; // is ISR safe, as we are in ISR.
		
	}
	else
	{ 
		// must be falling edge isr
		//if (RiseDiff < 5) break;
		if(RiseDiff >= 5 && RiseDiff < 13) // between 70 and 130 ms
		{
			// short pulse = 0

			if(dcf.bitcnt == 20)
			{
				dcf_resync(); //bit 20 is Start bit and must always be 1
			}
			
			if(dcf.bitcnt < 59)
			{
				// do not set bit
				dcf.bitcnt++;
				PORTB &= ~(1<<PB0); // LED off

			}
			else
			{
				dcf_resync(); // too many bits
			}

		}
		else if (RiseDiff > 14 && RiseDiff < 23) // between 150 and 230 ms
		{
			//long pulse = 1
			if(dcf.bitcnt == 0)
			{
				dcf_resync(); // Minute always starts with 0
			}
							

			if(dcf.bitcnt < 59)
			{
				dcf_setbit(); //dcf.bitcnt++; is included
				PORTB &= ~(1<<PB0); // LED off

			}
			else
			{
				dcf_resync(); // too many bits
			}

		}
		else
		{
			dcf_resync(); // pulse too short or too long
		}
		
		if(dcf.bitcnt == 59)
		{
			// copy result
			if (check_dcf() == 0)
			{
				// no error during decode
			}
		}

		CFG_ISR0_rise;
		
	}
	//GIFR |= 1<<INTF0; // clear int0
}


void initdcf77(void)
{

	//init INT0 for rising edge
	MCUCR |= (1<< ISC01) | (1<< ISC00); // cfg isr0 for rising edge

	// enable INT0
	GICR |= (1<<INT0);

	dcf.ok =0;

}

void dcf_setbit(void)
{
	if(dcf.bitcnt > 58)	// Hier geht was schief..
	{
	
	}
	else if(dcf.bitcnt == 58)	// Einsortieren der Daten		
		dcf.data[7] = 0x01;		// Parität Datum speichern
	else if (dcf.bitcnt > 49)	// Jahr
		dcf.data[6] |= ( 1 << (dcf.bitcnt - 50));
	else if (dcf.bitcnt > 44)	// Monat
		dcf.data[5] |= (1 << (dcf.bitcnt - 45));
	else if (dcf.bitcnt > 41)	// Wochentag
		dcf.data[4] |= (1 << (dcf.bitcnt - 42));
	else if (dcf.bitcnt > 35)	// Monatstag
		dcf.data[3] |= (1 << (dcf.bitcnt - 36));
	else if (dcf.bitcnt > 28)	// Stunden
		dcf.data[2] |= (1 << (dcf.bitcnt - 29));
	else if (dcf.bitcnt > 20)	// Monat
		dcf.data[1] |= (1 << (dcf.bitcnt - 21));
	else if (dcf.bitcnt > 14)	// Monat
		dcf.data[0] |= (1 << (dcf.bitcnt - 15));
	dcf.bitcnt++;
}


/*----------------------------------------------------------------------*/
//! \fn Berechnung der geraden Parität
//! @param	value	Byte über das die Parität berechnet werden soll
//! @return	Parität des Bytes
/*----------------------------------------------------------------------*/
uint8_t	check_parity(uint8_t value)
{
	uint8_t	even_parity = 0;

	while(value)
	{
		even_parity ^= (value & 0x01);
		value >>= 1; 
	};
	return even_parity;

}

/*----------------------------------------------------------------------*/
//! \fn Umwandlung der BCD-Zahl in DezimalZahl
//! @param	value	2-Stellige BCD-Zahl
//!	@return	Dezimalkodierte Zahl
/*----------------------------------------------------------------------*/
uint8_t bcd2dez(uint8_t value)
{
	uint8_t dez;

	dez = ((value & 0xF0)>> 4) * 10;
	dez += (value & 0x0F);

	return dez;
}

/*----------------------------------------------------------------------*/
//! \fn Überprüfung und Auswertung des DCF-Telegramms
/*----------------------------------------------------------------------*/
uint8_t check_dcf(void)
{
	uint8_t parity;

	parity = dcf.data[1] >> 7;
	
	if(check_parity(dcf.data[1]&0x7F) != parity)
	{
		return 1;	// Paritätsfehler!
	}

	dcf.min = bcd2dez(dcf.data[1] & 0x7F);	// Umwandlung Minuten
	
	parity = (dcf.data[2] & 0x40) >> 6;

	if(check_parity(dcf.data[2]&0x3F) != parity)
	{
		return 2;	// Paritätsfehler!
	}			

	dcf.hour = bcd2dez(dcf.data[2] & 0x3F);	// Umwandlung Minuten

	// Auswertung Parität Datum
	parity = check_parity(dcf.data[3]);		// Berechne Parity Kal-Day
	parity |= (check_parity(dcf.data[4])) << 1;	// Weekday
	parity |= (check_parity(dcf.data[5])) << 2;	// Month
	parity |= (check_parity(dcf.data[6])) << 3;	// Weekday

	parity = check_parity(parity);

	if(dcf.data[7] != parity)
	{
		return 3;	// Paritätsfehler!
	}

	dcf.day = bcd2dez(dcf.data[3]);
	dcf.wkday = bcd2dez(dcf.data[4]);
	dcf.month = bcd2dez(dcf.data[5]);
	dcf.year = bcd2dez(dcf.data[6]);


	/*printf("\r\nDCF-Time: %2d:%2d  %2d.%2d.%2d WK: %d\r\n",	// Debugausgabe
				dcf.hour,
				dcf.min,
				dcf.day,
				dcf.month,
				dcf.year,
				dcf.wkday);*/
	dcf.ok = 1;
	return 0;

}
