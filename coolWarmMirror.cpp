//"cool warm mirror" project.
// authors:
// Iryna Kulbashevska
// Valentine Lazurik
// Coolbash inc. Kharkiv. Ukraine. 2017.
//---------------------------------------------------------------

//todo:
// - error condition check and indicate.
// - check CRC on DS18B20.
// - run-time COM-port commands:
//		- indicator brightness
//		- time setup
//		- on/off debug data.

#define DEBUG //includes traces and other debug tools. It's better to disable it for final release.
//#define USE_TIMER_4_SONAR

constexpr float max_mirror_temperature_C = 30;	//maximum temperature of the mirror. 

//---------------------------------------------------------------
//libraries
#include <limits.h>
#include "HTU21D.h"			//HTU21 sensor library (humidity and temperature)
#include "OneWire.h"		//OneWire bus sowtware implementation.
#include "RTClib.h"			//library for real time clock
#include "LedControlMS.h"	//library for indicator @maxim max7219
#include "RCFilter.h"		//RC-filter by Coolbash
#include "common.h"
#include <avr/wdt.h>
//---------------------------------------------------------------

/*
printf types and thair sizes:
%u		suze_t	1
%d		int		2
%lu		ulong	4
%ld		long	4
*/
//---------------------------------------------------------------
//inline void verify(bool condition, ...)//check condition and, if it isn't true, trace debug message
//{
//	if (!condition)
//	{
//		trace("\nassert: ");
//		va_list argList;
//		va_start(argList, condition);
//		trace(argList);
//		va_end(argList);
//	}
//}
enum Eerrors :byte
{
	error_no,
	error_18B20_no_answer,
	error_NoMemForAnotherProc,
	error_NoResponce_I2C_HTU21D,
	error_HTU21D_CRC,

};

//---------------------------objects-----------------------------
typedef unsigned long ms_t;				//type for milliseconds time measurement;
typedef unsigned long distance_t;		//type for distance (maybe byte is enough?)

//pins, inputs, outputs
const int pin_debug_led = 13;		//debug led on board
const int pin_sonar_ping = 3;//4;	//sonar trigger (ping)
const int pin_sonar_pong = 2;//7;	//sonar echo input (pong)
const int pin_switch_heater = 4;//2;
const int pin_switch_light = 7;// 3;
const int pin_dallas_1wire = 10;		//DS18B20 1wire pin
const int pin_SPI_MOSI = 8;		//	PIN_SPI_MOSI??
const int pin_SPI_CLK = 12;
const int pin_indicator_CS = 11;

//HTU21:
//VIN   3v3
//SDA   A4
//SCL   A5

//RTC21 5v    
//SDA   A4
//SCL   A5

//ijndicator: 5v
//CLK     D12
//CS      D11
//DIN     D8

//Dallas: when lookd at labels left to right: GND, data(D10), 5v;
//Sonar: 5v, trigger  D3, echo D2.
//Rele: 5v, in1-D4, in2 - D7.





//distance constants and variables
#ifdef DEBUG
const int distance_max_cm = 300;	//maximum measured distance in cm. Shorter distance - shorter timout.
#else
const int distance_max_cm = 130;	//maximum measured distance in cm. Shorter distance - shorter timout.
#endif
//const float usPcm = 29.1; //microseconds per cantimeter
const int usPcm = 30; //microseconds per cantimeter
const ms_t distance_timeout = distance_max_cm * usPcm * 2;
const size_t distances_size = 32;			//number of distances to keep
static_assert(size_t(-1) % distances_size == distances_size - 1, "possible wrap problem with this buffer size");

const int switch_distance_cm = 15;

//light
const ms_t light_min_time_ms = 500; //for not to switch light too often
const ms_t light_max_time_ms = 3600000;// 60 * 60 * 1000; //light timout (to turn it off automatically)

const ms_t timeTH = 3000;	//время индикации температуры/влажности в стандартном режиме
//--------------------------------------------------------------- Proc in this scope:
void light_on();		//turn on the light
void light_off();	//turn off the light
inline	void heater_on();	//turn on  the mirror heater
inline	void heater_off();	//turn off the mirror heater
inline	bool is_light_on();	//return true if the light is on

void error(const byte e);//shows "Exxx" and stops
void heartbit1();
void heartbit2();
void heartbit3();
void heartbit4();
void receive_t_air();
void request_humidity();
void receive_t_mirror();
void receive_humidity();
void time_show();
void brightness_control();
//---------------------------------------------------------------
class CprocManager
{
static constexpr size_t procs_max = 6; //max procedures in queue
public:
	struct CProc
	{
		ms_t	time;
		void (*proc)();
	};

