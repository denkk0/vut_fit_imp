/*******************************************************************************
	Author: Denis Kramár <xkrama06@vutbr.cz>
*******************************************************************************/

#include <fitkitlib.h>
#include <keyboard/keyboard.h>
#include <lcd/display.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

char last_ch, cntPrint[16], memPrint[16];
unsigned long cnt, currCnt, mem[9];
int lastMem, pos, stop, i;
char charmap[8] = {0x1F, 0x02, 0x1F, 0x1F, 0x15, 0x1F, 0x02, 0x1F}; 

void start();
void reset();
void showTime(int mem, int firstChar, unsigned long time, char timeString[16], int delay);

/*******************************************************************************
 * Vypis uzivatelske napovedy (funkce se vola pri vykonavani prikazu "help")
*******************************************************************************/
void print_user_help(void) {}

/*******************************************************************************
 * Obsluha klavesnice
*******************************************************************************/
int keyboard_idle() {
	char ch;
	ch = key_decode(read_word_keyboard_4x4());
	
	if (ch != last_ch) {
		last_ch = ch;
		
		if (ch != 0) {
			switch(ch) {
                // pri stlaceni klavesy 'A' spustit stopky
				case 'A':
					start();
					break;
                // pri sltaceni klavesy 'B' ulozit aktualny cas do pamate,
                // ak je maximalny pocet prvkov v pamati vyuzity, tak sa na
                // displej vypise 'MAX MEM!'
				case 'B':
					if (!stop) {
						currCnt = cnt;
                        if (lastMem >= 9) {
                            while (cnt - currCnt < 100) {
                                LCD_clear();
                                LCD_write_string("MAX MEM!"); delay_ms(10);
                            }
                        } else {
							mem[lastMem] = cnt;
							lastMem++;
							while (cnt - currCnt < 100) {
								showTime(0, 42, cnt, cntPrint, 10);
							}
						}
					}
					break;
                // pri sltaceni klavesy 'C' sa zastavi SMCLK
				case 'C':
					TACTL &= ~MC_2;
					stop = 1;
					break;
                // pri sltaceni klavesy 'D' sa vynuluje cas
				case 'D':
					if (stop) {
						reset();
					}
					break;
                // pri stlaceni klaves '1' - '9' sa zobrazi ulozeny cas
				case '1': case '2': case '3': case '4': case '5':
				case '6': case '7': case '8': case '9':
					pos = toascii(ch) - 49;
					showTime(1, 32, mem[pos], memPrint, 1000);
					break;
				default:
					break;
			}
		}
	}
	
	return 0;
}

unsigned char decode_user_cmd(char *cmd_ucase, char *cmd) { return CMD_UNKNOWN; }

void fpga_initialized() {
    LCD_init();
    LCD_send_cmd(LCD_DISPLAY_ON_OFF | LCD_DISPLAY_ON | LCD_CURSOR_OFF, 0);
	showTime(0, 32, cnt, cntPrint, 10);
}

// funkcia na zobrazenie casu v pozadovanom formate
void showTime(int mem, int firstChar, unsigned long time, char timeString[16], int delay) {
	LCD_clear();
	if (mem) {
		LCD_load_char(1, charmap);
		LCD_append_char('\x1');
	} else {
		LCD_append_char((unsigned char)(firstChar));
	}
	sprintf(timeString, "%11lu.%.2lus", time/100, time%100);
	LCD_append_string(timeString); 
	delay_ms(delay);
}

// vynulovanie casu a pamate medzicasov
void reset() {
	cnt = 0;
	for (i = 0; i < 9; i++) {
		mem[i] = 0;
		lastMem = 0;
	}
}

// sputenie stopiek
void start() {
    // zastavenie watchdoga
	WDTCTL = WDTPW + WDTHOLD;
	
    // ak boli stopky zastavene, pred opatovnim spustenim sa vynuluje cas
    // a pamat medzicasov
	if (stop) { 
		reset();
		stop = 0;
	}

    // povolenie prerusenia casovacu
    // nastavenie po kolko tikoch ma nastat prerusenie (9216 = 0x2400, za 1/100s)
    // SMCLK (f_tiku = 7.3728 MHz), nepretrzity rezim, delenie tiku 8 (nova f_tiku = 921.6kHz)
	CCTL0 = CCIE;
	CCR0 = 0x2400;
	TACTL = TASSEL_2 + MC_2 + ID_3;

	while (1) {
        // ak je cas 4294967295ms (max. hodnota unsigned long), tak sa na displeji
        // zobrazi 'MAX TIME!' a zastavi sa clock, 
		if (cnt > 4294967295) {
			LCD_write_string("MAX TIME!");
			delay_ms(10);
			TACTL &= ~MC_2;
			stop = 1;
			keyboard_idle();
        // zobrazovanie aktualneho casu na stopkach
		} else {
			showTime(0, 32, cnt, cntPrint, 10);
			keyboard_idle();
		}
	}
}
  
/*******************************************************************************
 * Hlavni funkce
*******************************************************************************/
int main(void) {
	last_ch = 0, cnt = 0, lastMem = 0, stop = 0, pos = 0, currCnt = 0;

	initialize_hardware();
	keyboard_init();

	while (1) {
		delay_ms(10);
		keyboard_idle();
	} 
}

interrupt (TIMERA0_VECTOR) Timer_A (void) {
    // nastavenie po kolko tikoch ma nastat prerusenie (9216 = 0x2400, za 1/100s)
    // na kazdy interrupt navysit cnt (cas)
	CCR0 += 0x2400;
	cnt++;
}
