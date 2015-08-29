/*
 * 
 * DOLLY SLIDER CONTROL - CREATED BY PITTO
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * HARDWARE SETUP -----------------------------------------------------
 * PINS 0-5
 * connected to a LCD DISPLAY 16x2
 * PINS 6-9
 * connected to the Stepper Motor of the Slider
 * PINS A2, A3, A4, A5
 * connected to the Camera Shutter
 * PIN 13
 * connected to the RIGHT & LEFT ENDSTOP of the Slider
 * PIN 12
 * connected to the ENTER KEY
 * PIN 10
 * connected to the Shutter Lever Endstop
 * PIN A0
 * connected to the DIMMER
 * --------------------------------------------------------------------
 * 
 *  
 */

/*INCLUDE*/
#include <Stepper.h>
#include <LiquidCrystal.h>

/*VERSION*/
#define PRG_VERSION "0.25"
#define PRG_NAME "SLIDER.INO"

/*Program states*/
#define MENU_PROGRAM_MAIN				0
#define MENU_DEBUG_INPUTS				1
#define MENU_DEBUG_SLIDER_STEPPER		2
#define MENU_DEBUG_AUTOFOCUS_PAUSE 		3
#define MENU_SETUP_SLIDER_LENGHT		4
#define MENU_SETUP_SLIDER_RATIO			5
#define MENU_SETUP_PAUSE_RATIO			6
#define MENU_SETUP_SLIDER_DIRECTION		7
#define MENU_SETUP_PHOTO_TOT			8
#define MENU_SETUP_PHOTO_PAUSE			9
#define MENU_SETUP_COUNTDOWN			10
#define MENU_PROGRAM_LOAD				11
#define MENU_PROGRAM_CLEAR				12
#define MENU_DEBUG_MODE					13
#define MENU_CONTINUOUS_MODE			14
#define MENU_CONTROL_RUN_PROGRAM		15

#define TOT_MENU_ITEMS 16

#define WARNING_CHECK_SLIDE 0

#define SHUTTER_MINIMUM_TIME 2.5
#define LCD_HORZ_CHARS 16
#define LCD_HORZ_PIXELS 80

/*LCD CUSTOM CHARS*/
byte customChar[7][8] = {
	{B10000, B10000, B10000, B10000, B10000, B10000, B10000, B10000 },
	{B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000 },
	{B11100, B11100, B11100, B11100, B11100, B11100, B11100, B11100 },
	{B11110, B11110, B11110, B11110, B11110, B11110, B11110, B11110 },
	{B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 },
	{B11111, B10001, B10001, B10001, B10001, B10001, B10001, B11111 },
	{B00000, B00100, B01110, B11111, B11111, B01110, B00100, B00000 }
	};
	
int Countdown = 0;
int menu_item_counter;
int menu_item_selected = MENU_PROGRAM_MAIN;
/* LCD Default text for program state*/
char* lcd_text[]={
				"MAIN            ", /*DONE*/
				"<01.CHK INPUTS >", /*DONE*/
				"<02.SET SLD STP>", /*DONE*/
				"<03.AF PAUSE   >", /*DONE*/
				"<04.SLD LENGHT >", /*DONE*/
				"<05.SLD RATIO  >", /*DONE*/
				"<06.PAUSE RATIO>", /*DONE*/
				"<07.SLD DIR    >", /*DONE*/
				"<08.PHT TOTAL  >", /*DONE*/
				"<09.PHT PAUSE  >", /*DONE*/
				"<10.SET C.DOWN >",
				"<11.PRG LOAD   >",
				"<12.PRG CLEAR  >",
				"<13.DEBUG MODE >",/*DONE*/
				"<14.CONT  MODE >",/*DONE*/
				"*-* EXE PROG *-*" /*DONE*/
				};/*DONE*/
				
char* lcd_slider_direction[] = {
				"<<<<L E F T <<<<",
				"     N o n e    ",
				">>>>R I G H T>>>"};