	CProc	m_data[procs_max];
	size_t	m_size;

	//-----------------------------
	void removeProc(void(*f)())
	{
		CProc* cur = m_data;			//current element (the first one)
		CProc* last = cur + m_size;	//the last element in queue
		while (cur < last)
		{
			if (cur->proc == f)
			{
				TRACE_PROC("proc found at adress %x. Last=%x\n", cur, last);
				size_t num2move = last - cur - 1;
				if (num2move)
				{
					TRACE_PROC("moving from %x to %x %u elements", cur + 1, cur, num2move * sizeof(CProc));
					memmove(cur, cur + 1, num2move * sizeof(CProc));//delete the current element
				}
				last = cur + --m_size;	//the last element in queue
				break;
			}
			else
				cur++;
		}
	}
	//-----------------------------
	void push(ms_t t, void(*f)())
	{
		//dump();
		TRACE_PROC("add proc with t=%lu and f=%x\n", t, f);
		if (m_size == procs_max)
			error(error_NoMemForAnotherProc);
		removeProc(f);//first off: check if the proc already in the queue
		CProc* cur = m_data;			//current element (the first one)
		CProc* last = cur + m_size;	//the last element in queue

		//find a place to insert into.
		while (cur < last)
		{
			long dif = t - cur->time;
			if (dif < 0)
			{
				memmove(cur + 1, cur, (char*)last - (char*)cur);//must insert before cur
				break;
			}
			else cur++;
		}
		cur->time = t;
		cur->proc = f;
		m_size++;
		//dump();
	};
	//-----------------------------
	void pop()//runs a first waiting function and removes it from the queue
	{
		auto proc = m_data[0].proc;
		memmove(m_data, m_data + 1, --m_size * sizeof(CProc));//must insert before cur
		proc();	//now the proc can insert itself again.
		//dump();
	}
	//-----------------------------
#ifdef DEBUG
	void dump()//for debug 
	{
		//TRACE_PROC("procedures in time queue (element size = %u):\n", sizeof(CProc));
		for (size_t i = 0; i < m_size; i++)
			TRACE_PROC("%u:\t%lu\t%x\n", i, m_data[i].time, m_data[i].proc);
		TRACE_PROC("\n");
	}
#endif
	//-----------------------------
	bool check() //returns true if a function was called
	{
		if (m_size)
		{
			long dif = m_data[0].time - millis();
			if (dif <= 0)
			{
				pop();
				return true;
			}
		}
		return false;
	};
};
//---------------------------------------------------------------
CprocManager			procManager;
//---------------------------------------------------------------
void error(const byte e)//shows "Exxx" and stops
{
	light_off();
	heater_off();
	TRACE_ERROR("ERROR: ");
	switch (e)
	{
	case error_18B20_no_answer: TRACE_ERROR("\n18B20 not present\n"); /*indicator.setChar(0,,")*/break;
	case error_NoMemForAnotherProc: TRACE_ERROR("\nerror_NoMemForAnotherProc\n"); /*indicator.setChar(0,,")*/break;
	case error_NoResponce_I2C_HTU21D: TRACE_ERROR("\nerror_NoResponce_I2C_HTU21D\n"); /*indicator.setChar(0,,")*/break;
	default:	TRACE_ERROR("\nerror_Unknown\n");
	}
	delay(3000);

	//reset the whole program
	void(* resetFunc) (void) = 0;//declare reset function at address 0
	resetFunc(); //call reset  
}

//---------------------------------------------------------------

//structures for RTC_DS1307
union RTC_DS1307_seconds_t		//seconds and clock-halt bit
{
	struct
	{
		byte	s1 : 4;
		byte	s10 : 3;
		byte	ch : 1;	//clock halt bit (if 1, the clock is stopped)
	};
	byte s = 0;
};

union RTC_DS1307_hm_t		//hours and minutes in common 16-bit structure
{
	struct
	{
		union
		{
			struct
			{
				byte m1 : 4;
				byte m10 : 3;
			};
			byte m;
		};
		union
		{
			struct
			{
				byte h1 : 4;
				byte h24 : 2;
				byte bit12or24 : 1;	//if 1 - 12 bit format. 0 -  24 bit format
			};
			byte h;
		};
	};
	uint16_t	hm;	//hours and minutes only
};





