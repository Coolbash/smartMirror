
#include "LedControlMS.h"


//---------------------------------------------------------------
//template<size_t m_maxDevices>CLedControl<m_maxDevices>::CLedControl(byte dataPin, byte clkPin, byte csPin/*, size_t numDevices = 1*/)
//	:m_SPI_MOSI(dataPin)
//	,m_SPI_CLK(clkPin)
//	,m_SPI_CS(csPin)
//	//,m_status{0}
//	//,m_maxDevices(max(numDevices,8))
//{
//	//if (numDevices > 8)
//	//	numDevices = 8;
//	//m_maxDevices = numDevices;
//	pinMode(m_SPI_MOSI, OUTPUT);
//	pinMode(m_SPI_CLK, OUTPUT);
//	pinMode(m_SPI_CS, OUTPUT);
//	digitalWrite(m_SPI_CS, HIGH);
//	//m_SPI_MOSI = dataPin;
//	//for (int i = 0; i < 64; i++)
//	//	m_status[i] = 0x00;
//	for (int i = 0; i < m_maxDevices; i++)
//	{
//		spiTransfer(i, OP_DISPLAYTEST, 0);
//		setScanLimit(i, 7);//scanlimit is set to max on startup
//		spiTransfer(i, OP_DECODEMODE, 0);//decode is done in source
//		clearDisplay(i);
//		shutdown(i, true);//we go into shutdown-mode on startup
//	}
//}
//---------------------------------------------------------------

//template<size_t m_maxDevices> void CLedControl<m_maxDevices>::shutdown(size_t addr, bool b)
//{
//	if (addr >= m_maxDevices)
//		return;
//	if (b)
//		spiTransfer(addr, OP_SHUTDOWN, 0);
//	else
//		spiTransfer(addr, OP_SHUTDOWN, 1);
//}
//---------------------------------------------------------------

//template<size_t m_maxDevices> void CLedControl<m_maxDevices>::setScanLimit(size_t addr, byte limit)
//{
//	if ( addr >= m_maxDevices)
//		return;
//	if (limit < 8)
//		spiTransfer(addr, OP_SCANLIMIT, limit);
//}
//---------------------------------------------------------------

//template<size_t m_maxDevices> void CLedControl<m_maxDevices>::setIntensity(size_t addr, byte intensity)
//{
//    if(addr>=m_maxDevices)
//		return;
//    if(intensity<16)	
//		spiTransfer(addr, OP_INTENSITY, intensity);
//}
//---------------------------------------------------------------

//template<size_t m_maxDevices> void CLedControl<m_maxDevices>::clearDisplay(size_t addr)
//{
//	if (addr >= m_maxDevices)
//		return;
//	size_t offset = addr * 8;
//	for (int i = 0; i < 8; i++) 
//	{
//		//m_status[offset + i] = 0;
//		spiTransfer(addr, i + 1, 0);
//    }
//}
//---------------------------------------------------------------

template<size_t m_maxDevices> void CLedControl<m_maxDevices>::clearAll()
{
   for (size_t i=0;i<m_maxDevices;i++) clearDisplay(i);
}
//---------------------------------------------------------------

//void LedControl::setLed(size_t addr, size_t row, size_t column, boolean state) 
//{
//    if(addr>=m_maxDevices || row>7 || column>7)
//		return;
//    size_t offset=addr*8;
//    byte val=B10000000 >> column;
//    if(state)
//		m_status[offset + row] |=  val;
//    else 
//		m_status[offset+row] &= ~val;
//    spiTransfer(addr, row+1, m_status[offset+row]);
//}
//---------------------------------------------------------------

//void LedControl::setRow(size_t addr, size_t row, byte value) 
//{
//	int offset;
//	if ( addr >= m_maxDevices || row>7)
//		return;
//	offset = addr * 8;
//	m_status[offset + row] = value;
//	spiTransfer(addr, row + 1, value);
//}
    
//void LedControl::setColumn(size_t addr, size_t col, byte value)
//{
//	byte val;
//
//	if (addr >= m_maxDevices || col>7)
//		return;
//	for (int row = 0; row < 8; row++) 
//	{
//		val = value >> (7 - row);
//		val = val & 0x01;
//		setLed(addr, row, col, val);
//	}
//}

//void LedControl::setDigit(size_t addr, size_t digit, byte value, boolean dp) 
//{
//	int offset;
//	byte v;
//
//	if (addr >= maxDevices ||  digit>7 || value > 15)
//		return;
//	offset = addr * 8;
//	v = charTable[value];
//	if (dp)
//		v |= B10000000;
//	status[offset + digit] = v;
//	spiTransfer(addr, digit + 1, v);
//}

//---------------------------------------------------------------
//template<size_t m_maxDevices> void CLedControl<m_maxDevices>::setChar(size_t addr, size_t digit, char value, boolean dp)
//{
//	size_t offset;
//	byte v;
//
//	if ( addr >= m_maxDevices || digit>7)
//		return;
//	offset = addr * 8;
//	//index = (byte)value;
//	//if (index > 127) 
//	//	value = 32;//nothing define we use the space char
//	v = charTable[value & B01111111];
//	if (dp)
//		v |= B10000000;
//	//m_status[offset + digit] = v;
//	spiTransfer(addr, digit + 1, v);
//}
//---------------------------------------------------------------