int Slider_direction = 2; /*by default go to right*/

/* LCD Pins Setup */
LiquidCrystal lcd(5, 4, 3, 2, 1, 0);

/* Buttons & Dimmer Setup*/
const byte SLIDER_ENDSTOP = 13; // right and left endrun 
const byte ENTER_KEY = 12; // Enter Key
const byte SHUTTER = 10; // shutter endrun
const byte DIMMER = A0; //dimmer
int Dimmer_value = 0; // dimmer's value

/*Photo Stepper Variables*/
const int _1cm = 510;
int Shutter_run = 150;
int Autofocus_pause_ms = 100;

/* SLIDER & SHUTTER MOTORS SETUP*/
int stepsPerRevolution = 100;
Stepper Stepper_slide(stepsPerRevolution, 6, 8, 7, 9);
Stepper Stepper_photo(stepsPerRevolution, A2, A4, A3, A5);

/*PROGRAM VARIABLES*/
float			Slide_lenght = 0; //total lenght of the sliding routine
float			Slide_ratio = 1.0; /*ratio of each slide movement*/
float			Lenght_of_each_photo_step = 0; 
float			Total_time = 0 ;
int				Photo_tot = 240;
float			Photo_pause = 1.0;
float			Photo_pause_ratio = 1.0;
unsigned long 	begin_program_time = 0;
int 			shots = 0;
unsigned long 	old_time = 0;
unsigned long 	now_time = 0;
int 			Dbg_slide_step = 0;
byte			Debug_mode = 0;			
byte			Continuous_mode = 0;

void setup() {
	pinMode(SLIDER_ENDSTOP, INPUT);
	pinMode(ENTER_KEY, INPUT);
	pinMode(DIMMER, INPUT);
	pinMode(SHUTTER, OUTPUT);
	
	/*Calc of the Program total time*/
	Total_time = Photo_tot * Photo_pause/60 + Photo_tot * SHUTTER_MINIMUM_TIME / 60;
	
	// set the speed at 120 rpm:
	Stepper_slide.setSpeed(120);
	Stepper_photo.setSpeed(120);
	lcd.createChar(0, customChar[0]);
	lcd.createChar(1, customChar[1]);
	lcd.createChar(2, customChar[2]);
	lcd.createChar(3, customChar[3]);
	lcd.createChar(4, customChar[4]);
	lcd.createChar(5, customChar[5]);
	lcd.createChar(6, customChar[6]);
	lcd.begin(16, 2);
}

