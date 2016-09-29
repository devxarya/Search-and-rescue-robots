/*
? * Team Id : 2018
? * Author List :Devvrat arya  and Deva harasha bolisetty
? * Filename: <Filename>
? * Theme: Search and rescue
? * Functions:
? * Global Variables: These are listed below
? */

/************************************************************************/
/* no time left for documaentaion but function can make any body understand what they do */
/************************************************************************/


#define __OPTIMIZE__ -O0
#define F_CPU 14745600
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"

#include <math.h> //included to support power function

// Interrupt variables
volatile unsigned long int ShaftCountLeft = 0; //to keep track of left position encoder
volatile unsigned long int ShaftCountRight = 0; //to keep track of right position encoder
volatile unsigned int Degrees; //to accept angle in degrees for turning

//Global variables
unsigned int value;
unsigned char ADC_Value;
unsigned char sharp;
unsigned int b1,b2,b3,b4;
unsigned int l=0,r=0;
unsigned int arena[9][9];
unsigned int orientation=0;   // 1 for horizontal, 0 for vertical
unsigned int num_white_block=0;

unsigned int S[3];

unsigned char Left_white_line = 0;
unsigned char Center_white_line = 0;
unsigned char Right_white_line = 0;
unsigned char flag = 0;

unsigned int d=49*10;  // distance of a line=54cm
unsigned int mid= 500/2;
 
unsigned int perp=90;
unsigned int four=45;

unsigned int total;

unsigned int black;
unsigned int x_counter = 5; // 5
unsigned int y_counter = 0; // 0
unsigned int  counter=0;
unsigned int right_turns=0; //0    // <0 for x> <1 for y> 
unsigned int left_turns=0;
signed int direction=0;

unsigned int next_y=0,next_x=0;

unsigned stop_rescue=0; // Seaarch bot avoid collision=0; rescue bot avoid collision
unsigned int dest_x,dest_y,color;
unsigned Iam_at=0 ;        //  0 start , 1 node , 2 mid_1

// Zigbee connection starts here

unsigned int rescue_x,rescue_y,rescue_or;
unsigned int got_first_cordinate=0;
unsigned int previous,prev_rescue_x,prev_rescue_y;
int r_next_x,r_next_y,r_node_x,r_node_y;
unsigned int trigger=0,wait=0;
 unsigned int n=0;

unsigned num_plots=0;