//classes and structures
//---------------------------------------------------------------
struct CDistance
{
	distance_t		distances[distances_size] = { distance_max_cm };		//buffer for measured distances
	ms_t			moments[distances_size] = { 0 };		//measured times
	size_t			measured = 0;		//how many distances already measured
	size_t			analized = 0;	//how many distances already analized
	ms_t			last_hand_off_time_ms = 0;
	ms_t			last_hand_on_time_ms = 0;
	ms_t			light_switch_time_ms = 0;	//last light switch time
	ms_t			ping_time_ms = 0;
	ms_t			pong_begin_us = 0;
	bool			hand_on = false;

	void			push(distance_t dist, ms_t t) { size_t i = ((measured++) % distances_size); distances[i] = dist; moments[i] = t; };
	distance_t		get_distance(size_t index) { return distances[index % distances_size]; };
	ms_t			get_time(size_t index) { return moments[index % distances_size]; };
	void			get(distance_t& d, ms_t& t, size_t index) { size_t i = index % distances_size; d = distances[i]; t = moments[i]; };

	//-----------------------------
	void start()	//sending a ping-pulse
	{
		digitalWrite(pin_sonar_ping, HIGH);
		//delayMicroseconds(1);	//a couple of next lines is enough for delay
		ping_time_ms = millis();
		pong_begin_us = 0;
		digitalWrite(pin_sonar_ping, LOW);
	}
	//-----------------------------
	//void			measure()	
	//{
	//	start();
	//	auto duration_us = pulseIn(pin_sonar_pong, HIGH, distance_timeout);
	//	distance_t cm = duration_us / (usPcm * 2);
	//	TRACE_DISTANCE_ANALYZE("#%u:%lums-%luµs=%lucm\t", measured, time - get_time(measured - 1), duration, cm);
	//	push(cm, ping_time_ms);
	//	//dump();
	//};
	//-----------------------------
	void switch_light()
	{
		if (is_light_on())
			light_off();
		else
			light_on();
		light_switch_time_ms = millis();	//last light switch time
	}
	//-----------------------------
#ifdef DEBUG
	char in_que_max = 0;
#endif

	void analize()// check data in the buffer and analize it (switch the light)
	{
		//const int hands_off_time_ms = 500; 
		const int hand_time_ms = 40;
		const distance_t proximity_distance_cm = 15;

		distance_t d;
		ms_t t;
		//get(prev_d, prev_t, analized);
#ifdef DEBUG
		char in_queue = measured - analized;
		if (in_que_max < in_queue)
		{
			in_que_max = in_queue;
			TRACE_DISTANCE_ANALYZE("queue size = %u\n", in_que_max);
		}

#endif
		while (analized != measured) //shouldn't use < sign due to wrap
		{
			get(d, t, analized++);
			TRACE_DISTANCE_ONLY("%d\n", d); 
			TRACE_DISTANCE_ANALYZE("%u/%u(%lums): %lucm@%lums, light:%lums, off:%lums, on:%lums %s\n", measured, analized, t - get_time(analized - 2), d, t, light_switch_time_ms, last_hand_off_time_ms, last_hand_on_time_ms, hand_on ? "ON " : "off");
			if (d < proximity_distance_cm)
			{//in a proximity range
				if (last_hand_on_time_ms <= last_hand_off_time_ms)
					last_hand_on_time_ms = t;
				else
					if (!hand_on && t - last_hand_on_time_ms > hand_time_ms)
					{
						hand_on = true;
						if (last_hand_on_time_ms - light_switch_time_ms > light_min_time_ms)
							switch_light();
					}
			}
			else
			{
				if (last_hand_on_time_ms > last_hand_off_time_ms)
					last_hand_off_time_ms = t;
				else
					if (hand_on && t - last_hand_off_time_ms > hand_time_ms)
						hand_on = false;
			}
		}
	}
	//-----------------------------
	//void autoLightOff()	//automatically switches light off after light-on timeout
	//{
	//	if (is_light_on() && (millis() - light_switch_time_ms > light_max_time_ms)) //timeout
	//			light_off(), light_switch_time_ms = millis();	//last light switch time
	//}
	//void dump()
	//{
	//	const size_t sz_dbg_size = 32;
	//	char sz_dbg[sz_dbg_size];
	//	distance_t d;
	//	ms_t	t;
	//	Serial.print(measured);
	//	Serial.print(": ");
	//	for (size_t i = 0; i < distances_size; i++)
	//	{
	//		//d = distances[i];
	//		//t = moments[i];
	//		get(d, t, i);
	//		snprintf(sz_dbg, sz_dbg_size, "%lu-%lu ", t,d);
	//		Serial.print(sz_dbg);
	//	}
	//	Serial.println();
	//}

};
//---------------------------------------------------------------