void loop() {
	digitalWrite(SHUTTER,0);
	switch (menu_item_selected) {
		case MENU_PROGRAM_MAIN:
			delay(200);
			if (!digitalRead(ENTER_KEY)) {
				print_header(Dimmer_selection(TOT_MENU_ITEMS),0);
				switch (Dimmer_selection(TOT_MENU_ITEMS)) {
					case MENU_DEBUG_MODE:
						if (Debug_mode) {
							lcd.print("ON ");
						}else{
							lcd.print("OFF");
						}
					break;
					case MENU_DEBUG_AUTOFOCUS_PAUSE:
						lcd.print(Autofocus_pause_ms);
						lcd.print(" ms.");
					break;
					case MENU_PROGRAM_MAIN:
						print_program();
					break;
					case MENU_DEBUG_SLIDER_STEPPER:
						lcd.print("manual control");
					break;
					case MENU_SETUP_SLIDER_LENGHT:
						lcd.print (Slide_lenght, 1);
						lcd.print (" cm / ");
						lcd.print (Lenght_of_each_photo_step, 2);
					break;
					case MENU_SETUP_PAUSE_RATIO:
						lcd.print (Photo_pause_ratio, 2);
					break;
					case MENU_SETUP_SLIDER_RATIO:
						lcd.print (Slide_ratio, 2);
					break;
					case MENU_SETUP_SLIDER_DIRECTION:
						lcd.print(lcd_slider_direction[Slider_direction]);
					break;
					case MENU_SETUP_PHOTO_TOT:
						lcd.print (Photo_tot);
						lcd.print (" - ");
						lcd.print (Total_time);
						lcd.print ("'");
					break;
					case MENU_SETUP_PHOTO_PAUSE:
						lcd.print (Photo_pause, 1);
						lcd.print (" - ");
						lcd.print (Total_time);
						lcd.print ("'");
					break;
					case MENU_SETUP_COUNTDOWN:
						lcd.print (Countdown, 1);
					break;
					case MENU_CONTINUOUS_MODE:
						lcd.print (Continuous_mode);
					break;
				}
			} else {
				menu_item_selected = Dimmer_selection(TOT_MENU_ITEMS);
				lcd_refresh_display (1);
			}
		break;
		case MENU_CONTINUOUS_MODE:
			delay(200);
			print_header(MENU_CONTINUOUS_MODE, 1);
			if(!digitalRead(ENTER_KEY)){
				if(Dimmer_selection(2)) {
					lcd.print ("ON ");
				}else{
					lcd.print ("OFF");
				}
			}else{
				Continuous_mode = Dimmer_selection(2);
				if (Continuous_mode) {
					Photo_pause = 0.0;
					Photo_pause_ratio = 1.0;
				}
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_DEBUG_MODE:
			delay(200);
			print_header(MENU_DEBUG_MODE, 1);
			if(!digitalRead(ENTER_KEY)){
				if(Dimmer_selection(2)) {
					lcd.print ("ON ");
				}else{
					lcd.print ("OFF");
				}
			}else{
				Debug_mode = Dimmer_selection(2);
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_DEBUG_INPUTS: /*CHECKS IF ALL INPUT PINS WORK PROPERLY*/
			delay(200);
			print_header(MENU_DEBUG_INPUTS,1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.setCursor(12, 0);
				if (digitalRead(SLIDER_ENDSTOP)) {
					lcd.write(4);
				}else{
					lcd.write(5);
				}
				if (digitalRead(SHUTTER)) {
					lcd.write(4);
				}else{
					lcd.write(5);
				}
				lcd.setCursor(0,1);
				draw_progress_bar(analogRead(DIMMER)/10.24);
			} else {
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_DEBUG_SLIDER_STEPPER: /*MOVE STEPPER MANUALLY*/
			delay(50);
			print_header(MENU_DEBUG_SLIDER_STEPPER, 1);	
			if (!digitalRead(ENTER_KEY)) {
				lcd.print(int((512-analogRead(DIMMER))/102.4*2));
				Dbg_slide_step += int((512-analogRead(DIMMER))/102.4*2);
				if (abs(int((512-analogRead(DIMMER))/102.4))) {
					lcd.print (" * ");/*MOVE_STEPPER MANUALLY*/
					Stepper_slide.step(-int((512-analogRead(DIMMER))/102.4));
					lcd.print (Dbg_slide_step);
				}else {
					lcd.print ("   ");
					lcd.print (Dbg_slide_step);
				}
			} else {
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_DEBUG_AUTOFOCUS_PAUSE:
			delay(200);
			print_header(MENU_DEBUG_AUTOFOCUS_PAUSE, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.print(int(analogRead(DIMMER)/10.24*3));
			} else {
				Autofocus_pause_ms = int(analogRead(DIMMER)/10.24*3);
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}		
		break;
		case MENU_SETUP_COUNTDOWN:
			delay(200);
			print_header(MENU_SETUP_COUNTDOWN, 1);			
			if (!digitalRead(ENTER_KEY)) {
				lcd.print(analogRead(DIMMER)/10.24, 2);
			} else {
				Countdown = analogRead(DIMMER)/10.24;
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_SETUP_SLIDER_DIRECTION:
			delay(200);
			print_header(MENU_SETUP_SLIDER_DIRECTION, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.print(lcd_slider_direction[Dimmer_selection(3)]);
			} else {
				Slider_direction = Dimmer_selection(3);
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_SETUP_SLIDER_LENGHT:
			delay(200);
			print_header(MENU_SETUP_SLIDER_LENGHT, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.print ((analogRead(DIMMER))/ 1024.0 * 70.0, 2);
				lcd.print (" cm");
			} else {
				Slide_lenght = (analogRead(DIMMER))/1024.0 *70.0;
				Lenght_of_each_photo_step = Slide_lenght/Photo_tot;
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_SETUP_PAUSE_RATIO:
			delay(200);
			print_header(MENU_SETUP_PAUSE_RATIO, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.print (analogRead(DIMMER)/1024.0 + 0.5, 3);
			} else {
				Photo_pause_ratio = analogRead(DIMMER)/1024.0 + 0.5;
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_SETUP_SLIDER_RATIO:
			delay(200);
			print_header(MENU_SETUP_SLIDER_RATIO, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.print (analogRead(DIMMER)/1024.0 + 0.5, 3);
			} else {
				Slide_ratio = analogRead(DIMMER)/1024.0 + 0.5;
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_SETUP_PHOTO_TOT:
			delay(200);
			print_header(MENU_SETUP_PHOTO_TOT, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.setCursor(0, 1);
				lcd.print (int((analogRead(DIMMER))/ 1024.0 * 600.0));
			} else {
				Photo_tot = int((analogRead(DIMMER))/ 1024.0 * 600.0);
				Lenght_of_each_photo_step = Slide_lenght/Photo_tot;
				Total_time = Photo_tot * Photo_pause/60 + Photo_tot * SHUTTER_MINIMUM_TIME / 60;
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_SETUP_PHOTO_PAUSE:
			delay(200);
			print_header(MENU_SETUP_PHOTO_PAUSE, 1);
			if (!digitalRead(ENTER_KEY)) {
				lcd.print ((analogRead(DIMMER))/ 1024.0 * 20, 1);
				lcd.print (" sec.");
			} else {
				Photo_pause = (analogRead(DIMMER))/ 1024.0 * 20;
				Total_time = Photo_tot * Photo_pause/60 + Photo_tot * SHUTTER_MINIMUM_TIME / 60;
				menu_item_selected = MENU_PROGRAM_MAIN;
				lcd_refresh_display (0);
			}
		break;
		case MENU_CONTROL_RUN_PROGRAM:
			
			begin_program_time = millis();
			old_time = millis();
			if (Countdown > 0) {
				now_time = millis();
				Countdown -= (begin_program_time - now_time)/1000.0;
				lcd.clear();
				lcd.print("COUNTDOWN ");
				lcd.print(Countdown);
				lcd.print(" s.");
			}else{
				lcd.clear();
				lcd.print ("...starting");
				while (shots<Photo_tot) {
					/*BACK TO MAIN IF ENTER KEY IS PRESSED*/
					if (digitalRead(ENTER_KEY)) {
						menu_item_selected = MENU_PROGRAM_MAIN;
						lcd_refresh_display (0);
						break;
					}
					now_time = millis();
					if (now_time - old_time > Photo_pause * 1000.0) {
						/*CHECK THE ENDSTOP OF THE SLIDER*/
						if(digitalRead(SLIDER_ENDSTOP)) {
							switch (Slider_direction) {
								case 0:
									while ((digitalRead(SLIDER_ENDSTOP))) {
										Stepper_slide.step(10);
									}
								break;
								case 2:
									while ((digitalRead(SLIDER_ENDSTOP))) {
										Stepper_slide.step(-10);
									}
								break;
							}
							menu_item_selected = MENU_PROGRAM_MAIN;
							lcd_refresh_display (0);
							break;
						}
						/*MOVE THE SLIDER*/
						switch (Slider_direction) {
							case 0:
								Stepper_slide.step(-int(Lenght_of_each_photo_step*_1cm));
							break;
							case 2:
								Stepper_slide.step(int(Lenght_of_each_photo_step*_1cm));
							break;
						}
						Lenght_of_each_photo_step *= Slide_ratio;
						/*SHOT THE PHOTO
						 * if Debug mode is enabled bypass this statement
						 * and also if Continuous_mode is enabled*/
						if (!Debug_mode && !Continuous_mode) {
							shot_photo();
						}
						Photo_pause *= Photo_pause_ratio;
						shots++;
						old_time = millis();
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print(shots);
						lcd.print("/");
						lcd.print(Photo_tot);
						lcd.print("-");
						/*remaining time*/
						lcd.print (long(((Photo_tot * (now_time - begin_program_time) / shots)-now_time)/60000.0));
						lcd.print("'");
						draw_progress_bar(int(shots * 100.0 / Photo_tot));
					}
				}
				shots = 0;
				menu_item_selected = MENU_PROGRAM_MAIN;
			}
		break;
	
		default:
			menu_item_selected = MENU_PROGRAM_MAIN;
		break;
	}
}

/*functions*/
int Dimmer_selection(int selections_available) {
	return int ((selections_available * analogRead(DIMMER)) / 1024);
}

void shot_photo() {
	digitalWrite(SHUTTER,1);
	//delay(Autofocus_pause_ms);
	delay(120);
	digitalWrite(SHUTTER,0);
	delay(70);
}

void lcd_refresh_display (int dir) {
	int row = 0;
	int column = 0;
	if (dir) {
		for (row = 0; row < 2; row++) {
			for (column = 0; column < 16; column++) {
				lcd.setCursor(column, row);
				lcd.print(char(126));
				delay(10);
			}
		}
	}else{
		for (row = 1; row >= 0 ; row--) {
			for (column = 15; column >= 0; column--) {
				lcd.setCursor(column, row);
				lcd.print(char(127));
				delay(10);
			}
		}
	}
}

void draw_progress_bar(byte perc) {
	int c = 0;
	int a = 0;
	//float prog_bar;
	perc = int(perc * 0.8);
	lcd.setCursor(0,1);
	a = int(perc / 5);
	for (c = 0 ; c < a; c++) {
		lcd.print(char(255));
	}
	lcd.write(int(perc % 5));
}

void print_header (byte header, byte menu_level){
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(lcd_text[header]);
	if (menu_level) {
		lcd.setCursor(0,0);
		switch (int(millis()/500) % 2) {
			case 0:
				lcd.write(6);
				lcd.setCursor(15,0);
				lcd.write(6);
			break;
			case 1:
				lcd.print(" ");
				lcd.setCursor(15,0);
				lcd.print(" ");
			break;
		}
	}
	lcd.setCursor(0, 1);			
}

void print_program() {
	switch (millis()/1000 % 9) {
		case 0:
			lcd.print("PHT TOT    ");
			lcd.print(Photo_tot);
		break;
		case 1:
			lcd.print("PHT PAUSE  ");
			lcd.print(Photo_pause, 1);
		break;
		case 2:
			lcd.print("WAIT RATIO ");
			lcd.print(Photo_pause_ratio, 2);
		break;
		case 3:
			lcd.print("SLD LENGHT ");
			lcd.print(Slide_lenght, 1);
		break;
		case 4:
			lcd.print("SLD RATIO  ");
			lcd.print(Slide_ratio, 2);
		break;
		case 5:
			lcd.print("SLD DIR    ");
				switch(Slider_direction){
					case 0:
						lcd.print("LEFT");
					break;
					case 1:
						lcd.print("NONE");
					break;
					case 2:
						lcd.print("RIGHT");
					break;
				}
		break;
		case 6:
			lcd.print("DEBUG MODE ");
			lcd.print(Debug_mode);
		break;
		case 7:
			lcd.print("CONT MODE  ");
			if (Continuous_mode) {
				lcd.print("ON");
			}else{
				lcd.print("OFF");
			}
		break;
		case 8:
			lcd.print("COUNTDOWN  ");
			lcd.print(Countdown);
		break;
	}
}