// Navigaiton variable
unsigned int plot[9][9];
unsigned int path_log[11][11]={
	      { 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
		  { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
		  { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9}, 
	      { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
		  { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},  
		  { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
	      { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
	      { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
          { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
	      { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9},
		  { 9, 9, 9, 9, 9, 9, 9, 9 ,9, 9, 9}
           };

// color sensor, rgb and zigbee variable
volatile unsigned long int pulse = 0; //to keep the track of the number of pulses generated by the color sensor
volatile unsigned long int red;       // variable to store the pulse count when read_red function is called
volatile unsigned long int blue;      // variable to store the pulse count when read_blue function is called
volatile unsigned long int green;     // variable to store the pulse count when read_green function is called
volatile unsigned long int no_ff;     // variable to store the pulse count when no_filter function is called

unsigned char data; //to store received data from UDR1

unsigned int value;
unsigned char ADC_Value;
unsigned char sharp;

unsigned int color;
unsigned int block_on_bot2;
unsigned int block_on_bot1;

unsigned int connection=0;

void motion_set (unsigned char Direct)
{
	unsigned char PortARestore = 0;

	Direct &= 0x0F; 			// removing upper nibble as it is not needed
	PortARestore = PORTA; 			// reading the PORTA's original status
	PortARestore &= 0xF0; 			// setting lower direction nibble to 0
	PortARestore |= Direct; 	// adding lower nibble for direction command and restoring the PORTA status
	PORTA = PortARestore; 			// setting the command to the port
}

/************************************************************************/
/* These function are here to get basic functionality                   */
/************************************************************************/

void motor_init (void)
{
	DDRA = DDRA | 0x0F;
	PORTA = PORTA & 0xF0; 
	
	DDRL = DDRL | 0x18;   //Setting PL3 and PL4 pins as output for PWM generation
	PORTL = PORTL | 0x18; //PL3 and PL4 pins are for velocity control using PWM.

}



void timer5_init()
{
	TCCR5B = 0x00;	//Stop
	TCNT5H = 0xFF;	//Counter higher 8-bit value to which OCR5xH value is compared with
	TCNT5L = 0x01;	//Counter lower 8-bit value to which OCR5xH value is compared with
	OCR5AH = 0x00;	//Output compare register high value for Left Motor
	OCR5AL = 0xFF;	//Output compare register low value for Left Motor
	OCR5BH = 0x00;	//Output compare register high value for Right Motor
	OCR5BL = 0xFF;	//Output compare register low value for Right Motor
	OCR5CH = 0x00;	//Output compare register high value for Motor C1
	OCR5CL = 0xFF;	//Output compare register low value for Motor C1
	TCCR5A = 0xA9;	/*{COM5A1=1, COM5A0=0; COM5B1=1, COM5B0=0; COM5C1=1 COM5C0=0}
 					  For Overriding normal port functionality to OCRnA outputs.
				  	  {WGM51=0, WGM50=1} Along With WGM52 in TCCR5B for Selecting FAST PWM 8-bit Mode*/
	
	TCCR5B = 0x0B;	//WGM12=1; CS12=0, CS11=1, CS10=1 (Prescaler=64)
}

void velocity (unsigned char left_motor, unsigned char right_motor)
{
	OCR5AL = (unsigned char)left_motor;
	OCR5BL = (unsigned char)right_motor;
}

void forward (void) //both wheels forward
{
	motion_set(0x06);
}

void back (void) //both wheels backward
{
	motion_set(0x09);
}

void left (void) //Left wheel backward, Right wheel forward
{
	motion_set(0x05);
}

void right (void) //Left wheel forward, Right wheel backward
{
	motion_set(0x0A);
}


void stop (void) //hard stop
{
	motion_set(0x00);
}

void left_encoder_pin_config (void)
{
	DDRE  = DDRE & 0xEF;  //Set the direction of the PORTE 4 pin as input
	PORTE = PORTE | 0x10; //Enable internal pull-up for PORTE 4 pin
}

//Function to configure INT5 (PORTE 5) pin as input for the right position encoder
void right_encoder_pin_config (void)
{
	DDRE  = DDRE & 0xDF;  //Set the direction of the PORTE 4 pin as input
	PORTE = PORTE | 0x20; //Enable internal pull-up for PORTE 4 pin
}



void left_position_encoder_interrupt_init (void) //Interrupt 4 enable
{
	cli(); //Clears the global interrupt
	EICRB = EICRB | 0x02; // INT4 is set to trigger with falling edge
	EIMSK = EIMSK | 0x10; // Enable Interrupt INT4 for left position encoder
	sei();   // Enables the global interrupt
}

void right_position_encoder_interrupt_init (void) //Interrupt 5 enable
{
	cli(); //Clears the global interrupt
	EICRB = EICRB | 0x08; // INT5 is set to trigger with falling edge
	EIMSK = EIMSK | 0x20; // Enable Interrupt INT5 for right position encoder
	sei();   // Enables the global interrupt
}

//ISR for right position encoder
ISR(INT5_vect)
{
	ShaftCountRight++;  //increment right shaft position count
}


//ISR for left position encoder
ISR(INT4_vect)
{
	ShaftCountLeft++;  //increment left shaft position count
}

ISR(INT0_vect)
{
	pulse++; //increment on receiving pulse from the color sensor
}


//Function used for turning robot by specified degrees
void angle_rotate(unsigned int Degrees)
{
	float ReqdShaftCount = 0;
	unsigned long int ReqdShaftCountInt = 0;

	ReqdShaftCount = (float) Degrees/4.090; // division by resolution to get shaft count 4.62
	ReqdShaftCountInt = (unsigned int) ReqdShaftCount;
	ShaftCountRight = 0;
	ShaftCountLeft = 0;


	while (1)
	{
		if((ShaftCountRight >= ReqdShaftCountInt) | (ShaftCountLeft >= ReqdShaftCountInt))
		break;
	}
	stop(); //Stop robot
}


//Function used for moving robot forward by specified distance
void linear_distance_mm(unsigned int DistanceInMM)
	{
		float ReqdShaftCount = 0;
		unsigned long int ReqdShaftCountInt = 0;

		ReqdShaftCount = DistanceInMM / 5.338; // division by resolution to get shaft count
		ReqdShaftCountInt = (unsigned long int) ReqdShaftCount;
		ShaftCountRight = 0;
		ShaftCountLeft = 0;
		while(1)
		{
			if(ShaftCountLeft > ReqdShaftCountInt)
			{  break;
			}
		}
		stop(); //Stop robot
	}

void forward_mm(unsigned int DistanceInMM)
	{
		forward();
		linear_distance_mm(DistanceInMM);
	}

void back_mm(unsigned int DistanceInMM)
	{
		back();
		linear_distance_mm(DistanceInMM);
	}

void left_degrees(unsigned int Degrees)
	{
		// 88 pulses for 360 degrees rotation 4.090 degrees per count
		left(); //Turn left
		angle_rotate(Degrees);
	}



void right_degrees(unsigned int Degrees)
	{
		// 88 pulses for 360 degrees rotation 4.090 degrees per count
		right(); //Turn right
		angle_rotate(Degrees);
	}



void lcd_port_config (void)
	{
	 DDRC = DDRC | 0xF7; //all the LCD pin's direction set as output
	 PORTC = PORTC & 0x80; // all the LCD pins are set to logic 0 except PORTC 7
	}

// Servo pin Configuration
void servo1_pin_config (void)
{
	DDRB  = DDRB | 0x20;  //making PORTB 5 pin output
	PORTB = PORTB | 0x20; //setting PORTB 5 pin to logic 1
}

//Configure PORTB 6 pin for servo motor 2 operation
void servo2_pin_config (void)
{
	DDRB  = DDRB | 0x40;  //making PORTB 6 pin output
	PORTB = PORTB | 0x40; //setting PORTB 6 pin to logic 1
}

//Configure PORTB 7 pin for servo motor 3 operation
void servo3_pin_config (void)
{
	DDRB  = DDRB | 0x80;  //making PORTB 7 pin output
	PORTB = PORTB | 0x80; //setting PORTB 7 pin to logic 1
}

void timer1_init(void)
{
 TCCR1B = 0x00; //stop
 TCNT1H = 0xFC; //Counter high value to which OCR1xH value is to be compared with
 TCNT1L = 0x01;	//Counter low value to which OCR1xH value is to be compared with
 OCR1AH = 0x03;	//Output compare Register high value for servo 1
 OCR1AL = 0xFF;	//Output Compare Register low Value For servo 1
 OCR1BH = 0x03;	//Output compare Register high value for servo 2
 OCR1BL = 0xFF;	//Output Compare Register low Value For servo 2
 OCR1CH = 0x03;	//Output compare Register high value for servo 3
 OCR1CL = 0xFF;	//Output Compare Register low Value For servo 3
 ICR1H  = 0x03;	
 ICR1L  = 0xFF;
 TCCR1A = 0xAB; /*{COM1A1=1, COM1A0=0; COM1B1=1, COM1B0=0; COM1C1=1 COM1C0=0}
 					For Overriding normal port functionality to OCRnA outputs.
				  {WGM11=1, WGM10=1} Along With WGM12 in TCCR1B for Selecting FAST PWM Mode*/
 TCCR1C = 0x00;
 TCCR1B = 0x0C; //WGM12=1; CS12=1, CS11=0, CS10=0 (Prescaler=256)
}

void buzzer_pin_config (void)
	{
	 DDRC = DDRC | 0x08;		//Setting PORTC 3 as output
	 PORTC = PORTC & 0xF7;		//Setting PORTC 3 logic low to turnoff buzzer
	}

void buzzer_on (void)
	{
		unsigned char port_restore = 0;
		port_restore = PINC;
		port_restore = port_restore | 0x08;
		PORTC = port_restore;
	}	 

void buzzer_off (void)
	{
		unsigned char port_restore = 0;
		port_restore = PINC;
		port_restore = port_restore & 0xF7;
		PORTC = port_restore;
	}

void ADC_init (void)
	{
		 DDRF = 0x00;
		 PORTF =  0x00;
		 ADCSRA = 0x00;
	     ADCSRB = 0x00;		//MUX5 = 0
		 ADMUX = 0x20;		//Vref=5V external --- ADLAR=1 --- MUX4:0 = 0000
	     ACSR = 0x80;
		 ADCSRA = 0x86;		//ADEN=1 --- ADIE=1 --- ADPS2:0 = 1 1 0
	}	

unsigned char ADC_Conversion(unsigned char Ch)
	{
		unsigned char a;
		if(Ch>7)
		{
			ADCSRB = 0x08;
		}
		Ch = Ch & 0x07;
		ADMUX= 0x20| Ch;
		ADCSRA = ADCSRA | 0x40;		//Set start conversion bit
		while((ADCSRA&0x10)==0);	//Wait for ADC conversion to complete
		a=ADCH;
		ADCSRA = ADCSRA|0x10;       //clear ADIF (ADC Interrupt Flag) by writing 1 to it
		ADCSRB = 0x00;
		return a;
	}




void Sharp (void)
	{
	DDRK = 0x00; //set PORTK direction as input
	PORTK = 0x00; //set PORTK pins floating
	}

void buzzer_init (void)
	{
		DDRC = DDRC | 0x08;		    //Setting PORTC 3 as output
		PORTC = PORTC & 0xF7;		//Setting PORTC 3 logic low to turnoff buzzer	
	}

void print_sensor(char row, char coloumn,unsigned char channel)
	{
		
		ADC_Value = ADC_Conversion(channel);
		lcd_print(row, coloumn, ADC_Value, 3);
	}


unsigned int Sharp_GP2D12_estimation(unsigned char adc_reading)
{
	float distance;
	unsigned int distanceInt;
	distance = (int)(10.00*(2799.6*(1.00/(pow(adc_reading,1.1546)))));
	distanceInt = (int)distance;

	if(distanceInt>800)
			{
				distanceInt=800;
			}
	return distanceInt;
}

//Color sensor and rgb LED
void color_sensor_pin_config(void)
{
	DDRD  = DDRD | 0xFE; //set PD0 as input for color sensor output
	PORTD = PORTD | 0x01;//Enable internal pull-up for PORTD 0 pin
}

void rgb_off(void)
{
	PORTH= PORTH &~ 0x60;
	PORTG= PORTG &~ 0x02;
	PORTL= PORTL &~ 0x80;
}

void rgb_config (void)
{
	DDRH = DDRH | 0x60; //Setting PortH 5,6 as output
	DDRG = DDRG | 0x02;  // Setting PortG 1 as output
	DDRL = DDRL | 0x80;
	rgb_off();	
}

void red_on (void)  //rph6 gph5 bpg1
{
	rgb_off();
	PORTH = PORTH | 0x20;
}

void green_on (void)
{
	rgb_off();
	PORTH = PORTH | 0x40;
}

void blue_on (void)
{
	rgb_off();
	PORTL = PORTL | 0x80;
}

//Function To Initialize UART0
// desired baud rate:9600
// actual baud rate:9600 (error 0.0%)
// char size: 8 bit
// parity: Disabled
void uart0_init(void)
{
	UCSR0B = 0x00; //disable while setting baud rate
	UCSR0A = 0x00;
	UCSR0C = 0x06;
	UBRR0L = 0x5F; //set baud rate lo
	UBRR0H = 0x00; //set baud rate hi
	UCSR0B = 0x98;
}

void color_sensor_pin_interrupt_init(void) //Interrupt 0 enable
{
	cli(); //Clears the global interrupt
	EICRA = EICRA | 0x02; // INT0 is set to trigger with falling edge
	EIMSK = EIMSK | 0x01; // Enable Interrupt INT0 for color sensor
	sei(); // Enables the global interrupt
}

//Filter Selection
void filter_red(void)    //Used to select red filter
{
	//Filter Select - red filter
	PORTD = PORTD & 0xBF; //set S2 low
	PORTD = PORTD & 0x7F; //set S3 low
}

void filter_green(void)	//Used to select green filter
{
	//Filter Select - green filter
	PORTD = PORTD | 0x40; //set S2 High
	PORTD = PORTD | 0x80; //set S3 High
}

void filter_blue(void)	//Used to select blue filter
{
	//Filter Select - blue filter
	PORTD = PORTD & 0xBF; //set S2 low
	PORTD = PORTD | 0x80; //set S3 High
}
//Color Sensing Scaling
void color_sensor_scaling()		//This function is used to select the scaled down version of the original frequency of the output generated by the color sensor, generally 20% scaling is preferable, though you can change the values as per your application by referring datasheet
{
	//Output Scaling 20% from datasheet
	//PORTD = PORTD & 0xEF;n
	PORTD = PORTD | 0x10; //set S0 high
	//PORTD = PORTD & 0xDF; //set S1 low
	PORTD = PORTD | 0x20; //set S1 high
}

void rgb_test (void)
{
	while(1)
	{
		
	red_on();
	_delay_ms(1000);
	green_on();
	_delay_ms(1000);
	blue_on();
	_delay_ms(1000);
	}	
}
/************************************************************************/
/* Ports are initialized here, after it function are for specific purpose*/                                                         
/************************************************************************/
void port_init()
		{  
			motor_init(); //robot motion pins config
			buzzer_init();	
			left_encoder_pin_config(); //left encoder pin config
			right_encoder_pin_config(); //right encoder pin config
			left_position_encoder_interrupt_init();
			right_position_encoder_interrupt_init();
			
			ADC_init();
			lcd_port_config();
			timer5_init();
			
			color_sensor_pin_config();//color sensor pin configuration
			color_sensor_pin_interrupt_init();
			rgb_config();
			uart0_init(); //Initailize UART0 for serial communiaction
			
			servo1_pin_config(); //Configure PORTB 5 pin for servo motor 1 operation
			servo2_pin_config(); //Configure PORTB 6 pin for servo motor 2 operation
			servo3_pin_config(); //Configure PORTB 7 pin for servo motor 3 operation
			timer1_init(); //For servo PWM
		}



void red_read(void) // function to select red filter and display the count generated by the sensor on LCD. The count will be more if the color is red. The count will be very less if its blue or green.
{
	//Red
	filter_red(); //select red filter
	pulse=0; //reset the count to 0
	_delay_ms(100); //capture the pulses for 100 ms or 0.1 second
	red = pulse;  //store the count in variable called red
	
// 	  	lcd_cursor(1,1);  //set the cursor on row 1, column 1
// 	  	lcd_string("Red Pulses"); // Display "Red Pulses" on LCD
// 	  	lcd_print(2,1,red,5);  //Print the count on second row
// 	  	_delay_ms(1000);	// Display for 1000ms or 1 second
// 	  	lcd_wr_command(0x01); //Clear the LCD
}

void green_read(void) // function to select green filter and display the count generated by the sensor on LCD. The count will be more if the color is green. The count will be very less if its blue or red.
{
	//Green
	filter_green(); //select green filter
	pulse=0; //reset the count to 0
	_delay_ms(100); //capture the pulses for 100 ms or 0.1 second
	green = pulse;  //store the count in variable called green
	
// 	 	lcd_cursor(1,1);  //set the cursor on row 1, column 1
// 	  	lcd_string("Green Pulses"); // Display "Green Pulses" on LCD
// 	  	lcd_print(2,1,green,5);  //Print the count on second row
// 	  	_delay_ms(1000);	// Display for 1000ms or 1 second
// 	  	lcd_wr_command(0x01); //Clear the LCD
}

void blue_read(void) // function to select blue filter and display the count generated by the sensor on LCD. The count will be more if the color is blue. The count will be very less if its red or green.
{
	//Blue
	filter_blue(); //select blue filter
	pulse=0; //reset the count to 0
	_delay_ms(100); //capture the pulses for 100 ms or 0.1 second
	blue = pulse;  //store the count in variable called blue
	
// 	 	lcd_cursor(1,1);  //set the cursor on row 1, column 1
// 	 	lcd_string("Blue Pulses"); // Display "Blue Pulses" on LCD
// 	 	lcd_print(2,1,blue,5);  //Print the count on second row
// 	 	_delay_ms(1000);	// Display for 1000ms or 1 second
// 	 	lcd_wr_command(0x01); //Clear the LCD
}

unsigned int color_read (void)
{ int i=0;
	red_read(); //display the pulse count when red filter is selected
	//	_delay_ms(1000);
	green_read(); //display the pulse count when green filter is selected
	 //  _delay_ms(1000);
	blue_read(); //display the pulse count when blue filter is selected
	 //  _delay_ms(1000);
	
// 	if(green+red<1500 )
// 	{
// 		return 0;  //color is black
// 	}
	
	if((green>red) && (green>blue))
	{
		return 2;  //color is green
	}
	
	
    if ((red>green)&&(red>blue))
	{
		return 1;  // color is red    // rph6 gph5 bpg1
	}
}

// 2sec buzzer for block detection
void block_buzzer (void)
{
	buzzer_on();                // Making buzzer sound for two seconds
	_delay_ms(1000);
	buzzer_off();
}

void block_buzzer2 (void)
{
	buzzer_on();                // Making buzzer sound for two seconds
	_delay_ms(2000);
	buzzer_off();
}

// 10 sec buzzer for ending 
void end_buzzer (void)
{
	buzzer_on();                // Making buzzer sound for two seconds
	_delay_ms(10000);
	buzzer_off() ;
}


// Align with black line present in right <alternate>
void align_right_1(void)
		{   
			velocity (190, 190);
			 right();
			 line_scan();
			
				while (S[2]==1)
         				{    
							 line_scan();
						}
	
			velocity (255, 255);
			stop();
		}

//Align with black line present in left <alternate>
void align_left_1(void)
{		
		     velocity (150,190);
		   left();
		   line_scan();
		   
		  while (S[2]==1)
					{
					  line_scan();
                     }
	
  velocity (255, 255);
  stop();
 
}



// 90 degree right turn at intersection  <alternate>
void right_90_1(void)
{
	right_degrees(40);
	
	align_right_1();
	
}

//90 degree lest turn at intersection <alternate>
void  left_90_1 (void)
{   left_degrees(40);
	
	align_left_1();	
}
  
  

// if block is in range it return 1 otherwise zero
int scan_block (void)
{  unsigned int yes=0;
	for(int i=0;i<=5;i++)
	{	sharp = ADC_Conversion(11);						//Stores the Analog value of front sharp connected to ADC channel 11 into variable "sharp"
		value = Sharp_GP2D12_estimation(sharp);				//Stores Distance calculated in a variable "value".
		lcd_print(2,1,value,3);	
	}
		
	if(value<180 && value >50)   //Check for block in a plot by distance
	{
		yes=1;                     // if yes=1 , block exist
 	}
	
	return yes;
}

void IDR (void)
{  
	find_plot_pos();
	//block_buzzer();
    forward_mm(mid-90);
	connection=0;
	back_mm(10);
	left_degrees(25);
	color=color_read();
	patient_led();
	send_data();
	right_degrees(25);
    back_mm(mid-100);
	rgb_off(); 
}

void right_block_scan (void)
{
	right_degrees(86);
	r=scan_block(); 
	if (r==1)
	{ 
		IDR();
	}
	else
	{
		no_block();
	}
	left_90_1();
	num_plots++;
	r=0;
}

// A scan for block present in left
void left_block_scan(void)
{
	left_degrees(88);
	l=scan_block();
	if (l==1)
	{
		IDR();
	}
	else
	{
	no_block();
	}
	right_90_1();
		num_plots++;
	l=0;
}



void no_block (void)
{
	blue_on();
	block_buzzer2();
	rgb_off();
}

// Scan for block using sharp in front of bot
//This function make sure that a plot is scanned only once
void scan_plot (void)
{  
		if (x_counter%2==0)
		{
		      //orientation =2 <==  bot
			  if (orientation ==3) 
					{	if ( (y_counter<9) && (y_counter>1) ) //somewhere in between
						{
						  if(plot[x_counter][y_counter-1]==0)
							  {
								  left_block_scan();
								  plot[x_counter][y_counter-1]=1;
							  }
		  
						  if (plot[x_counter][y_counter+1]==0)
							  {
								right_block_scan();	  
							   	plot[x_counter][y_counter+1]=1;

							  }
	   
						}	   
	    
		 
						 if  (y_counter==9)  //top most line
						{
							  if (plot[x_counter][y_counter-1]==0)
								  {
									  left_block_scan();
									  plot[x_counter][y_counter-1]=1;
								  }
						}
	  
	  
	  					if  (y_counter==1)     //bottom line
						{
							if (plot[x_counter][y_counter+1]==0)
							{  
								right_block_scan();
								plot[x_counter][y_counter+1]=1;
							}
						}
	 
					}	    
			 // if orientation=1 => bot
	          if(orientation==1)
					 {  
					   if ( (y_counter<9) && (y_counter>1) ) //somewhere in between
					   {
						   if(plot[x_counter][y_counter-1]==0)
						   {
							   right_block_scan();
							   plot[x_counter][y_counter-1]=1;
						   }
		   
						   if (plot[x_counter][y_counter+1]==0)
						   {
							   left_block_scan();
						       plot[x_counter][y_counter+1]=1;
						   }
		   
					   }
	   
	   
					   if  (y_counter==9)   //top most line
					   {
						   if (plot[x_counter][y_counter-1]==0)
						   {
							  right_block_scan();
							  plot[x_counter][y_counter-1]=1;
						   }
					   }
	   
	   
					   if  (y_counter==1) // bottom line
					   {
						   if (plot[x_counter][y_counter+1]==0)
						   {
							   left_block_scan();
							   plot[x_counter][y_counter+1]=1;
						   }
					   }
		    }	      
	   }	  
	
 if (y_counter%2==0)
	{
		//orientation =1   bot
		if (orientation ==0)
		{	if ( (x_counter<9) && (x_counter>1) ) //somewhere in between
			{
				if(plot[x_counter-1][y_counter]==0)
				{
					left_block_scan();
					plot[x_counter-1][y_counter]=1;
					
				}
				
				if (plot[x_counter+1][y_counter]==0)
				{
					right_block_scan();
				    plot[x_counter+1][y_counter]=1;
				}
				
			}
			  
			
			if  (x_counter==9)  //right most line
			{
				if (plot[x_counter-1][y_counter]==0)
				{
					left_block_scan();
				    plot[x_counter-1][y_counter]=1;
				}
			}
			
			
			if  (x_counter==1)     //left  most line
			{
				if (plot[x_counter+1][y_counter]==0)
				{
					right_block_scan();
				    plot[x_counter+1][y_counter]=1;
				}
			}
			
		}
		// if orientation=1 => bot
		if(orientation==2)
		{
			if ( (x_counter<9) && (x_counter>1) ) //somewhere in between
			{
				if(plot[x_counter-1][y_counter]==0)
				{
					right_block_scan();
					plot[x_counter-1][y_counter]=1;
				}
				
				if (plot[x_counter+1][y_counter]==0)
				{
					left_block_scan();
					plot[x_counter+1][y_counter]=1;
				}
				
			}
			
			
			if  (x_counter==9)  //right most line
			{
				if (plot[x_counter-1][y_counter]==0)
				{
					right_block_scan();
					plot[x_counter-1][y_counter]=1;
				}
			}
			
			
			if  (x_counter==1)     //left  most line
			{
				if (plot[x_counter+1][y_counter]==0)
				{
					left_block_scan();
					plot[x_counter+1][y_counter]=1;
				}
			}
		}
	}
	
	
	
	}
	



/*
? * Function Name: Line_scan
? * Input :from three WL sensor
? * Output : A global arrray is set S[3] is set by this func
? * Logic: It scans black line on WL sensor
? * Example Call: line_scan();
? */
void line_scan(void)
{   unsigned int s[3];

	s[1]=ADC_Conversion(3);  // Value of sensor 1 left
	s[2]=ADC_Conversion(2);
	s[3]=ADC_Conversion(1); //right
	
	
	if (s[1]<20)
{ S[1]=1; } //variable is set to 1 ,if white
	else
{ S[1]=0;  }  //if black
	
	if (s[2]<20)
{ S[2]=1; } //variable is set to 1 ,if white
	else
{ S[2]=0;  }  //if black
	
	
	if (s[3]<20)
{ S[3]=1; } //variable is set to 1 ,if white
	else
{ S[3]=0;  }  //if black
	
}


/*
? * Function Name: Align_right
? * Input :none
? * Output : none
? * Logic: It rotates in right direction till the line is detected by central WL sensor
? * Example Call: align_right(void)
//Purpose: for aligning robot at a node
? */
void align_right(void)
{ 	velocity (150, 150);
	right();
	line_scan();
	
	while (S[2]==1)
	{
		line_scan();
	}
	
	velocity (255, 255);
	stop();
}

/*
? * Function Name: align_left
? * Input :none
? * Output : none
? * Logic: It rotates in left direction till the line is detected by central WL sensor
? * Example Call: align_left(void)
//Purpose: for aligning robot at node
? */
void align_left(void)
{
	velocity (150,150);
	left();
	line_scan();
	
	while (S[2]==1)
	{
		line_scan();
	}
	
	velocity (255, 255);
	stop();
	
}


/*
? * Function Name: right_90
? * Input :none
? * Output : none
? * Logic: It rotates in right direction till the line is detected by central WL sensor
? * Example Call: align_right_1(void)
//Purpose: for aligning robot after scanning a plot in left
? */
// 90 degree right at grid lines
void right_90(void)
{
	right_degrees(40);
	align_right();
	right_turns++;
	
}


/*
? * Function Name: left_90
? * Input :none
? * Output : none
? * Logic: It rotates in left direction till the line is detected by central WL sensor
? * Example Call: align_left_1(void)
? *///90 degree lest turn at intersection <alternate>


//90 degree turn left on grid lines
void  left_90 (void)
{   left_degrees(40);
	//	_delay_ms(500);
	align_left();

	
	left_turns++;
}

/*
? * Function Name: Increase counter
? * Input :none
? * Output : It is responsible for maintaitng all coordinates throughout traversing
? * Logic: Increse counter according to orientaion
? * Example Call: increase_counter(void)
*/
void increase_counter (void)   
 { direction = right_turns-left_turns;
	if ( (direction==0 ) || (direction==4) || (direction == -4))
	{
		y_counter++;
		right_turns=0;
		left_turns=0;
	    orientation=0;  // +y axis
	}
	
	 if ( (direction==1) || (direction== -3) )
	{
		x_counter++;
		orientation=1;  // +x axis
	}
   
    if (direction== -1 || direction == 3)
       {
		   x_counter--;
		   orientation=3; // -x axis
	   }
 
    if ( (direction==-2) || (direction ==2) )
       {
		 y_counter--;
		 orientation=2;   //-y axis
	   }
   lcd_cursor(1,2);
   lcd_string("x");
   lcd_print(1,1, x_counter,1 );
   
   lcd_cursor(1,4);
   lcd_string("y");
   lcd_print(1,3, y_counter, 1);
   
   lcd_cursor(1,5);
   lcd_string("o");
   lcd_print(1,6, orientation,1);
   connection=1;
   send_data();
 //  dont_leave_arena();       // Prevent bot to go outside the plot 
 }


/*
? * Function Name: Update_counter
? * Input :none
? * Output : Update orientation as per turns
? * Logic: Subtract  R and L turns
? * Example Call: increase_orientation(void)
*/
void update_orientation (void)
	{
		direction = right_turns-left_turns;
		if ( (direction==0 ) || (direction==4) || (direction == -4))
		{
			right_turns=0;
			left_turns=0;
			orientation=0;  // +y axis
		}
		
		if ( (direction==1) || (direction== -3) )
		{
			orientation=1;  // +x axis
		}
		
		if (direction== -1 || direction == 3)
		{
			orientation=3; // -x axis
		}
		
		if ( (direction==-2) || (direction ==2) )
		{
			orientation=2;   //-y axis
		}
	
	
}

/*-
? * Function Name: follow_line
? * Input :none
? * Output :  Line following till node/Midpoint detected
? * Logic: Turn left if right sensor on right and inverse
? * Example Call: follow_line(void)
*/
// Line following till node/Midpoint detected
void follow_line(void)   
	{       
		unsigned int stayOnLine=1;
		unsigned int flag1=0;
	
	    while(stayOnLine==1)
	
		{

			Left_white_line = ADC_Conversion(3);	//Getting data of Left WL Sensor
			Center_white_line = ADC_Conversion(2);	//Getting data of Center WL Sensor
			Right_white_line = ADC_Conversion(1);	//Getting data of Right WL Sensor

			flag=0;

		//	print_sensor(1,1,3);	//Prints value of White Line Sensor1
		//	print_sensor(1,5,2);	//Prints Value of White Line Sensor2
      	//	print_sensor(1,9,1);	//Prints Value of White Line Sensor3
		
		
			if(Center_white_line>0x14)
			{
				flag=1;
				forward();
				velocity(250,250);
			}

			if((Left_white_line<0x14) && (flag==0))
			{
				flag=1;
				forward();                            // value<0x14 means sensor on white
				velocity(255,195);
			}

			if((Right_white_line<0x14) && (flag==0))  
			{
				flag=1;
				forward();
				velocity(195,255);
			}

			if((Center_white_line>0x14 && Left_white_line>0x14 )||(Center_white_line>0x14 && Right_white_line>0x14))
			 { // _delay_ms(100);
				 flag1++;
				// lcd_print(1,13,flag1,2);
				
			 }
	
	
			if (flag1==1)
						{
							stayOnLine=0;
							linear_distance_mm(95);
							//_delay_ms(300);   //700
							//stop();
							flag1=0;
						}
	
		}			
		increase_counter();
}

 void clear_matrix(void)   //for making all elements of matrix zero
  { int i,j;
 	for(i=0;i<=9;i++)
 	 {
 		 for (j=0;j<=9;j++)
 		 {
 			 plot[i][j]=0;
 		 }
 	 }
 }
 
/*-
? * Function Name: next_cordin
? * Input :orientation
? * Output :  next node x-y coordinates in global var(next_x,next_y)
? * Logic: Add or subtract x-y as the direction of robot
? * Example Call: next_node(2)
*/
 void next_cordinate (int my_orientation)
{  
	
	if(my_orientation==0)
	{
		next_x=x_counter;
		next_y=y_counter+1;
	}
	
	if(my_orientation==1)
	{
	    next_x=x_counter+1;
		next_y=y_counter;		
	}
	
	if(my_orientation==2)
	{
		next_x=x_counter;
		next_y=y_counter-1;
	}
	
	if(my_orientation==3)
	{
		next_x=x_counter-1;
		next_y=y_counter;
	}	
}


/*-Logic: It Find the the cooridnte of scanned plot
? * this coordintes are sent to rescue robot
? * 
? * 
? * 
? * Example Call: nodeORmid(void)
*/

void find_plot_pos (void)
{
  int plot_orientation;
  if (r==1)
  {
  	plot_orientation=orientation+1;
  }  
   if (l==1)
  {
	  plot_orientation=orientation-1;
  }
  
	if (plot_orientation==4)
	{
	   plot_orientation=0;
	}
	
	if (plot_orientation==-1)
	{
		plot_orientation=3;
	}

  //plot_orientation=correct_orientation_overflow(plot_orientation);

  next_cordinate(plot_orientation);
  dest_x=next_x;
  dest_y=next_y;
}
void clear_path(void)   //for making all elements of matrix zero
{ int i,j;
	for(i=1;i<=9;i++)
	{
		for (j=1;j<=9;j++)
		{
			path_log[i][j]=0;
		}
	}
}

/*-
? * Function Name: find_a_way
? *  It finds path on path log basis
? * Logic: It takes a path and increases path log vale by 1 every travesing
? *
*/
void find_a_way(void)
{ int i =0,p[4],prev=4,way, turns=0, o1, o2;
  
	for (i=0;i<=3;i++)
	{
		next_cordinate(i);
		p[i]=path_log[next_x][next_y];
		lcd_print(2,5+i,p[i],1);
	}
	
	
	for (i=0;i<=3;i++)
	{
		if (p[i]<prev)
		{
			prev=p[i];
			way=i;
		}
	}
	
	if (p[way]==p[orientation])
	{
		
	}
	
	else
	{
		o1=orientation+1;
		o2=orientation-1;
		if (o1==4)
		{
			o1=0; 
		}
		
		if ( o2==-1)
		{
			o2=3;
		}
		
		if (p[o1]>p[o2])
		{
			left_90();
		}
		
		 if (p[o1]<p[o2])
		{
			right_90();
		}
		
	     if (p[o1]==p[o2])
		{
			right_90();
			
		}
	}	
}

void path_log_entry (void)
{
	path_log[x_counter][y_counter]++;
}	


/*-
? * Function Name: nodeORmid
? * Input : x_counter and y_counter
? * Output :  1 for node and 2 for mid point
? * Logic: Obtained by dividing 2
? * Example Call: nodeORmid(void)
*/
void nodeORmid (void)
{
	if( !(x_counter%2) && (y_counter%2) )
      {
		  Iam_at= 2;      // midpoint and orientation x axis
	  }
     if ((x_counter%2) && !(y_counter%2))
     {
		 Iam_at= 2;        // midpoint and orientation y axis
	 }
	 if ((x_counter%2) && (y_counter%2))
	 {
		 Iam_at= 1;        // at node
	 }	    
	 
}

/*
? * Function Name: node_action 
? * Logic: Search robot detects if any block present if not it will take a path
? *In case a block is present it sets 9 in path log;
*(large value of path log means unlikely path to take by search robot)
? *further way is led by find a way
? * Example: node_action();
*/
void node_action (void)
{ int clear=1;
  while(clear)
    {
	   find_a_way();
	   update_orientation();
	   black=scan_block();
	   if(black==1)
		   {
			   black=0;
			   next_cordinate(orientation);
			   path_log[next_x][next_y]=9;
		   }
	   else
			{
			  clear=0;
			}
	}     
}


// Recive data regularly
SIGNAL(SIG_USART0_RECV) 		// ISR for receive complete interrupt
{ 
	data=UDR0;
	
	if (data/100==0)
	{
		rescue_x=data%10;                   //y           // 0-dest_x-dest_y
		rescue_y=(data%100-data%10)/10;     //x
		avoid_collision();
	}
	if (data/100==1)
	{
		rescue_or=data%10;
	}
	
		
}

/*
? * Function Name: Avoid collision
? * Logic: It sets a high value of path log on the coordinates rescue is presnt
? *(large valuee of path log means unlikely path to take by search robot)
? *  till then rescue robot will wait at a place
? * After the search robot find a path it resotre the previous value
*/
void avoid_collision (void)
{
	//restore
  if ( got_first_cordinate)
  {
    path_log[prev_rescue_x][prev_rescue_y] =previous;
  } 
   //entry
  previous=path_log[rescue_x][rescue_y];
  path_log[rescue_x][rescue_y]=7;
 
  prev_rescue_x=rescue_x;
  prev_rescue_y=rescue_y;
  
  got_first_cordinate=1;
}




/*
? * Sent data:Destination of patient,Search bot present coordinates
? * Send_data after muxing in 10 base number system
? * 
? * Example: send_data();
*/

 void send_data(void)
{ 
	if (connection==0)        // Destination of patient
	{   trigger=1;
		lcd_print(1,14,dest_x,1);
		lcd_print(1,15,dest_y,1);
		lcd_print(1,16,color,1);
	   UDR0=10*dest_x+dest_y;
	   UDR0=200+10*color+trigger;
	}
	
	if ( connection==1)     // Search bot present coordinates
	{
	  UDR0=100+10*x_counter+y_counter;
	}
}

//////////////////////////////////////////////////////////////////////////
// The following func are end prodcedure when all blocks are scanned it takes it back 
//////////////////////////////////////////////////////////////////////////

/*
? * Function Name: get_me_this_orientatoin
? * Input :The desired orientation 
? * Output : the bot will attain it
? * Example: get_me_this_orientatoin(2)
*/
void get_me_this_orientatoin (unsigned int wanted_orientation)
{ unsigned int clear=1, o1,o2, o1_log, o2_log;
	while(clear)
	{
		o1=orientation+1;
		o2=orientation-1;
		if (o1==4)
		{
			o1=0;
			
		}
		
		if ( o2==-1)
		{
			o2=3;
			
		}
		
		if (orientation==wanted_orientation)
		{
			clear=0;
		}
		
		else if (o1==wanted_orientation)
		{
			right_90();
		}
		
		else if (o2==wanted_orientation)
		{
			left_90();
		}
		
		else
		{
			next_cordinate(o1);
			o1_log=path_log[next_x][next_y];
			next_cordinate(o2);
			o2_log=path_log[next_x][next_y];
			if(o1_log>o2_log)
	     	{ left_90(); }
			else
			{
				right_90();
			}
			
		}
		
		update_orientation();
	}
}


/*-
? * Function Name: distance_of_safe_plce
? * Input :x_counter and y_counter
? * Output : Absolute Distance between present position and destination  point
? * Logic: Subtract source co-rdinates from destination
? * Example Call:distance_of_safe_plce(5,8);
*/
int distance_of_safe_plce (int x,int y)
{ int b;
	b=abs(5-x)+abs(1-y);
	return b;
}

/*-
? * Function Name: find_a_way_to_safe
? * Input :none
? * Output : Shortest path with least hinderence is found out
? * Excuted while on node
? * Example Call: find_a_way_to_safe (1)
*/
void find_a_way_to_safe (void)
{  unsigned int i=0,j=0,blockings,distance[4],result, prev=100, wanted_orientation;
		for (i=0;i<4;i++)
		{
			next_cordinate(i);

			blockings=path_log[next_x][next_y];
			//lcd_print(2,11+i,blocnkings,1);
			if (blockings==0)
			{
				next_cordinate(i);
				distance[i]= distance_of_safe_plce(next_x,next_y);
			}
			else
			{
				next_cordinate(i);
				distance_of_safe_plce(next_x,next_y);
				distance[i]= distance_of_safe_plce(next_x,next_y)+blockings*4;
			}
			
		}
		lcd_string("x");
		for (i=0;i<4;i++)
		{
			
			if (distance[i]<prev)
			{
				prev=distance[i];
				result=i;
			}
		}
		
		wanted_orientation=result;
		get_me_this_orientatoin(wanted_orientation);
		
		// 	 next_cordinate(result);
		// 	 distance_of_plot(next_x,next_y);

}

/*-
? * 
? * 
? * Ascociated with go safe place
? * This logic detects any block in front of robot while standing at node
? *And places robot in valid direction
*/ 
void node_procedure (void)
{ unsigned int clear=1;
	while(clear)
	{  
		find_a_way_to_safe();
		
		black=scan_block();
		if(black==1)
		{	
			next_cordinate(orientation);
			path_log[next_x][next_y]=8;
		}
		else
		{
			clear=0;
		}
	}
}

/*-
? * Function Name: go_to_safe_place 
? * 
? * Logic: After all plots are scanned function takes it to the starting point to stop any hindernece
? * Example Call: go_to_safe_place ();
*/
void go_to_safe_place (void)
{ int travel=1;
	clear_path();
  while(travel)
	{
	 nodeORmid();
	  if (Iam_at==1)
	  {
		  if ((x_counter==5) &&( y_counter==1))
		  {
			  travel=0;
			  break;
		  }
		  node_procedure();
		  follow_line();
		  
		  }
	  
	  if (Iam_at==2)
	  {
		  path_log[x_counter][y_counter]++;
		  follow_line();
	  }
	
	}  
}

/*-
? * 
? * Input :type of patient
? * Output :  Led glow accordingly
? * 
? *
*/
void patient_led (void)
{
	if (color==1) //red_patient
	{	
	red_on();
	_delay_ms(1000);
	rgb_off();
	}	
	
	if (color==2) // greeen_patient
	{
	 green_on();
	 _delay_ms(1000);
	 rgb_off();	
	}
}

// Search robot
int main (void)
{ 
	cli();
	port_init();
	sei();

    clear_matrix();          // clear matrix of arena

    lcd_set_4bit();
	lcd_init();


follow_line();


while (1)
   {
	   nodeORmid();
	  
	  
	  if (Iam_at==1)     // robot at node 
	  {
		  node_action();
		  follow_line();
	  }
	  
	 if (Iam_at==2)     // robot at mid point
	 {
		 path_log_entry();
	     scan_plot();
		 follow_line();
		
	 }

	 if (num_plots==16)
	 {   go_to_safe_place();  //At Starting point
		 stop();
		 break;
	 }
   }
}