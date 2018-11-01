////////////////////////////////////////////////////////////////////////////////////
//
//		Autor: TSTSI1709
//		Datum: 25.10.2018
//		Projekt: EduBoard mit AD und Serielle Übertragung
//
////////////////////////////////////////////////////////////////////////////////////

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

volatile uint32_t Time;
volatile uint8_t anzahl = 0;
volatile uint8_t ADC_result = 0x00;
volatile char writeledbyte = 0x00;								// variable nullsetzen

volatile uint8_t SW1_actual;
volatile uint8_t SW2_actual;
volatile uint8_t SW3_actual;

volatile uint16_t Realer_Wert;
char Display_Wert[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void SETLED (int value)
{
	value = ~value;									// Variable wird negiert

	PORTF.OUT |= 0x0F;
	PORTF.OUT &= (value | 0xF0);
		
	PORTE.OUT |= 0x0F;
	PORTE.OUT &= ((value | 0x0F) >> 4);	
}

int main(void)
{
	/******* Start A/D Wandler Initialisierung ********/

	PORTA.DIR = 0x00;												// Setze PortB0 als Output (Pot1)
	ADCA.CTRLB = ADC_RESOLUTION_8BIT_gc;							// Auloesung = 8bit unsigned
	ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_BANDGAP_bm ;			// Referenzspannung = Bandgap 1.0V
	ADCA.PRESCALER = ADC_PRESCALER_DIV4_gc;							// ADC Clockteiler = 4
	ADCA.CTRLA = ADC_ENABLE_bm;										// Enable Wandler (freigeben)

	//Eingangspin und Wandlungsart fuer Kanal0 festlegen
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;				//Single ended Modus
	ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN8_gc;						//ADCPin 8 = Mess-Eingang

	/******* Ende A/D Wandler Initialisierung ********/

	/******* Start UART Initialisierung ********/
	// Initialisiere UART (9600,8,o,1)
	// bestimmen der Baudrate
	USARTC0_BAUDCTRLA = 0x0c;
	USARTC0_BAUDCTRLB = 0x00;
	
	// Senden und Empfangen freigeben
	USARTC0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;
	
	// Protokoll bestimmen
	USARTC0_CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_ODD_gc | USART_CHSIZE_8BIT_gc;
	PORTC_DIR = PIN3_bm;
	/******* Ende UART Initialisierung ********/

	/******* Start Ports Initialisierung ********/
	PORTE.DIR = 0x0F;												// PORTE Port 0 bis 3 auf Output, 4 bis 7 auf Eingang
	PORTF.DIR = 0x0F;												// PORTF Port 0 bis 3 auf Output, 4 bis 7 auf Eingang
	/******* Ende Ports Initialisierung ********/
	
	SETLED (ADC_result);											// LEDs setzen

	while(1)
	{	
		SW1_actual = ((~PORTF.IN >> 4) & 0x01);				// SW1 Status auslesen
		SW2_actual = ((~PORTF.IN >> 5) & 0x01);				// SW2 Status auslesen
		SW3_actual = ((~PORTF.IN >> 6) & 0x01);				// SW3 Status auslesen
		
		if (SW1_actual)												// Taste 1 gedrueckt?
		{
			ADCA.CH0.CTRL |= ADC_CH_START_bm;						// A/D Wandler starten
			while (!ADCA.CH0.INTFLAGS);								// Warte bis Wandlung abgeschlossen
			ADC_result= ADCA.CH0.RES;								// Wandlung Resultat ins ADC_result eintragen

			SETLED (ADC_result);											// A/D Wandlung Resultat wird dann optisch auf LED �bertragen und es zeigt nun Bin�re Wert an.
			
			while (SW1_actual)										// bleibt sollange in die Schleife, bis die Taste 1 nicht mehr gedrueckt ist.
			{
				SW1_actual = ((~PORTF.IN >> 4) & 0x01);		// SW1 Status auslesen
			}
		}				
			 
	
		if (SW2_actual)
		{	
			Realer_Wert = ADC_result * 39;							// wie man auf *39 kommt, AD Resultat ist immer max 255. Also 1V / 255 = 0.003921
			Display_Wert[0] = (Realer_Wert / 1000) | 0x30;
			Display_Wert[1] = ((Realer_Wert % 1000) / 100) | 0x30;
			Display_Wert[2] = ((Realer_Wert % 1000) % 100 / 10) | 0x30;
			Display_Wert[3] = 0x6D;									// m
			Display_Wert[4] = 0x56;									// V
			Display_Wert[5] = 0x0D;									// Carriage Return			
			Display_Wert[6] = 0x0A;									// Line Feed
		
			for (int j = 0; j < sizeof(Display_Wert); j++)
			{
				while (!(USARTC0_STATUS & USART_DREIF_bm));			// wartet bis Zeichen gesendet
				USARTC0_DATA = Display_Wert[j];						// sendet das Zeichen TX_Char	
			}
			
			while (SW2_actual)										// bleibt sollange in die Schleife, bis die Taste 2 nicht mehr gedrueckt ist.
			{
				SW2_actual = ((~PORTF.IN >> 5) & 0x01);		// SW2 Status auslesen
			}
		}

		if (SW3_actual)
		{	// mit 2 Stellen nach Komma
			Realer_Wert = ADC_result * 39;							// Hier z.b. Resultat 9945,  wie man auf *39 kommt, AD Resultat ist immer max 255. Also 1V / 255 = 0.003921
			Display_Wert[0] = (Realer_Wert / 10000) | 0x30;				// Vom Beispiel 10000 nach 1 V
			Display_Wert[1] = 0x2C;										// , (Komma)
			Display_Wert[2] = ((Realer_Wert % 10000) / 1000) | 0x30;		// vom beispiel 9945 Modulo 10000 gibt 9945 rest, dann geteilt durch 1000 gibts 9
			Display_Wert[3] = ((Realer_Wert % 1000) / 100) | 0x30;		// vom beispiel 9945 Modulo 1000 gibt 945 rest, dann geteilt durch 100 gibts 9
//			Display_Wert[2] = ((Realer_Wert % 100) / 10) | 0x30;			// vom beispiel 9945 Modulo 100 gibt 45 rest, dann geteilt durch 10 gibts 4
//			Display_Wert[2] = ((Realer_Wert % 10) | 0x30;				// vom beispiel 9945 Modulo 10 gibt 5 rest, gibts 9
			Display_Wert[4] = 0x56;										// V
			Display_Wert[5] = 0x0D;										// Carriage Return			
			Display_Wert[6] = 0x0A;										// Line Feed

			for (int j = 0; j < sizeof(Display_Wert); j++)
			{
				while (!(USARTC0_STATUS & USART_DREIF_bm));			// wartet bis Zeichen gesendet
				USARTC0_DATA = Display_Wert[j];						// sendet das Zeichen TX_Char	
			}

			while (SW3_actual)										// bleibt sollange in die Schleife, bis die Taste 3 nicht mehr gedrueckt ist.
			{
				SW3_actual = ((~PORTF.IN >> 6) & 0x01);				// SW3 Status auslesen
			}		
		}		
		
//		Time=0;
//		for(Time=0; Time <= 3000; Time++);							//Delay Time
	}
}