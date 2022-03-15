/*
 * engine.cpp
 *
 * Created: 18.12.2021
 *  Author: Frank Bjørnø
 *
 * Purpose:	
 *
 *          To demonstrate the use of a timer and state machine to control the behavior 
 *          of a program.
 *
 * Dependencies:
 *
 *          lcd.h:           Engine uses a display for output.
 *          ds1307.h:        Engine gets input from a DS1307
 *          timer.h:         Engine uses a Timer object to control the behavior of the display and other things.
 *          avr/interrupt.h: This program uses interrupts to interface with the user.
 *          stdio.h:         Engine uses functions from stdio.h, specifically sprintf.
 *          util/delay.h:    Engine relies on delays to control the execution of some functions.
 *
 * License:
 * 
 *          Copyright (C) 2021 Frank Bjørnø
 *
 *          1. Permission is hereby granted, free of charge, to any person obtaining a copy 
 *          of this software and associated documentation files (the "Software"), to deal 
 *          in the Software without restriction, including without limitation the rights 
 *          to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
 *          of the Software, and to permit persons to whom the Software is furnished to do 
 *          so, subject to the following conditions:
 *        
 *          2. The above copyright notice and this permission notice shall be included in all 
 *          copies or substantial portions of the Software.
 *
 *          3. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 *          INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 *          PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 *          HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 *          CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 *          OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */ 

/*
 *     F_CPU has to be defined in order to use the delay function in util/delay.h
 *     if you use a 16Mhz crystal as a clock for the mcu, then leave this unchanged, 
 *     if not alter it accordingly
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif

//  define the interrupt pin
#define BUTTON_INT_PIN       0x04;

//  define macros to control external interrupts
#define __enable_ext_ints__  EIMSK |= 0x03
#define __disable_ext_ints__ EIMSK &= 0xFC


//  include libraries
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>

#include "engine.h"

/*
 *     Interrupt Service Routines (ISR) need access to engine and Timer
 */
static Engine* _sinstance = nullptr;		//  Provides access to Engine from ISR0
static Timer   _stimer;						//  ISR updates Timer

/*
 *     When the push button is pressed, this ISR is called
 */
ISR(INT0_vect)
{
	__disable_ext_ints__;
	_sinstance -> button_pressed();
	__enable_ext_ints__;
}

/*
 *     The internal MCU timer/counter calls this ISR in predetermined intervals
 *     specified in the Timer class in Timer.h/cpp
 */
ISR(TIMER0_COMPA_vect)
{
	_stimer++;
}

/*
 *     DisplayState Constructor.
 *
 *     \param *sm     A pointer to the owner of the DisplayState instances. 
 */
DisplayState::DisplayState(StateMachine *sm) : _sm{sm}, _next_state{nullptr}
{
	_def_timeout = 5000;
	_timeout = _def_timeout;
}

/*
 *     Get DisplayState status.
 *
 *     \return     false
 */
bool DisplayState::is_active()
{		
	return false;
}

/*
 *     Update the timeout variables
 *
 *     \param dt    Milliseconds since last update.
 *
 *     Note: The default behavior is to do nothing since the default
 *           state does not time out, but changes when the user interacts
 *           with the push button.
 */
void DisplayState::update(int dt) {/*do nothing*/}

/*
 *     Reset the timeout variable, that is, restore it to its default value.
 */
void DisplayState::reset()
{
	_timeout = _def_timeout;
}

/*
 *     Notify the display state that the push button has been activated.
 */
void DisplayState::button_pressed()
{	
	_sm -> displaystate_changed();
}

/*
 *     Set the default number of milliseconds for a timeout
 *
 *     \param def     The new default initial value for timeout
 */
void DisplayState::set_default_timeout(int def)
{
	_def_timeout = def;
	_timeout = _def_timeout;
}

/*
 *     Set the next_state pointer
 *
 *     \param *state    A pointer to the next state.
 */
void DisplayState::set_next_state(DisplayState *state)
{	
	_next_state = state;
}

/*
 *     Get a pointer to the next state. Call this when the display state 
 *     should change.
 *
 *     \return    A pointer to the next state.
 */
DisplayState* DisplayState::next_state()
{	
	return _next_state;
}

/*
 *     DefaultDisplayState Constructor.
 *
 *     \param *sm     A pointer to the owner of the DisplayState instances. 
 */
DefaultDisplayState::DefaultDisplayState(StateMachine *sm) : DisplayState(sm) {}

/*
 *     ActiveDisplayState Constructor.
 *
 *     \param *sm     A pointer to the owner of the DisplayState instances. 
 */