//struct CDecimalPointHeartBeat	//структура (класс) для реализации ежесекундного мигания десятичной точки а-ля сердцебиение.
//{
//	char			phase;
//	ms_t			starttime;
//	boolean			light;
//	const ms_t phase_times_ms[4] = { 50,200,50,700 };
//	CDecimalPointHeartBeat() { phase = 0; starttime = 0; light = true; }
//
//	void point()
//	{
//		if (millis() - starttime > phase_times_ms[phase])
//		{
//			light = !light;
//			starttime += phase_times_ms[phase];
//			if (phase == 3) phase = 0;
//			else phase++;
//		}
//	}
//
//};
//---------------------------------------------------------------

//---------------------------------------------------------------


//variables
CFilterRC_HiPass		filter_H_diff;
CFilterRC_HiPass		filter_T_diff;

CDistance				distance;
//CDecimalPointHeartBeat	HeartBeatPoint;
RTC_DS1307				RTC;						//real time clock DS1307
HTU21D					htu21;						//humidity and temperature sensor htu21
OneWire					oneWire(pin_dallas_1wire);	// one_wire object for dallas sensor
CLedControl<1>			indicator(pin_SPI_MOSI, pin_SPI_CLK, pin_indicator_CS);	//7-segment indicator
//CDateTime				time;						//этот класс лучше не создавать в процедуре, которая запускается в цикле, поскольку конструктор делает много работы.
ms_t					tstartTH,
HoldTime = 0,
heartbeat_start_time = 0;

float					T_last = 0;		//last measured air temperature
float					H_last = 0;		//last measured humidity
float					T_diff, H_diff;	//changing of temperature and humidity
float					T_mirror = 0;	//morror temperature
float					humidity_air_prev = 0;	
RTC_DS1307_hm_t			rtc_hm;	//hours and minutes for display

char brightness = 4;
bool brightness_phase = false;

bool					showTorH = true;
bool					changemode = false;
bool					heating = false;
bool					time_heartbit_light = false;

//functions to run:
bool					bTair_read = false;
bool					bHair_read = false;
bool					bTm_read = false;
bool					bAnalizeTH = false;

//---------------------------------------------------------------
void light_on() { digitalWrite(pin_switch_light, LOW); procManager.push(millis() + light_max_time_ms, light_off); }
void light_off() { digitalWrite(pin_switch_light, HIGH); }
inline	void heater_on() { digitalWrite(pin_switch_heater, LOW); heating = true;}
inline	void heater_off() { digitalWrite(pin_switch_heater, HIGH); heating = false;}
inline	bool is_light_on() { return digitalRead(pin_switch_light) == LOW; }
//---------------------------------------------------------------
void distance_interruption_routine()
{
	ms_t time_us = micros(); //first thing - get the time
	if (digitalRead(pin_sonar_pong) == HIGH)
		distance.pong_begin_us = time_us;
	else
	{
		if (distance.pong_begin_us)
		{
			ms_t dif = time_us - distance.pong_begin_us;
			distance_t d = dif / (usPcm * 2);
			distance.push(d, distance.ping_time_ms);
		}
		else
			distance.push(0, distance.ping_time_ms);
#ifndef USE_TIMER_4_SONAR
		distance.start();	// send ping pulse just at the end of pong responce.
#endif
	}
}
#ifdef USE_TIMER_4_SONAR
//--------------------------------------------------------------- timer interrupt routine
ISR(TIMER1_COMPA_vect)
{
	distance.start();
}
#endif
//---------------------------------------------------------------	
//watchdog timer routine
ISR (WDT_vect)
{
	wdt_reset();
	wdt_disable();  // disable watchdog
					//reset the whole program
	heater_off();
	light_off();
	void(* resetFunc) (void) = 0;//declare reset function at address 0
	resetFunc(); //call reset  
}  // end of WDT_vect
//---------------------------------------------------------------	

void float2indicator(float src, bool right, char c02 = '*', char c03 = 'C')
{
	size_t digit = right ? 4 : 8;

	int i = lrint(src * 100);
	char c = i / 1000;
	indicator.setChar(0, --digit, c, false);
	i -= c * 1000;
	c = i / 100;
	i -= c * 100;
	if (i)
	{
		indicator.setChar(0, --digit, c, true);
		indicator.setChar(0, --digit, i / 10, false);
		indicator.setChar(0, --digit, i % 10, false);
	}
	else
	{
		indicator.setChar(0, --digit, c, false);
		indicator.setChar(0, --digit, c02, false);
		indicator.setChar(0, --digit, c03, false);
	}
}
//---------------------------------------------------------------

