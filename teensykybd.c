#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "usb_dual.h"
#include "usb_keyboard.h"
#include "eeprom.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

int8_t flash_leds = 0;
#define TURN_OFF_LEDS() do { PORTD &= 0b11111001; PORTB &= 0b10011111; } while(0)
#define LED_NONE	0x00
#define LED_RED		0x01
#define LED_BLU		0x02
#define LED_GRN		0x04
#define LED_WHT		0x08
volatile int8_t which_led	= LED_NONE;
void set_led_state(int8_t which)
{
	cli();
	TURN_OFF_LEDS();
	which_led = which;
	switch(which)
	{
	case LED_RED:  PORTB |= 0b00100000; break;
	case LED_BLU:  PORTB |= 0b01000000; break;
	case LED_GRN:  PORTD |= 0b00000010; break;
	case LED_WHT:  PORTD |= 0b00000100; break;
	default:       which_led = LED_NONE;
	}
	sei();
}
int8_t get_led_state()
{
	return which_led;
}

int is_btn_down()
{
	return ((PINE & 0b01000000)==0);
}

char buf[256];

//
// Watchdog reset
//
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

#define soft_reset()        \
do                          \
{                           \
    wdt_enable(WDTO_15MS);  \
    for(;;)                 \
    {                       \
    }                       \
} while(0)


void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();

    return;
}

int b_mode_kybd = 1;

static uint8_t k[] = {KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,KEY_H,KEY_I,KEY_J,
			KEY_K,KEY_L,KEY_M,KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,KEY_S,
			KEY_T,KEY_U,KEY_V,KEY_W,KEY_X,KEY_Y,KEY_Z};

static uint8_t nums[] = {KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};

void kybd_type(char* pbuf,int bcnt)
{
	for(int i=0;i<bcnt;++i)
	{
		if ((pbuf[i]>='A') && (pbuf[i]<='Z'))
		{
			int x = pbuf[i] - 'A';
			usb_keyboard_press(k[x], KEY_SHIFT);
		} else if ((pbuf[i]>='a') && (pbuf[i]<='z')) {
			int x = pbuf[i] - 'a';
			usb_keyboard_press(k[x], 0);
		} else if ((pbuf[i]>='0') && (pbuf[i]<='9')) {
			int x = pbuf[i] - '0';
			usb_keyboard_press(nums[x], 0);
		} else if (pbuf[i]==' ') {
			usb_keyboard_press(KEY_SPACE, 0);
		} else if ((pbuf[i]=='\n') || (pbuf[i]=='\r')) {
			usb_keyboard_press(KEY_ENTER, 0);
		} else if (pbuf[i]=='.') {
			usb_keyboard_press(KEY_PERIOD, 0);
		} else if (pbuf[i]=='/') {
			usb_keyboard_press(KEY_SLASH, 0);
		} else if (pbuf[i]=='!') {
			usb_keyboard_press(KEY_1, KEY_SHIFT);
		} else if (pbuf[i]=='"') {
			usb_keyboard_press(KEY_QUOTE, KEY_SHIFT);
		} else if (pbuf[i]=='\'') {
			usb_keyboard_press(KEY_QUOTE, 0);
		} else if (pbuf[i]=='?') {
			usb_keyboard_press(KEY_SLASH, KEY_SHIFT);
		} else if (pbuf[i]=='=') {
			usb_keyboard_press(KEY_EQUAL, 0);
		} else if (pbuf[i]=='{') {
			usb_keyboard_press(KEY_LEFT_BRACE, 0);
		} else if (pbuf[i]=='}') {
			usb_keyboard_press(KEY_RIGHT_BRACE, 0);
		} else if (pbuf[i]==';') {
			usb_keyboard_press(KEY_SEMICOLON, 0);
		} else if (pbuf[i]==',') {
			usb_keyboard_press(KEY_COMMA, 0);
		} else {
			usb_keyboard_press(KEY_MINUS, 0);
		}
	}
}

