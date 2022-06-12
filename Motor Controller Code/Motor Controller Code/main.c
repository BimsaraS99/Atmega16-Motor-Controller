#define F_CPU 1000000UL			/* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>			/* Include AVR std. library file */
#include <util/delay.h>			/* Include inbuilt defined Delay header file */
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
int pos, i, num;

int resistor_val;
int temp_val;
int overload_val;
int counter;


//-------------------------------------------------------- LCD Display Functions [Start] -----------------------------------------------------------

#define LCD_Data_Dir DDRC		/* Define LCD data port direction */
#define LCD_Command_Dir DDRD		/* Define LCD command port direction register */
#define LCD_Data_Port PORTC		/* Define LCD data port */
#define LCD_Command_Port PORTD		/* Define LCD data port */
#define RS PD2				/* Define Register Select (data/command reg.)pin */
#define RW PD3				/* Define Read/Write signal pin */
#define EN PD6				/* Define Enable signal pin */

#define MOTOR_PIN1 PB0
#define MOTOR_PIN2 PB1
#define MOTOR_SPEED PB3



void LCD_Command(unsigned char cmnd) {

	LCD_Data_Port= cmnd;
	LCD_Command_Port &= ~(1<<RS);	/* RS=0 command reg. */
	LCD_Command_Port &= ~(1<<RW);	/* RW=0 Write operation */
	LCD_Command_Port |= (1<<EN);	/* Enable pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1<<EN);
	_delay_ms(3);
}

void LCD_Char (unsigned char char_data) {	/* LCD data write function */

	LCD_Data_Port= char_data;
	LCD_Command_Port |= (1<<RS);	/* RS=1 Data reg. */
	LCD_Command_Port &= ~(1<<RW);	/* RW=0 write operation */
	LCD_Command_Port |= (1<<EN);	/* Enable Pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1<<EN);
	_delay_ms(1);
}

void LCD_Init (void) {			/* LCD Initialize function */

	LCD_Command_Dir = 0xFF;		/* Make LCD command port direction as o/p */
	LCD_Data_Dir = 0xFF;		/* Make LCD data port direction as o/p */
	_delay_ms(20);			/* LCD Power ON delay always >15ms */
	
	LCD_Command (0x38);		/* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command (0x0C);		/* Display ON Cursor OFF */
	LCD_Command (0x06);		/* Auto Increment cursor */
	LCD_Command (0x01);		/* Clear display */
	LCD_Command (0x80);		/* Cursor at home position */
}

void LCD_String (char *str) {		/* Send string to LCD function */

	int i;
	for(i=0;str[i]!=0;i++)		/* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str) { /* Send string to LCD with xy position */

	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);	/* Command of first row and required position<16 */
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);	/* Command of first row and required position<16 */
	LCD_String(str);		/* Call LCD string function */
}

void LCD_Clear() {
	
	LCD_Command (0x01);		/* clear display */
	LCD_Command (0x80);		/* cursor at home position */
}

void Display_on_LCD(char line1[], char line2[]) {
	
	LCD_Command (0x80);
	LCD_String(line1);	/* write string on 1st line of LCD*/
	//LCD_Command(0xC0);		/* Go to 2nd line*/
	//LCD_String(line2);	/* Write string on 2nd line*/
}


char arr[200];
char text_display[16];

void Scroll_Text1(char text_scroll[], int size) {
	//char text_passed[] = text_scroll;
	char space = ' ';
	void text_arrange(){
		for(int i = 0; i < 16; i++){
			arr[i] = space;
		}
		
		for(int i = 0; i < size; i++) {
			//char value = text_scroll[i];
			//if(value != NULL){
			arr[i+16] = text_scroll[i];
			//}
		}
		arr[size + 16] = space;
	}

	void remove_items(){
		int len = sizeof arr / sizeof arr[0];
		for (i = -1; i < len -1; i++)  {
			arr[i] = arr[i+1];
		}
	}
	
	void process_text(){
		for(int q = 0; q < 16; q++){
			text_display[q] = arr[q];
		}
	}
	

	text_arrange();
	int len2 =  size + 18;
	for(int i = 0; i < len2; i++){
		remove_items();
		process_text();
		Display_on_LCD(text_display, "");
		_delay_ms(50);
	}
}