void hello()	//hello. Don't need, accually.
{
	for (int i = 0; i < 12; i++)
	{
		indicator.clearDisplay(0);
		if (i < 8)
			indicator.setChar(0, i		, 'H', false);
		if ((i > 0) && (i < 9))
			indicator.setChar(0, i - 1	, 'E', false);
		if ((i > 1) && (i < 10))
			indicator.setChar(0, i - 2	, 'L', false);
		if ((i > 2) && (i < 11))
			indicator.setChar(0, i - 3	, 'L', false);
		if (i > 3)
			indicator.setChar(0, i - 4	, '0', false);
		delay(150);
	}

	heater_on();
	delay(300);
	heater_off();
	delay(300);
	light_on();
	delay(300);
	light_off();
	indicator.clearDisplay(0);

}
//---------------------------------------------------------------
inline void heartbit_show()
{
	if (!heating)
		indicator.setChar(0, 2, rtc_hm.h1, time_heartbit_light);
}
//---------------------------------------------------------------
//---------------------------------------------------------------
void heartbit1()
{
	time_heartbit_light = true;
	heartbit_show();
	procManager.push(heartbeat_start_time + 50, heartbit2);
}
//---------------------------------------------------------------
void heartbit2()
{
	time_heartbit_light = false;
	heartbit_show();
	procManager.push(heartbeat_start_time + 250, heartbit3);
}
//---------------------------------------------------------------
void heartbit3()
{
	time_heartbit_light = true;
	heartbit_show();
	procManager.push(heartbeat_start_time + 300, heartbit4);
}
//---------------------------------------------------------------
void heartbit4()
{
	time_heartbit_light = false;
	heartbit_show();
	procManager.push(heartbeat_start_time += 1000, heartbit1);
}
//---------------------------------------------------------------
void request_t_air()
{
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
	Wire.endTransmission();

	procManager.push(millis() + 55, receive_t_air);
}
//---------------------------------------------------------------
void receive_t_air()
{
	Wire.requestFrom(HTDU21D_ADDRESS, 3);

	//Wait for data to become available
	int counter = 0;
	while (Wire.available() < 3)
	{
		counter++;
		delay(1);
		if (counter > 100)
			error(error_NoResponce_I2C_HTU21D);// return 998; //Error out
	}

	unsigned char msb, lsb, checksum;
	msb = Wire.read();
	lsb = Wire.read();
	checksum = Wire.read();

	/* //Used for testing
	byte msb, lsb, checksum;
	msb = 0x68;
	lsb = 0x3A;
	checksum = 0x7C; */

	unsigned int rawTemperature = ((unsigned int)msb << 8) | (unsigned int)lsb;

	if (htu21.check_crc(rawTemperature, checksum) != 0) error(error_HTU21D_CRC);// return(999); //Error out

	//sensorStatus = rawTemperature & 0x0003; //Grab only the right two bits
	rawTemperature &= 0xFFFC; //Zero out the status bits but keep them in place

	//Given the raw temperature data, calculate the actual temperature
	float tempTemperature = rawTemperature / (float)65536; //2^16 = 65536
	T_last = -46.85 + (175.72 * tempTemperature); //From page 14
	bTair_read = true;
	procManager.push(millis() + 10, request_humidity);

}
//---------------------------------------------------------------
void request_t_mirror()
{
	if (oneWire.reset())
	{
		oneWire.write(0xCC);
		oneWire.write(0x44);
		procManager.push(millis() + 750, receive_t_mirror);
	}
	else
		error(error_18B20_no_answer);
	procManager.push(millis() + 10, request_t_air);
}
//---------------------------------------------------------------
void receive_t_mirror()
{
	if (oneWire.reset())
	{
		oneWire.write(0xCC);
		oneWire.write(0xBE);
		const int bytes2read = 9;
		byte data[bytes2read];
		oneWire.read_bytes(data, bytes2read);
		//data[0] = oneWire.read();
		//data[1] = oneWire.read();
		//it would be nice to check CRC here
		T_mirror = *(int*)data * 0.0625;
		//temperature_mirror = (data[1] << 8) + data[0];
		//byte t_mirror_low_part = data[0] & 0x0f;	//low part (after decimal point)
		//byte t_mirror_after_decimal_4 = t_mirror_low_part * 625;
		//temperature_mirror = temperature_mirror >> 4;

		bTm_read = true;
		if (heating)
			float2indicator(T_mirror, true);
	}
	else
		error(error_18B20_no_answer);
}
//---------------------------------------------------------------
void request_humidity()
{
	Wire.beginTransmission(HTDU21D_ADDRESS);
	Wire.write(TRIGGER_HUMD_MEASURE_NOHOLD); //Measure humidity with no bus holding
	Wire.endTransmission();
	procManager.push(millis() + 55, receive_humidity);//Hang out while measurement is taken. 50mS max, page 4 of datasheet.
}
//---------------------------------------------------------------
void receive_humidity()
{
	//Comes back in three bytes, data(MSB) / data(LSB) / Checksum
	Wire.requestFrom(HTDU21D_ADDRESS, 3);

	//Wait for data to become available
	int counter = 0;
	while (Wire.available() < 3)
	{
		counter++;
		delay(1);
		if (counter > 100)
			error(error_NoResponce_I2C_HTU21D);// return 998; //Error out
	}

	byte msb, lsb, checksum;
	msb = Wire.read();
	lsb = Wire.read();
	checksum = Wire.read();

	/* //Used for testing
	byte msb, lsb, checksum;
	msb = 0x4E;
	lsb = 0x85;
	checksum = 0x6B;*/

	unsigned int rawHumidity = ((unsigned int)msb << 8) | (unsigned int)lsb;

	if (htu21.check_crc(rawHumidity, checksum) != 0) error(error_HTU21D_CRC);// return(999); //Error out

	//sensorStatus = rawHumidity & 0x0003; //Grab only two low bits
	rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place

	//Given the raw humidity data, calculate the actual relative humidity
	float tempRH = rawHumidity / (float)65536; //2^16 = 65536
	H_last = -6 + (125 * tempRH); //From page 14 of the datasheet

	bHair_read = true;
}
//---------------------------------------------------------------
inline float dew_point(float t, float h) {return t-(100-h)/5;};//dew point temperature		//Td = T - ((100 - RH)/5.)	
//---------------------------------------------------------------

