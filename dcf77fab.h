#include <stdint.h>

void initdcf77(void);
uint8_t check_dcf(void); // return 0 if ok
void dcf_setbit(void);



typedef struct	dcf_format_tag {
	volatile	uint8_t		data[8];	//!< Datenbits -> in Bl�cken in einem Byte gespeichert. 
	volatile	uint8_t		bitcnt;		//!< Bitz�hler f�r den Empfang
				uint8_t		min;		//!< Minutenz�hler dekodiert
				uint8_t		hour;		//!< Stundenz�hler
				uint8_t		wkday;		//!< Wochentag
				uint8_t		day;		//!< Tag
				uint8_t		month;		//!< Monat
				uint8_t		year;		//!< Jahr
				uint8_t		ok; // ready to be processed
}dcf_format_t;
