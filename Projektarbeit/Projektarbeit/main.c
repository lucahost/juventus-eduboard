#include <avr/io.h>
#include <stdint.h>

volatile uint32_t Time;
volatile uint8_t anzahl = 0;
volatile uint16_t ADC_result;

volatile uint8_t SW1_actual;

volatile uint8_t SW2_actual;

volatile uint8_t SW3_actual;

volatile uint16_t Realer_Wert;
volatile char Display_Wert[6];


char SETLED (writeledbyte)
{
	writeledbyte = ~writeledbyte;									// Variable wird negiert
	PORTF.OUT = (PORTF.OUT & 0xF0);									// PORTF wird mit UND Maskiert
	PORTF.OUT = ((PORTF.OUT & 0x0F) | (writeledbyte & 0x0F));		// PORTF wird mit UND Maskiert, nur F wird beschrieben. Variable writeledbyte ebenfalls mit UND Maskiert, low nibble wird ins PORTF übertragen
	PORTE.OUT = (PORTE.OUT & 0xF0);									// PORTE wird mit UND Maskiert
	PORTE.OUT = ((PORTE.OUT & 0x0F) | ((writeledbyte & 0xF0) >> 4));// PORTF wird mit UND Maskiert, nur F wird beschrieben. Variable writeledbyte ebenfalls mit UND Maskiert high nibble und dann wird die high nibble um 4 mal nach rechts geshiftet und dann ins PORTF übertragen
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
	PORTF.DIR = 0x0F;												// PORTF Port 0 bis 3 auf Output, 4 bis 7 auf Eingang
	PORTE.DIR = 0x0F;												// PORTE Port 0 bis 3 auf Output, 4 bis 7 auf Eingang

	volatile char writeledbyte = 0x00;								// variable nullsetzen
	SETLED (writeledbyte);											// LEDs setzen

	/******* Ende Ports Initialisierung ********/
	
	while(1)
	{	
		SW1_actual = ((~PORTF.IN >> 4) & 0b00000001);				// SW1 Status auslesen
		SW2_actual = ((~PORTF.IN >> 5) & 0b00000001);				// SW2 Status auslesen
		SW3_actual = ((~PORTF.IN >> 6) & 0b00000001);				// SW3 Status auslesen
		
		if (SW1_actual)												// Taste 1 gedrueckt?
		{
			ADCA.CH0.CTRL |= ADC_CH_START_bm;						// A/D Wandler starten
			while (!ADCA.CH0.INTFLAGS);								// Warte bis Wandlung abgeschlossen
			ADC_result= ADCA.CH0.RES;								// Wandlung Resultat ins ADC_result eintragen
			writeledbyte = ADC_result;								// ADC_result ins writeledbyte eintragen

			SETLED (writeledbyte);									// A/D Wandlung Resultat wird dann optisch auf LED übertragen und es zeigt nun Binäre Wert an.
			
			while (SW1_actual)										// bleibt sollange in die Schleife, bis die Taste 1 nicht mehr gedrueckt ist.
			{
				SW1_actual = ((~PORTF.IN >> 4) & 0b00000001);		// SW1 Status auslesen
			}
		}				
			 
	
		if (SW2_actual)
		{		
			Realer_Wert = ADC_result * 39;							// wie man auf *39 kommt, AD Resultat ist immer max 255. Also 1V / 255 = 0.003921
			Display_Wert[0] = (Realer_Wert / 1000) | 0x30;
			Display_Wert[1] = ((Realer_Wert % 1000) / 100) | 0x30;
			Display_Wert[2] = ((Realer_Wert % 1000) % 100 / 10) | 0x30;
//			Display_Wert[3] = 0x6D;									// m
//			Display_Wert[4] = 0x56;									// V
			Display_Wert[3] = 0x0A;									// Line Feed
			Display_Wert[4] = 0x0D;									// Carriage Return			
		
			for (uint8_t i = 0; i <= 3; i++)
			{
				USARTC0_DATA = Display_Wert[i];						// sendet das Zeichen RX_Char
				while (!( USARTC0_STATUS & USART_TXCIF_bm));		// wartet bis Zeichen gesendet	
			}
			
			while (SW2_actual)										// bleibt sollange in die Schleife, bis die Taste 2 nicht mehr gedrueckt ist.
			{
				SW2_actual = ((~PORTF.IN >> 5) & 0b00000001);		// SW2 Status auslesen
			}
		}

		if (SW3_actual)
		{
			Realer_Wert = ADC_result * 39;							// wie man auf *39 kommt, AD Resultat ist immer max 255. Also 1V / 255 = 0.003921
			Display_Wert[0] = (Realer_Wert / 1000) | 0x30;
			Display_Wert[1] = ((Realer_Wert % 1000) / 100) | 0x30;
			Display_Wert[2] = ((Realer_Wert % 1000) % 100 / 10) | 0x30;
//			Display_Wert[3] = 0x6D;									// m
//			Display_Wert[4] = 0x56;									// V
			Display_Wert[3] = 0x0A;									// Line Feed
			Display_Wert[4] = 0x0D;									// Carriage Return			
		
			for (uint8_t i = 0; i <= 3; i++)
			{
				USARTC0_DATA = Display_Wert[i];						// sendet das Zeichen RX_Char
				while (!( USARTC0_STATUS & USART_TXCIF_bm));		// wartet bis Zeichen gesendet	
			}			
			
			while (SW3_actual)										// bleibt sollange in die Schleife, bis die Taste 3 nicht mehr gedrueckt ist.
			{
				SW3_actual = ((~PORTF.IN >> 6) & 0b00000001);		// SW3 Status auslesen
			}
			
		}		
		
//		Time=0;
//		for(Time=0; Time <= 3000; Time++);							//Delay Time
	}
}