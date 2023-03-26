

#include "sys/alt_stdio.h"
#include "system.h"
#include <unistd.h>
#include "sys/alt_irq.h"

unsigned char seven_seg_decode_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7C, 0x07, 0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
char	hex_segments[] = { 0, 0, 0, 0 };

volatile int * BOTTONS_ptr = (int *) BOTTONS_BASE;

int var ;

void Update_HEX_display( int buffer )
{
	volatile int * HEX3_HEX0_ptr = (int *) HEX_BASE;
	int shift_buffer, nibble;
	char code;
	int i;

	shift_buffer = buffer;
	for ( i = 0 ; i < 4 ; ++i )
	{
		nibble = shift_buffer & 0x0000000F;		// character is in rightmost nibble
		code = seven_seg_decode_table[nibble];
		hex_segments[i] = code;
		shift_buffer = shift_buffer >> 4;
	}
	*(HEX3_HEX0_ptr) =  *(int *) hex_segments; 		// drive the hex displays
	return;
}


volatile int edge_capture;
volatile int * Timer_POINTER = (int *) TIMER_BASE;


short seconds = 0 ;
short minuts = 0 ;
short heurs   = 0 ;


short TIME_minute ;
short TIME_heurs ;
short TIME_seconds ;

short chrono_minuts ;
short chrono_seconds ;

void SHOW_hex_min_sec();
void SHOW_hex_heur_min();
short separe_num_2_time(short num);


void Increment_chrono ( )
{
		if (chrono_seconds >= 59)
		{
			chrono_seconds = 0 ;
			if (chrono_minuts >= 59 )
			{
				chrono_minuts = 0 ;
			}
			else chrono_minuts ++  ;
		}
		else chrono_seconds ++ ;
}

void Increment_time ()
{
		if (TIME_seconds >= 59)
		{
			TIME_seconds = 0 ;

			if (TIME_minute >= 59 )
			{
				TIME_minute = 0 ;
				if (TIME_heurs >= 23 ) TIME_heurs = 0 ;
				else TIME_heurs ++ ;
			}
			else TIME_minute  ++  ;
		}
		else TIME_seconds ++ ;
}

void handle_TIMER_interrupts (void* context, alt_u32 id) ;

void init_timer(){
	void * edge_capture_ptr = (void*) & edge_capture ;

	*(Timer_POINTER + 1) = 0X7 ;
	alt_irq_register (TIMER_IRQ   , edge_capture_ptr, handle_TIMER_interrupts);

}

int Mode_Watch 		= 	0x1 ;                	// 1 : Watch Mode   / 0 : Chrono Mode
int chrono_STATE  	=  	0x0 ;    			// 1 : etat declanche / 0 : etat pause
int Modifucation TIME 	=  	0x0 ; 		        // 1 : mode change    / 0 : mode sans chang. par l'utilisateur
int Change_Minuts 	= 	0x1 ; 		         // 1 : change min  / 0 : change heurs



void handle_TIMER_interrupts (void* context, alt_u32 id)
{

		if ( chrono_STATE  )
		{
			Increment_chrono() ;
			Increment_time() ;
		}

	( Mode_Watch | Modifucation TIME  ) ? SHOW_hex_heur_min() : SHOW_hex_min_sec() ;


	*(Timer_POINTER ) = 0 ;
}


short separe_num_2_time(short num)
{
	int i ;
	short num_dec, num_unit ;
	for ( i = 0 ; i <= 5 ; i++)
	{
		if( (int) ( (num - (i*10))) / 10  == 0 )
		{
			num_dec = i;
			break;
		}
	}

	num_unit = num - (num_dec * 10) ;
	return ((num_unit & 0xf ) | (( num_dec << 4 )& 0xf0) ) ;
}



void SHOW_hex_heur_min()
{
	int buff_heurs, buff_minuts;
	int Buffer_HEX ;

	buff_heurs    =		separe_num_2_time  (TIME_heurs);
	buff_minuts  =		separe_num_2_time  (TIME_minute);

	Buffer_HEX = (buff_minuts & 0xff ) | ((buff_heurs & 0xff) << 8);
	Update_HEX_display(Buffer_HEX);
}