void warming()
{
#ifdef DEBUG
	ms_t	start = micros();
#endif // DEBUG
	//filtering temperature and humidity data
	H_diff = filter_H_diff.HiPass(H_last);
	T_diff = filter_T_diff.HiPass(T_last);

	float const T_dew_point_predict = dew_point(T_last, H_last+H_diff*3);

	float T_warming = T_last; //temperature to warm up the mirror
	if (T_diff > 0)
		T_warming += T_diff; //adding current temperature gradient to warm ahead in fast warming environment
	if(T_warming > max_mirror_temperature_C)
		T_warming = max_mirror_temperature_C;

	bool cooling_criteria = false		//criterias to turning off the heater
		||	T_mirror > T_warming + 8	//if mirror temperature is achieved
		||	H_diff < 0					//humidity began dropping.
		;

	bool heating_criteria =	false			//criterias for heating
		//|| H_last - H_mean > 5			//humidity is 5% above mean value
		|| H_diff > 2						//humidity is rising
		//|| H_last > 60						//humidity is more than 60%
		|| T_mirror < T_dew_point_predict	
		//&& (T_mirror < T_warming
		;

	if (heating && cooling_criteria)
	{
		{
			//turning the heater off
			heater_off();
			time_show();
		}
	}
	else
		if (heating_criteria && !cooling_criteria)
		{
			//turning the heater on
			heater_on();
			float2indicator(T_mirror, true);
			brightness_control();
		}

	bAnalizeTH = false;
	bTair_read = false;
	bHair_read = false;
	bTm_read = false;

	procManager.push(millis() + 250, request_t_mirror);
	TRACE_HT("humidity=%s%%	(diff=%5s);	t_air=%s°C	(diff=%5s);	t_mirror=%s°C.	dew_point_now=%s°C.		%s (proc took %luus)\n",
			toS(H_last),	toS(H_diff), toS(T_last), toS(T_diff), toS(T_mirror),toS(dew_point(T_last, H_last)), heating ? "heating" : "cooling",micros()-start);
	TRACE_HT_LOG(	"%s	"		"%5s	"	"%s	"			"%5s	"	"%s	"			"%s	"						"%s	"									"%s	\n",
					toS(H_last),	toS(H_diff), toS(T_last),	toS(T_diff), toS(T_mirror),	toS(T_dew_point_predict),	heating ? "heating" : "cooling",	is_light_on()? "light" :"");
	//wdt_reset();
}
//---------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void distance_old()
//{
//	unsigned long duration, cm, start;
//	//
//	
//	digitalWrite(pin_sonar_ping, HIGH);
//	delayMicroseconds(10);
//	digitalWrite(pin_sonar_ping, LOW);
//	duration = pulseIn(pin_sonar_pong, HIGH, distance_timeout);
//
//	//delayMicroseconds(500);
//	//start = micros();
//	//digitalWrite(13, HIGH);
//	//while (digitalRead(pin_sonar_pong) == HIGH)
//	//{
//	//	duration = micros() - start;
//	//	if (duration > 15000)
//	//		break;
//	//}
//	//digitalWrite(13, LOW);
//
//	cm = duration / (29.412 * 2);
//#ifdef DEBUG
//	Serial.print(duration);
//	Serial.print("us = ");
//	Serial.print(cm);
//	Serial.print("cm; ");
//
//	////на одном из индикаторов у нас наблюдалась проблема с глюками. 
//	////При этом помогала очистка экрана. Но очистку нельзя делать в рабочем цикле.
//	////Был введен хук: если держать руку на расстоянии менее 5см более 3 секунд, индикатор очищался.
//	//if (cm < 5)
//	//{
//	//	if (HoldTime == 0)
//	//	{
//	//		HoldTime = millis();
//	//		return;
//	//	}
//	//	if (millis() - HoldTime >= 3000)
//	//	{
//	//		Serial.println("Resetting Led indicator");
//	//		//LedControl lc2(8,12,11,1);
//	//		//lc = LedControl(8,12,11,1);
//
//	//		indicator.clearAll();
//	//		//lc.shutdown(0,true);
//	//		//delay(1000);
//	//		//lc.shutdown(0,false);
//	//		//lc.setIntensity(0,4);
//	//		HoldTime = 0;
//	//	}
//	//	return;
//	//}
//#endif // DEBUG
//	if (cm < 15 && distance_cm_prev>15 && cm != 4 && cm != 5)
//	{
//		if (digitalRead(pin_switch_light) == LOW)
//		{
//			digitalWrite(pin_switch_light, HIGH);
//			//    Serial.println(cm);
//			//    Serial.println("Light is off");
//		}
//		else
//		{
//			digitalWrite(pin_switch_light, LOW);
//			//     Serial.println("Light is on");
//		}
//	}
//	distance_cm_prev = cm;
//}
//
//-----------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
void time_read()
{
	RTC_DS1307_hm_t			hm;	//hours and minutes f
	RTC_DS1307_seconds_t	seconds;	//hours and minutes f

	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(char(0));
	Wire.endTransmission();

	Wire.requestFrom(DS1307_ADDRESS, 3);
	seconds.s = Wire.read();
	hm.m = Wire.read();
	hm.h = Wire.read();

	if (hm.hm != rtc_hm.hm)
	{
		rtc_hm.hm = hm.hm;
		if (!heating)
			time_show();
	}
	//add itseslf to timed quere
	byte seconds_till_change = 60 - (seconds.s1 + seconds.s10 * 10);
	if (0 >= seconds_till_change)
		seconds_till_change = 1;

	TRACE_TIME("time: %u%u:%u%u:%u%u. (%s mode, %s). next change in %us\n",hm.h24,hm.h1, hm.m10,hm.m1, seconds.s10,seconds.s1,hm.bit12or24?"am/pm":"24h",seconds.ch?"stopped":"ticking", seconds_till_change);
	procManager.push(millis() + ms_t(1000) * seconds_till_change, time_read); //push itself
}

