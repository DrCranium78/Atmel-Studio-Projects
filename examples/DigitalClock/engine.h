/*
 * engine.h
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
 *          Copyright (C) 2022 Frank Bjørnø
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

#pragma once


#include "lcd.h"
#include "ds1307.h"
#include "timer.h"



/*
 *     StateMachine interface. Engine inherits this so DisplayState
 *     can communicate with Engine through the interface.
 */
class StateMachine
{
	public: virtual void displaystate_changed() = 0;
};


/*
 *     Engine uses display states to control the behavior of the display.
 *     The base class DisplayState defines default behavior that can be overridden in
 *     derived classes
 */
class DisplayState
{
	public:
		DisplayState(StateMachine *sm);
		virtual bool is_active();					//  active state?
		virtual void update(int dt);
		virtual void button_pressed();
		
		void reset();
		void set_default_timeout(int def);	
		void set_next_state(DisplayState *state);
		DisplayState* next_state();
	protected:
		StateMachine *_sm;
		int           _timeout;						//  time (ms) until the state should change.
		int           _def_timeout;					//  default initialization for _timeout
		DisplayState *_next_state;					//  pointer to next state.
};


/*
 *     DefaultDisplayState controls the behavior of the display
 *     in default mode, that is, when the display is inactive/off.
 *     Default behavior is defined in the base class, so no need 
 *     to override anything here
 */
class DefaultDisplayState : public DisplayState
{
	public:
		DefaultDisplayState(StateMachine *sm);	
};

/*
 *     ActiveDisplayState controls the behavior of the display 
 *     in active mode, that is, after it has been activated by
 *     the user pressing the push button.
 *     ActiveDisplayState overrides the is_active and update functions
 */
class ActiveDisplayState : public DisplayState
{
	public:
		ActiveDisplayState(StateMachine *sm);
		bool is_active();
		void update(int dt);		
};



/*
 *     The Engine class is the 
 */
class Engine : public StateMachine
{
	public:
		Engine();
		
		void button_pressed();						//  call this from Interrupt Service Routine (ISR)
		void run();							//  call this to start the engine/program
		
		virtual void displaystate_changed();				//  an instance of DisplayState calls this when state changes
	private:	
		void main_loop();						
		void update();							//  does updating between iterations in main loop.
		
		LCD    _display;						
		DS1307 _clock;							//  real time clock
		char   _buffer[17];						//  buffer for display output
		uint8_t _A, _B, _C;						//  variables to set and get info from DS1307
		DSAMPM  _MI;
		
		/*
		 *     The clock display looks nicer with a blinking colon between hours and minutes.
		 *     The following variables are used to control this behavior.
		 */
		bool   _show_colon;						//  the colon between hours and minutes
		int    _countdown;						//  counts down the time for colon to remain (in)visible.
		
		/*
		 *     mcu's don't handle new and delete very well, so a solution is to keep states 
		 *     alive at all time and change between them as needed.
		 */
		DefaultDisplayState  _dstate;
		ActiveDisplayState   _astate;
		DisplayState        *_current_state;	
};