void SHOW_hex_min_sec()
{
	int buff_minuts, buff_seconds;
	int Buffer_HEX ;

	buff_minuts    =  separe_num_2_time(chrono_minuts);
	buff_seconds   =  separe_num_2_time(chrono_seconds);

	Buffer_HEX = (buff_seconds & 0xff ) | ((buff_minuts & 0xff) << 8);
	Update_HEX_display(Buffer_HEX);
}


volatile int * KEY_POINTER = (int *) BOTTONS_BASE;

void handle_KEY_interrupts(void* context, alt_u32 id)
{
			int press = * (KEY_POINTER + 3 ); // Reading of the  of the perepheral's register

			if ( press & 0X1) 								// KEY 1 ::       RESET      / Exite mode Modif. time
				{
							if ( Modifucation TIME | (!Mode_Watch) )
							{
								TIME_heurs = 0 ; TIME_minute = 0 ; TIME_seconds = 0 ;
								chrono_minuts = 0 ;  chrono_seconds = 0 ;
							}
				}

			else if ( press & 0X4 )					 // KEY 2   ::    STOP CHRONO    /  START CHRONO      /    -- Decrement
			{
						if ( Modifucation TIME )
						{
							if (Change_Minuts)
							{
									if (TIME_minute >=  0  )
											TIME_minute -- ;
									else
											TIME_minute = 59 ;
							}
							else if ( !Change_Minuts )
							{
									if (TIME_heurs >  0 )
										TIME_heurs -- ;
									else
										TIME_heurs = 23 ;
							}
				}
				else if ( !Modifucation TIME )
				chrono_STATE = ! chrono_STATE ;
			}

			else if ( press & 0X2 ) 			// KEY 1 		/   ++ Increment 
			{
						if ( Modifucation TIME )
							{
								if (Change_Minuts)
								{
										if (TIME_minute < 59  )
												TIME_minute ++ ;
										else
												TIME_minute = 0 ;
								}
								else if ( !Change_Minuts )
								{
										if (TIME_heurs < 59  )
											TIME_heurs ++ ;
										else
											TIME_heurs = 59 ;
								}
							}
			}

			else if ( press & 0X8 ) 				// KEY 3 / Watch Mode => Chrono Mode => Modifucation TIME => Watch Mode
			{
						if ( Mode_Watch == 0x1 ) 										// chang. Watch Mode => Chrono Mode
								Mode_Watch =0x0 ;

						else if ( (!Mode_Watch) & (!Modifucation TIME) ) 				// chang. Chrono Mode => mode Modifucation TIME
						{
							Modifucation TIME  = 1 ;
							Change_Minuts = 1 ;
						}
						else if ( (!Mode_Watch) & Modifucation TIME & Change_Minuts )   // chang. mode Modifucation TIME => Watch Mode
						{
							Change_Minuts =  0  ;
						}
						else if ( (!Mode_Watch) & Modifucation TIME  & !Change_Minuts ) // chang. mode Modifucation TIME => Watch Mode
						{
							Modifucation TIME  = 0 ;
							Change_Minuts = 0 ;
							Mode_Watch   = 1  ;
						}
				}

			*(KEY_POINTER + 3) = 0 ; // clear the rised interrupt of KEYs
}

void init_key()
{
	void * edge_capture_ptr = (void*) & edge_capture ;
	*(KEY_POINTER + 2) = 0XF ; 	// Hide the interruption

	alt_irq_register ( BOTTONS_IRQ , edge_capture_ptr,handle_KEY_interrupts);
}

// the main function 

int main()
{
	TIME_heurs = 0 ; TIME_minute = 0 ; TIME_seconds = 0 ;
	chrono_minuts = 0 ;  chrono_seconds = 0 ;
	init_key();
	init_timer();


	return 0 ;

}