//--------------------------------------------------------------------------

int check_res, check_temp, check_ove;
void Print_on_LCD(int read_res, int read_temp, int read_overl) {
	
	//changing
	char speed[] = "Spd:";
	char temp[] = "Te:";
	char motor[] = "Mot:";
	char speed2[4];
	char temp2[4];
	char motorR[] = " Running";
	char motorOL[] = " Overload";
	char motorOT[] = " OverTemp";
	char line1_text[20] = "                    ";
	char line2_text[20] = "                    ";
	
	//speed [resistor value] print on lcd
	sprintf(speed2,"%d", read_res);
	if(read_res < 10){speed2[1] = ' '; speed2[2] = ' ';}
	if(read_res < 100){speed2[2] = ' ';}
	
	for(int i = 0; i < 4; i++){
		line1_text[i] = speed[i];
	}
	for(int j = 0; j < 3; j++){
		line1_text[j+4] = speed2[j];
	}
	
	//temp print on lcd
	sprintf(temp2,"%d", read_temp);
	if(read_temp < 10) {temp2[1] = ' '; temp2[2] = ' ';}
	if(read_temp < 100) {temp2[2] = ' ';}
	
	for(int k = 0; k < 3; k++){
		line1_text[9+k] = temp[k];
	}
	for(int l = 0; l < 3; l++){
		line1_text[12+l] = temp2[l];
	}
	
	//motor state print on lcd
	for(int m = 0; m < 4; m++){
		line2_text[m] = motor[m];
	}
	if(read_overl == 1){
		for(int n = 0; n < 9; n++){
			line2_text[4+n] = motorOL[n];
		}
	}
	else if(read_temp >= 100){ // over temp when more than
		for(int n = 0; n < 9; n++){
			line2_text[4+n] = motorOT[n];
		}
	}
	else{
		for(int n = 0; n < 8; n++){
			line2_text[4+n] = motorR[n];
		}
	}
	
	if(read_res < 10){line1_text[5] = '%';}
	else if(read_res < 100){line1_text[6] = '%';}
	else {line1_text[7] = '%';}
	
	if(read_temp < 10){line1_text[13] = 'C';}
	else if(read_temp < 100){line1_text[14] = 'C';}
	else {line1_text[15] = 'C';}
	
	if(read_res != check_res || read_temp != check_temp || read_overl != check_ove){
		LCD_Command (0x80);
		LCD_String(line1_text);
		LCD_Command(0xC0);
		LCD_String(line2_text);
		check_res = read_res; check_temp = read_temp; check_ove = read_overl;
		//this if statement for update the LCD display only if some value has changed.
	}
}

//-------------------------------------------------------- LCD Display Functions [Finish] -----------------------------------------------------------

//-------------------------------------------------------- Motor Controlling Function [Start] ------------------------------------------------------

int motor_current_rotation_direction;
int motor_current_speed;
void DC_motor(int diection, int speed) {
	//speed
	motor_current_rotation_direction = diection;
	motor_current_speed = speed;
	if (diection == 1) {
		PORTB &= ~(1 << MOTOR_PIN1);
		PORTB |= (1 << MOTOR_PIN2);
	}
	else if (diection == 2){
		PORTB |= (1 << MOTOR_PIN1);
		PORTB &= ~(1 << MOTOR_PIN2);
	}
	else {  // break
		PORTB &= ~(1 << MOTOR_PIN1);
		PORTB &= ~(1 << MOTOR_PIN2);
	}
	if(speed >= 255){
		speed = 255;
	}
	OCR0 = speed;
}

