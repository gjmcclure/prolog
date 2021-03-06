///////////////////////////////////////////////////////////////////////////////////
//                       Copyright (C) 2007Robert P. Wolf                       //
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

#include "prolog_windows_console.h"

#include "process.h"

static void command_runner (void * parameter) {((PrologWindowsConsole *) parameter) -> run ();}

void PrologWindowsConsole :: print (char * text) {
	DWORD x;
	WriteFile (output, text, (DWORD) strlen (text), & x, NULL);
}

void PrologWindowsConsole :: run (void) {
	DWORD x;
	running = true;
	while (running) {
		ReadFile (input, area, AREA_SIZE_1, & x, NULL);
		area [x] = '\0';
		insert (area);
		Sleep (delay);
	}
	running = true;
}

PrologWindowsConsole :: PrologWindowsConsole (int horizontal) {
	if (horizontal < 1) horizontal = 1;
	delay = 1000 / horizontal;
	configure ();
}

PrologWindowsConsole :: PrologWindowsConsole (int horizontal, int seconds) {
	if (seconds < 1) seconds = 1;
	if (horizontal < 1) horizontal = 1;
	seconds *= 1000;
	delay = seconds / horizontal;
	configure ();
}

void PrologWindowsConsole :: configure (void) {
	output = NULL;
	input = NULL;
	running = false;
}

void PrologWindowsConsole :: open (void) {
	if (output != NULL) return;
	AllocConsole ();
	SetConsoleTitle ("HERCs PROLOG CONSOLE");
	output = GetStdHandle (STD_OUTPUT_HANDLE);
	input = GetStdHandle (STD_INPUT_HANDLE);
	SetConsoleTextAttribute (output, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	_beginthread (command_runner, 0, this);
}

void PrologWindowsConsole :: close (void) {
	if (output == NULL) return;
	stop ();
	CloseHandle (output);
	CloseHandle (input);
	FreeConsole ();
	input = NULL;
	output = NULL;
}

void PrologWindowsConsole :: stop (void) {
	if (running) {
		running = false;
		while (! running) Sleep (20);
		running = false;
	}
}

PrologWindowsConsole :: ~ PrologWindowsConsole (void) {}

void PrologWindowsConsole :: setColors (int foreground, int background) {
	if (output == NULL) return;
	int foreground_red = (foreground >> 16) & 0xff;
	int foreground_green = (foreground >> 8) & 0xff;
	int foreground_blue = foreground & 0xff;
	bool foreground_intensity = foreground_red > 128 || foreground_green > 128 || foreground_blue > 128;
	int background_red = (background >> 16) & 0xff;
	int background_green = (background >> 8) & 0xff;
	int background_blue = background & 0xff;
	bool background_intensity = background_red > 128 || background_green > 128 || background_blue > 128;
	WORD attribute = 0;
	if (foreground_red >= 128) attribute |= FOREGROUND_RED;
	if (foreground_blue >= 128) attribute |= FOREGROUND_BLUE;
	if (foreground_green >= 128) attribute |= FOREGROUND_GREEN;
	if (foreground_intensity) attribute |= FOREGROUND_INTENSITY;
	if (background_red >= 128) attribute |= BACKGROUND_RED;
	if (background_blue >= 128) attribute |= BACKGROUND_BLUE;
	if (background_green >= 128) attribute |= BACKGROUND_GREEN;
	if (background_intensity) attribute |= BACKGROUND_INTENSITY;
	SetConsoleTextAttribute (output, attribute);
}
