///////////////////////////////////////////////////////////////////////////////////
//                       Copyright (C) 2005 Robert P. Wolf                       //
//                                                                               //
// Permission is hereby granted, free of charge, to any person obtaining a copy  //
// of this software and associated documentation files (the "Software"), to deal //
// in the Software without restriction, including without limitation the rights  //
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell     //
// copies of the Software, and to permit persons to whom the Software is         //
// furnished to do so, subject to the following conditions:                      //
//                                                                               //
// The above copyright notice and this permission notice shall be included in    //
// all copies or substantial portions of the Software.                           //
//                                                                               //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR    //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   //
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, //
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     //
// THE SOFTWARE.                                                                 //
///////////////////////////////////////////////////////////////////////////////////

#include "prolog.h"

#ifdef WINDOWS_OPERATING_SYSTEM
#include <windows.h>
#endif

#ifdef LINUX_OPERATING_SYSTEM
#include <pthread.h>
#include <unistd.h>
#endif

#ifndef _MIDI_COMMAND_PROMPT_
#define _MIDI_COMMAND_PROMPT_

class HERCs_PROLOG_SDK_EXPORT MidiCommandPrompt;

class MidiCommandPrompt {
private:
	midi_stream * line;
	#ifdef WINDOWS_OPERATING_SYSTEM
	HANDLE output;
	HANDLE input;
	HANDLE thread;
	#endif
	#ifdef LINUX_OPERATING_SYSTEM
	pthread_t thread;
	#endif
	AREA area;
	int delay;
	void configure (midi_stream * line);
public:
	void print (char * text);
	void setColors (int foreground, int background);
	void open (void);
	void close (void);
	void run (void);
	MidiCommandPrompt (midi_stream * line, int horizontal);
	MidiCommandPrompt (midi_stream * line, int horizontal, int seconds);
	~ MidiCommandPrompt (void);
};

#endif