ActiveDisplayState::ActiveDisplayState(StateMachine *sm) : DisplayState(sm) {}

/*
 *     Get DisplayState status. Overrides the default behavior.
 *
 *     \return     true
 */
bool ActiveDisplayState::is_active()
{		
	return true;
}

/*
 *     Update the timeout variables. Overrides the default behavior by
 *     subtracting from the timeout variable and notify the StateMachine
 *     if time is up.
 *
 *     \param dt    Milliseconds since last update.
 */
void ActiveDisplayState::update(int dt)
{
	_timeout -= dt;
	if (_timeout <= 0) _sm -> displaystate_changed();	
}

/*
 *     Engine constructor. Initializes member variables and configures
 *     interrupts.
 */
Engine::Engine() : _dstate{this}, _astate{this}
{			
		//  initialize static variable
	_sinstance = this;
	
		//  initialize display
	_display.init();
	
		//  initialize and configure clock
	_clock.init();	
	_clock.set_mode(DSMODE12);					//  use 12 mode for this. 
	_A = 21;
	_B = 12;
	_C = 29;
	_clock.set_ymd(_A, _B, _C);
	_A = 11;
	_B = 59;
	_C = 00;
	_MI = PM;
	_clock.set_12hms(_A, _B, _C, _MI);
	_clock.transfer_data();
	
		//  initialize and configure display states
	_dstate.set_next_state(&_astate);
	_astate.set_next_state(&_dstate);
	_current_state = &_dstate;
		
		//  enable and configure interrupts
	DDRD  = 0x00;					//  clear data direction register on port d
	PORTD |= BUTTON_INT_PIN;		//  set port bit 2 to enable internal pull up on PD2 (INT0)
	
	EICRA  = 0x0A;					//  rising edge generates an external interrupt request
	SREG  |= 0x80;					//  enable interrupts by setting BIT7 in the Status Register	
}

/*
 *     Notify engine about button press. This should 
 *     only be called from the ISR.
 */
void Engine::button_pressed()
{
	_current_state -> button_pressed();
}

/*
 *     Start the engine. Call this when program is ready to run.
 */
void Engine::run()
{
	__enable_ext_ints__;
	main_loop();
}

/*
 *     Notify the engine that display state should change. This is the
 *     interface between DisplayState objects and Engine.
 */
void Engine::displaystate_changed()
{	
		//  change the state	
	_current_state = _current_state -> next_state();		
	
		//  check if the new state is the active state and 		
	if (_current_state -> is_active())
	{				
		_display.display(ON);					//  if so, activate display.
		_display.clear();
		_display.backlight(ON);
		_current_state -> reset();				//  reset timeout
		_show_colon = true;						//  start with a visible colon
		_countdown = 500;						//  initialize countdown timer.
	}
	else
	{
		_display.backlight(OFF);				//  if not, deactivate display
		_display.display(OFF);
	}
}

/*
 *     The main loop updates and displays the time if the display
 *     is active and controls the timer.
 */
void Engine::main_loop()
{
	int delta;										//  the execution time of the last iteration of main loop.
	while(true)
	{
		delta = _stimer.stop();						//  new delta
		
		
		if (_current_state->is_active())			//  is the display active?
		{
			//  update state
			_current_state->update(delta);			//  if so, update the timeout counter in display state.
			
			//  update countdown
			_countdown -= delta;					//  update the timeout counter for the colon.
			if (_countdown <= 0)					//  if timed out
			{
				_show_colon ^= 1;					//  toggle colon visibility by xor'ing with 1.
				_countdown = 500;					//  reset countdown.
			}
		}
		
		_stimer.start();							//  start the timer
		
		if (_current_state->is_active()) update();	//  only update if display is active
				
		_delay_ms(100);								//  delay 100ms between iterations
	}
}

/*
 *     Updates the information in the display. 
 */
void Engine::update()
{
	_clock.update();													//  update clock
		
		//  get and print date
	_clock.get_ymd(_A, _B, _C);											//  grab data from _clock instance
	sprintf(_buffer, "%02i.%02i.20%02i", _C, _B, _A);					//  prepare display buffer
	_display.pos(FIRST, 3);												//  move cursor into position
	_display.print(_buffer);											//  display date
	
		//  get and print time
	_clock.get_12hms(_A, _B, _C, _MI);
	sprintf(_buffer, "%02i %02i %2s", _A, _B, (_MI ? "PM" : "AM"));
	_display.pos(SECOND, 4);
	_display.print(_buffer);
	
		//  print or hide colon between hour and minute
	_display.pos(SECOND, 6);
	_display.print(_show_colon ? ":" : " ");
}