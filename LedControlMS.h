
#ifndef LedControl_h
#define LedControl_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

// Segments to be switched on for characters and digits on 7-Segment Displays
const static byte charTable[128] = 
{
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00011101,B00000000,B00000000,  //Замена '%' на нижний кружочек
    B00000000,B00000000,B01100011,B00000000,B10000000,B00000001,B10000000,B00000000,  //Замена '*' на верхний кружочек 
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B01110111,B00011111,B01001110,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000
};

//the opcodes for the MAX7221 and MAX7219
#define OP_NOOP   0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE  9
#define OP_INTENSITY   10
#define OP_SCANLIMIT   11
#define OP_SHUTDOWN    12
#define OP_DISPLAYTEST 15

template<const size_t m_maxDevices>
class CLedControl 
{
 public:
    /* 
     * Create a new controler 
     * Params :
     * dataPin		pin on the Arduino where data gets shifted out
     * clockPin		pin for the clock
     * csPin		pin for selecting the device 
     * numDevices	maximum number of devices that can be controled
     */
    CLedControl(byte dataPin, byte clkPin, byte csPin)
		:m_SPI_MOSI(dataPin)
		, m_SPI_CLK(clkPin)
		, m_SPI_CS(csPin)
	{
		pinMode(m_SPI_MOSI, OUTPUT);
		pinMode(m_SPI_CLK, OUTPUT);
		pinMode(m_SPI_CS, OUTPUT);
		digitalWrite(m_SPI_CS, HIGH);
		for (size_t i = 0; i < m_maxDevices; i++)
		{
			spiTransfer(i, OP_DISPLAYTEST, 0);
			setScanLimit(i, 7);//scanlimit is set to max on startup
			spiTransfer(i, OP_DECODEMODE, 0);//decode is done in source
			clearDisplay(i);
			shutdown(i, true);//we go into shutdown-mode on startup
		}
	}


    /*
     * Gets the number of devices attached to this LedControl.
     * Returns :
     * int	the number of devices on this LedControl
     */
    size_t getDeviceCount() {return m_maxDevices;};

    /* 
     * Set the shutdown (power saving) mode for the device
     * Params :
     * addr	The address of the display to control
     * status	If true the device goes into power-down mode. Set to false
     *		for normal operation.
     */
    void shutdown(size_t addr, bool status)
	{
		//if (addr >= m_maxDevices)
		//	return;
		//if (status)
        spiTransfer(addr, OP_SHUTDOWN, ~(byte(status)));
		//else spiTransfer(addr, OP_SHUTDOWN, 1);
	}


    /* 
     * Set the number of digits (or rows) to be displayed.
     * See datasheet for sideeffects of the scanlimit on the brightness
     * of the display.
     * Params :
     * addr	address of the display to control
     * limit	number of digits to be displayed (1..8)
     */
    void setScanLimit(size_t addr, byte limit)
	{
		spiTransfer(addr, OP_SCANLIMIT, limit);
	}

    /* 
     * Set the brightness of the display.
     * Params:
     * addr		the address of the display to control
     * intensity	the brightness of the display. (0..15)
     */
    void setIntensity(size_t addr, byte intensity){ spiTransfer(addr, OP_INTENSITY, intensity);}

    /* 
     * Switch all Leds on the display off. 
     * Params:
     * addr	address of the display to control
     */
    void clearDisplay(size_t addr)
	{
		for (int i = 0; i < 8; i++)
			spiTransfer(addr, i + 1, 0);
	}

	
    void clearAll();

    /* 
     * Set the status of a single Led.
     * Params :
     * addr	address of the display 
     * row	the row of the Led (0..7)
     * col	the column of the Led (0..7)
     * state	If true the led is switched on, 
     *		if false it is switched off
     */
    //void setLed(size_t addr, size_t row, size_t column, boolean state);

    /* 
     * Set all 8 Led's in a row to a new state
     * Params:
     * addr	address of the display
     * row	row which is to be set (0..7)
     * value	each bit set to 1 will light up the
     *		corresponding Led.
     */
    //void setRow(size_t addr, size_t row, byte value);

    /* 
     * Set all 8 Led's in a column to a new state
     * Params:
     * addr	address of the display
     * col	column which is to be set (0..7)
     * value	each bit set to 1 will light up the
     *		corresponding Led.
     */
    //void setColumn(size_t addr, size_t col, byte value);

    /* 
     * Display a hexadecimal digit on a 7-Segment Display
     * Params:
     * addr	address of the display
     * digit	the position of the digit on the display (0..7)
     * value	the value to be displayed. (0x00..0x0F)
     * dp	sets the decimal point.
     */
    //void setDigit(size_t addr, size_t digit, byte value, boolean dp);

    /* 
     * Display a character on a 7-Segment display.
     * There are only a few characters that make sense here :
     *	'0','1','2','3','4','5','6','7','8','9','0',
     *  'A','b','c','d','E','F','H','L','P',
     *  '.','-','_',' ' 
     * Params:
     * addr	address of the display
     * digit	the position of the character on the display (0..7)
     * value	the character to be displayed. 
     * dp	sets the decimal point.
     */
    void setChar(size_t addr, size_t digit, char value, boolean dp)
	{
		byte v = charTable[value & B01111111];
		if (dp)
			v |= B10000000;
		spiTransfer(addr, digit + 1, v);
	}

	size_t getCharArrayPosition(char c);//Returns the array number in the alphabetBitmap array 
	//void writeString(int mtx, char * displayString);
    //void displayChar(int matrix, int charIndex);

 private:
	 //byte	m_status[64];/* We keep track of the led-status for all 8 devices in this array */
	 byte	m_SPI_MOSI;/* Data is shifted out of this pin*/
	 byte	m_SPI_CLK;/* The clock is signaled on this pin */
	 byte	m_SPI_CS;/* This one is driven LOW for chip selectzion */
	 //const size_t m_maxDevices;/* The maximum number of devices we use */

	 void spiTransfer(size_t addr, byte opcode, byte data)/* Send out a single command to the device */
	 {
		 const size_t offset = addr * 2;
		 constexpr size_t maxbytes = m_maxDevices * 2;
		 byte spidata[maxbytes] = { 0 };//Create an array with the data to shift out /* The array for shifting the data to the devices */

		 spidata[offset + 1] = opcode;	//put our device data into the array
		 spidata[offset] = data;		//put our device data into the array

         noInterrupts();    //disabling interrupts for the output time
		 digitalWrite(m_SPI_CS, LOW);//enable the line 
		 for (size_t i = maxbytes; i; --i)
			 shiftOut(m_SPI_MOSI, m_SPI_CLK, MSBFIRST, spidata[i - 1]);//Now shift out the data 
		 digitalWrite(m_SPI_CS, HIGH);//latch the data onto the display
         interrupts(); //enabling interrupts
	 }
};

#endif	//LedControl.h