//template<size_t m_maxDevices> void CLedControl<m_maxDevices>::spiTransfer(size_t addr, byte opcode,  byte data)
//{
//	//Create an array with the data to shift out
//	const size_t offset = addr * 2;
//	const size_t maxbytes = m_maxDevices * 2;
//	byte spidata[16] = { 0 };/* The array for shifting the data to the devices */
//
//	//for (int i = 0; i < maxbytes; i++)
//	//	spidata[i] = (byte)0;
//	//put our device data into the array
//	spidata[offset + 1] = opcode;
//	spidata[offset] = data;
//	
//	digitalWrite(m_SPI_CS, LOW);//enable the line 
//	//Now shift out the data 
//	for (size_t i = maxbytes; i; --i)
//		shiftOut(m_SPI_MOSI, m_SPI_CLK, MSBFIRST, spidata[i - 1]);
//	digitalWrite(m_SPI_CS, HIGH);//latch the data onto the display
//}    
//---------------------------------------------------------------

template<size_t m_maxDevices> size_t CLedControl<m_maxDevices>::getCharArrayPosition(char input)
{
     if ((input==' ')||(input=='+')) return 10;
     if (input==':') return 11;
     if (input=='-') return 12;
     if (input=='.') return 13;
     if ((input =='(')) return  14;  //replace by 'ñ'   
     if ((input >='0')&&(input <='9')) return (input-'0');
     if ((input >='A')&&(input <='Z')) return (input-'A' + 15);
     if ((input >='a')&&(input <='z')) return (input-'a' + 15);     
     return 13;
}  

//void LedControl::writeString(int mtx, char * displayString) 
//{
//  while ( displayString[0]!=0) 
//  {
//    char c =displayString[0];
//    int pos= getCharArrayPosition(c);
//    displayChar(mtx,pos);
//    delay(300);
//    clearAll();
//    displayString++;
//  }  
//}


//void LedControl::displayChar(int matrix, int charIndex) 
//{
//	//for matrix 8x8 indicator
//	const static byte alphabetBitmap[41][6] =
//	{
//		{0x7E,0x81,0x81,0x81,0x7E,0x0}, //0
//		{0x4,0x82,0xFF,0x80,0x0,0x0},  //1
//		{0xE2,0x91,0x91,0x91,0x8E,0x0},//2
//		{0x42,0x89,0x89,0x89,0x76,0x0},//3
//		{0x1F,0x10,0x10,0xFF,0x10,0x0},//4
//		{0x8F,0x89,0x89,0x89,0x71,0x0},//5
//		{0x7E,0x89,0x89,0x89,0x71,0x0},//6
//		{0x1,0x1,0xF9,0x5,0x3,0x0},//7
//		{0x76,0x89,0x89,0x89,0x76,0x0},//8
//		{0x8E,0x91,0x91,0x91,0x7E,0x0},//9
//		{0x0,0x0,0x0,0x0,0x0,0x0},// blank space
//		{0x0,0x0,0x90,0x0,0x0,0x0}, //:
//		{0x0,0x10,0x10,0x10,0x10,0x0},// -
//		{0x0,0x0,0x80,0x0,0x0,0x0},// .
//		{0xFC,0x9,0x11,0x21,0xFC,0x0},//?
//		{0xFE,0x11,0x11,0x11,0xFE,0x0},//A
//		{0xFF,0x89,0x89,0x89,0x76,0x0},//B
//		{0x7E,0x81,0x81,0x81,0x42,0x0},//C
//		{0xFF,0x81,0x81,0x81,0x7E,0x0},//D
//		{0xFF,0x89,0x89,0x89,0x81,0x0},//E
//		{0xFF,0x9,0x9,0x9,0x1,0x0},//F
//		{0x7E,0x81,0x81,0x91,0x72,0x0},//G
//		{0xFF,0x8,0x8,0x8,0xFF,0x0},//H
//		{0x0,0x81,0xFF,0x81,0x0,0x0},//I
//		{0x60,0x80,0x80,0x80,0x7F,0x0},//J
//		{0xFF,0x18,0x24,0x42,0x81,0x0},//K
//		{0xFF,0x80,0x80,0x80,0x80,0x0},//L
//		{0xFF,0x2,0x4,0x2,0xFF,0x0},//M
//		{0xFF,0x2,0x4,0x8,0xFF,0x0},//N
//		{0x7E,0x81,0x81,0x81,0x7E,0x0},//O
//		{0xFF,0x11,0x11,0x11,0xE,0x0},//P
//		{0x7E,0x81,0x81,0xA1,0x7E,0x80},//Q
//		{0xFF,0x11,0x31,0x51,0x8E,0x0},//R
//		{0x46,0x89,0x89,0x89,0x72,0x0},//S
//		{0x1,0x1,0xFF,0x1,0x1,0x0},//T
//		{0x7F,0x80,0x80,0x80,0x7F,0x0},//U
//		{0x3F,0x40,0x80,0x40,0x3F,0x0},//V
//		{0x7F,0x80,0x60,0x80,0x7F,0x0},//W
//		{0xE3,0x14,0x8,0x14,0xE3,0x0},//X
//		{0x3,0x4,0xF8,0x4,0x3,0x0},//Y
//		{0xE1,0x91,0x89,0x85,0x83,0x0}//Z
//	};
//
//  for (int i=0; i<6;i++) 
//      setRow(matrix,i, alphabetBitmap[charIndex][i]);
//}