void numset(uint32_t n,char* pbuf)
{
	char tmp[16];
	memset(tmp,0,sizeof(tmp));
	int pos = 0;

	while(n>0)
	{
		int digit = n % 10;
		switch(digit)
		{
		case 0:  tmp[pos++] = '0'; break;
		case 1:  tmp[pos++] = '1'; break;
		case 2:  tmp[pos++] = '2'; break;
		case 3:  tmp[pos++] = '3'; break;
		case 4:  tmp[pos++] = '4'; break;
		case 5:  tmp[pos++] = '5'; break;
		case 6:  tmp[pos++] = '6'; break;
		case 7:  tmp[pos++] = '7'; break;
		case 8:  tmp[pos++] = '8'; break;
		case 9:  tmp[pos++] = '9'; break;
		default:  tmp[pos++] = '?'; 
		}
		n = n / 10;
	}

	for(int i=pos-1; i>=0; i--)
	{
		pbuf[0] = tmp[i];
		pbuf++;
	}

}

int main(void)
{
	// set for 16 MHz clock
	CPU_PRESCALE(0);

        // Configure timer 0 to generate a timer overflow interrupt every
        // 256*1024 clock cycles, or approx 61 Hz when using 16 MHz clock
	TCCR0A = 0x00;
	TCCR0B = 0x05;
	TIMSK0 = (1<<TOIE0);

	DDRD = 0b00000110;
	DDRB = 0b01100000;
	DDRE = 0b00000000;
	PORTD = 0b01111000;
	PORTB = 0xFF;
	PORTE = 0b01000000;

	set_led_state(LED_NONE);

	flash_leds = 0;
	usb_init();
        while (!usb_configured()) ;
	_delay_ms(1000); // Wait for PC

// Cap sensing experiment
	DDRC = 0b00000000; // C7 is input
	PORTC = 0b01111111; // C7 is not pulled up
	DDRF = 0b11111111; // F7 is output
for(;;)
{
	PORTF = 0b11111111; // F7 high
	_delay_ms(1000);
	PORTF = 0b00000000; // F7 low
	uint32_t cntr = 0;
	for(;;cntr++)
	{
		uint8_t pc = PINC;
		if (pc & 0b10000000) {} else {break;}
	}
	memset(buf,0,sizeof(buf));
	numset(cntr,buf);
	kybd_type(buf,32);
}
//for(;;){}

	int vert = 0, horz = 0;
	uint8_t prev_pind = PIND;
	while (1) 
	{
		uint8_t pd = PIND;

		if ((pd & 0b00001000) != (prev_pind & 0b00001000)) {
			if (vert>-10) vert--;
		} else if ((pd & 0b00010000) != (prev_pind & 0b00010000)) {
			if (vert<10) vert++;
		} else if ((pd & 0b00100000) != (prev_pind & 0b00100000)) {
			if (horz>-10) horz--;
		} else if ((pd & 0b01000000) != (prev_pind & 0b01000000)) {
			if (horz<10) horz++;
		}
		prev_pind = pd;

		if ((vert>=0)&&(horz>=0)&&(get_led_state()!=LED_RED)) set_led_state(LED_RED);
		else if ((vert>=0)&&(horz<0)&&(get_led_state()!=LED_BLU)) set_led_state(LED_BLU);
		else if ((vert<0)&&(horz>=0)&&(get_led_state()!=LED_WHT)) set_led_state(LED_WHT);
		else if ((vert<0)&&(horz<0)&&(get_led_state()!=LED_GRN)) set_led_state(LED_GRN);

		float downtime = 0;
		if (is_btn_down())
		{
			do
			{
				downtime += 0.250;
				_delay_ms(250);

				if ((downtime > 1.0)&&(get_led_state!=LED_NONE)) set_led_state(LED_NONE);
			}
			while (is_btn_down());

			if (downtime > 1.0)
			{
				soft_reset();
			} else {
				int stringnumber = 0;
				if (get_led_state()==LED_GRN) stringnumber=1;
				else if (get_led_state()==LED_BLU) stringnumber=2;
				else if (get_led_state()==LED_WHT) stringnumber=3;

				if (stringnumber)
				{
					// Do appropriate action for given color state
					memset(buf,0,sizeof(buf));
					int bcnt_ret = ee_readback(stringnumber,buf,sizeof(buf));
					kybd_type(buf,bcnt_ret);
				}
			}
		}
	}

}

// Fires about 61 times/sec, but we further scale this down to 6 Hz here
ISR(TIMER0_OVF_vect)
{
	static int8_t postscalar = 30;
	if (flash_leds)
	{
		postscalar--;
		if (postscalar <= 0)
		{
			postscalar = 30;
			set_led_state(get_led_state());
		}
		else if (postscalar<=15)
		{
			TURN_OFF_LEDS();
		}
	}
}