//---------------------------------------------------------------
void time_show()
{
	indicator.setChar(0, 3, rtc_hm.h24, false);
	indicator.setChar(0, 2, rtc_hm.h1, time_heartbit_light);
	indicator.setChar(0, 1, rtc_hm.m10, false);
	indicator.setChar(0, 0, rtc_hm.m1, false);
}
//---------------------------------------------------------------

void humidity_show()
{
	indicator.setChar(0, 7, H_last / 10, false);
	indicator.setChar(0, 6, int(H_last) % 10, false);
	indicator.setChar(0, 5, '*', false);
	indicator.setChar(0, 4, '%', false);
}
//---------------------------------------------------------------

void temperature_air_show()
{
	indicator.setChar(0, 7, T_last / 10, false);
	indicator.setChar(0, 6, int(T_last) % 10, false);
	indicator.setChar(0, 5, '*', false);
	indicator.setChar(0, 4, 'C', false);
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------

void indicatorTHb()
{
	if (millis() - tstartTH > timeTH)
	{
		showTorH = !showTorH;
		tstartTH += timeTH;
		humidity_air_prev = H_last;
	}
	if (showTorH) humidity_show();
	else temperature_air_show();
}
//---------------------------------------------------------------

void indicatorTH()//shows air temperature and humidity
{
	if (heating)
	{
		float2indicator(T_last, false);
		//temperature_air_show();
		return;
	}
	if (changemode)
	{
		if (H_last < humidity_air_prev)
		{
			changemode = false;
			tstartTH = millis();
			indicatorTHb();
			return;
		}
		else
		{
			humidity_show();
			humidity_air_prev = H_last;
			return;
		}
	}
	if (H_last - humidity_air_prev > 1)
	{
		changemode = true;
		tstartTH = millis();
		humidity_show();
		return;
	}
	indicatorTHb(); //recursion
}
//---------------------------------------------------------------
void brightness_control()
{
	if (heating)
	{
		if (brightness_phase)
			if (brightness == 15)
				brightness_phase = false, brightness--;
			else
				brightness++;
		else
			if (brightness == 0)
				brightness_phase = true, brightness++;
			else
				brightness--;
		procManager.push(millis() + 40, brightness_control);
	}
	else
		brightness = 4;
	indicator.setIntensity(0, brightness & 0x0f);
}
//---------------------------------------------------------------
void setup()
{
#ifdef DEBUG
	Serial.begin(115200);// (230400);
	TRACE_SIZES("sizeof(long)=%u, sizeof(int)=%u, sizeof(size_t)=%u, sizeof(short)=%u\n", sizeof(long), sizeof(int), sizeof(size_t), sizeof(short));

#endif // DEBUG
	htu21.begin();
	Wire.begin();
	//RTC.begin();
	//time_raw t;


	//RTC.adjust(CDateTime(__DATE__, __TIME__));  //setting up RTC time to compile time. 

	pinMode(pin_sonar_pong, INPUT);
	pinMode(pin_sonar_ping, OUTPUT);
	pinMode(pin_debug_led, OUTPUT);

	digitalWrite(pin_sonar_ping, LOW);

	pinMode(pin_switch_heater, OUTPUT);
	pinMode(pin_switch_light, OUTPUT);
	digitalWrite(pin_switch_heater, HIGH);
	digitalWrite(pin_switch_light, HIGH);

	indicator.shutdown(0, false);
	indicator.setIntensity(0, 4);
	//indicator.clearDisplay(0);
	hello();

	//watchdog timer setup
	//delay(2000)	;
	//hello();
	wdt_reset();
	//init watchdog timer
	//wdt_enable(WDTO_2S);
	//wdt_reset();
	{
		noInterrupts();
		WDTCSR = B00011000;
		WDTCSR = B01001110;
		interrupts();
	}
	time_read();	//read the time for the first time
	attachInterrupt(digitalPinToInterrupt(pin_sonar_pong), distance_interruption_routine, CHANGE);

#ifdef USE_TIMER_4_SONAR
	// initialize timer1 //https://www.robotshop.com/community/forum/t/arduino-101-timers-and-interrupts/13072
	noInterrupts();				// disable all interrupts
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 256;				// compare match register 16MHz/256/2Hz
	TCCR1B |= (1 << WGM12);		// CTC mode
	TCCR1B |= (1 << CS12);		// 256 prescaler 
	TCCR1B |= (1 << CS10);		// 1024 prescaler 
	TIMSK1 |= (1 << OCIE1A);	// enable timer compare interrupt
	interrupts();				// enable all interrupts
#else
	distance.start();
#endif

	humidity_air_prev = htu21.readHumidity();
	//filter_H_mean.SetParam(3600, 1);
	//filter_T_mean.SetParam(3600, 1);
	filter_H_diff.SetParam(10, 1);
	filter_T_diff.SetParam(10, 1);
	//filter_H_mean.m_Out = humidity_air_prev;
	filter_H_diff.m_Out = humidity_air_prev;
	filter_T_diff.m_Out = htu21.readTemperature();

	request_t_mirror();
	heartbeat_start_time = millis();
	heartbit1();
	//brightness_control();
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
byte phase = 0;
void loop()
{
#ifdef DEBUG
	auto loopstart = millis();
#endif
	if (!procManager.check())
	{
		if (bTair_read && bHair_read && bTm_read)
			warming();
		else
			switch (++phase)
			{
			case 1: distance.analize(); break;
			case 2: indicatorTH(); break;
			//case 3: t_mirror_show(); break;
			default: phase = 0;
			}
	}
	wdt_reset();

	TRACE_LOOP("loop=%lims\n",millis() - loopstart);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