void Motor_overload_event(){ // not yet finished function
	overload_val = 1;
	DC_motor(0, 0);
	while(Read_overload()){  //loop until overload end
		Print_on_LCD(0, Read_temprature(), overload_val); // updating temperature even while inside the while loop.
		DC_motor(0, 0);
		_delay_ms(10);
	}
	_delay_ms(3000);  //delay at least 3 seconds
	DC_motor(motor_current_rotation_direction, motor_current_speed);
	overload_val = 0;
	Print_on_LCD(resistor_val, temp_val, overload_val);
}

void Motor_overTemp_event(){ // not yet finished function
	int counting_time = 0;
	DC_motor(0, 0);
	while(Read_temprature() >= 100){  //loop until overload end - delay at least 3 seconds
		Print_on_LCD(0, Read_temprature(), Read_overload()); // updating temperature even while inside the while loop.
		DC_motor(0, 0);
		_delay_ms(10);
		counting_time++;
	}
	_delay_ms(1000);
	DC_motor(motor_current_rotation_direction, motor_current_speed);
	//Print_on_LCD(resistor_val, temp_val, overload_val);
}

ISR(INT2_vect){  //motor external interrupts
	Motor_overload_event();
}

//-------------------------------------------------------- Motor Controlling Functions [Finish] -----------------------------------------------------

//-------------------------------------------------------- Sensor reading Functions [Start] -----------------------------------------------------


int Read_resistor(){
	ADMUX = ((1 << REFS0) | (1 << REFS1)| (1 << ADLAR)); //enable A0 pin to readings and enable internal ref
	ADCSRA |= (1 << ADEN);
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	ADCSRA|=(1<<ADIF);
	return ADCH / 2.55; // converting 0-255 range value to 0-100 value.
}

int Read_temprature(){ // not yet finished function
	ADMUX = ((1 << REFS0) | (1 << REFS1) | (1 << ADLAR) | (1 << MUX0)); //enable A1 pin to readings ENABLE internal ref
	ADCSRA |= (1 << ADEN);
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	ADCSRA|=(1<<ADIF);
	return ADCH; // converting 0-255 range value to 0-100 value.
}

int Read_overload(){ // not yet finished function
	int reading = (PINB & (1 << PINB2));
	if(reading){
		return 0;
	}
	else {
		return 1;
	}
}

//-------------------------------------------------------- Sensor reading Functions [Finish] -----------------------------------------------------

//-------------------------------------------------------- Main Functions [Start] -----------------------------------------------------
void setup() {
	
	// initialize the display
	LCD_Init();
	_delay_ms(100);
	Scroll_Text1("Developed by Bimsara Sandaruwan - EGT19517", 42); // text, number of characters in the text
	_delay_ms(100);
	LCD_Clear();
	Scroll_Text1("System is starting.", 19); // text, number of characters in the text
	_delay_ms(100);
	LCD_Clear();
	DC_motor(1, 2);  // turn on the dc motor with a speed of 0.
	
	//initialize the motor
	DDRB |= (1 << MOTOR_PIN1);
	DDRB |= (1 << MOTOR_PIN2);
	TCCR0 = (1<<WGM00) | (1<<COM01) | (1<<CS00); // phase control mode PWM Turn ON
	DDRB |= (1 << MOTOR_SPEED); //for speed controlling pin - PWM - //PB30
	
	//initialize the external hardware interrupt for overload detection
	DDRB &= ~(1 << PINB2);
	PORTB |= (1 << PINB2);
	GICR = (1 << INT2);
	MCUCSR &= ~(1 << ISC2);
	sei();
}


int main() {  // this is the main function
	setup();
	while(1) {
		
		resistor_val = Read_resistor();
		temp_val = Read_temprature();
		
		if(temp_val >= 100){
			Motor_overTemp_event();
		}
		
		int speed_motor = resistor_val * 2.55;
		DC_motor(1, speed_motor);
		Print_on_LCD(resistor_val, temp_val, overload_val); //updating lcd display only if some data has changed.
	}
}
//-------------------------------------------------------- Main Functions [Finish] -----------------------------------------------------
