///////////////////////////////////////////////////////////////////////////////////
//                       Copyright (C) 2003 Robert P. Wolf                       //
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

#include <stdio.h>
#include "midi_stream.h"

midi_stream :: midi_stream (void) {
	thru = NULL;
	checksum = 0;
	get_checksum = 0;
	last_message = 0;
	last_get = 0;
	marked_last_get = 0;
	marked_get_checksum = 0;
	channel_extension = 127;
	write_channel_extension = 0;
	set_manufacturers_id (0x7d);
//	set_product_id (0, 0, 0, 0);
//	set_product_version (0, 0, 0, 0);
//	set_manufacturers_id ();
	set_product_id ("HRCs");
	set_product_version ("CORE");
}

int midi_stream :: get (void) {
	last_get = internal_get ();
	get_checksum -= last_get;
	return last_get;
}

int midi_stream :: get (char * text) {
	while ((* text = (char) get ()) > 0) text++;
	* text = '\0';
	return last_get;
}

int midi_stream :: get_f7 (void) {
	while (last_get != 0xf7 && message_waiting ()) get ();
	return 0xf7;
}

int midi_stream :: get_int (void) {
	int ind = get ();
	ind += get () << 7;
	return ind;
}

int midi_stream :: get_negative (void) {
	int ind = get_int ();
	if (ind > 8191) ind -= 16384;
	return ind;
}

int midi_stream :: get_very_negative (void) {
	int ind = get_int ();
	if (ind > 0) ind -= 16384;
	return ind;
}

int midi_stream :: get_short_negative (void) {
	int ind = get ();
	if (ind > 63) ind -= 128;
	return ind;
}

int midi_stream :: get_int128 (void) {
	int lsb = get ();
	int msb = get ();
	if (msb == 127 && lsb == 127) return 128;
	return msb;
}

bool midi_stream :: get_boolean (void) {
	return get () != 0;
}


bool midi_stream :: check_system_exclusive (void) {
	if (manufacturers_id_2 > 127) {
		if (get () != manufacturers_id_1) return false;
	} else {
		if (get () != 0) return false;
		if (get () != manufacturers_id_1) return false;
		if (get () != manufacturers_id_2) return false;
	}
	if (get () != product_id [0]) return false;
	if (get () != product_id [1]) return false;
	if (get () != product_id [2]) return false;
	if (get () != product_id [3]) return false;
	return true;
}

bool midi_stream :: check_checksum (void) {
	mark ();
	int check = get_checksum;
	int sub = get ();
	while (sub < 0x7f) {
		check = get_checksum;
		sub = get ();
	}
	restore ();
	return (check & 0x7f) == 0;
}

void midi_stream :: insert (int value) {
	internal_insert (value);
	checksum -= value;
	if (value > 0x7f) {last_message = value; close_message ();}
	if (thru == NULL) return;
	thru -> insert (value);
}

void midi_stream :: insert_command (int value) {
	internal_insert_command (value);
	if (thru == NULL) return;
	thru -> insert_command (value);
}

void midi_stream :: insert_running_command (int value) {
	internal_insert_running_command (value);
	if (thru == NULL) return;
	thru -> insert_running_command (value);
}

void midi_stream :: insert (int v1, int v2) {
	if (v1 != last_message) {
		last_message = v1;
		insert_command (v1);
	} else insert_running_command (v1);
	insert (v2);
	close_message ();
}

void midi_stream :: insert (int v1, int v2, int v3) {
	if (v1 != last_message) {
		last_message = v1;
		insert_command (v1);
	} else insert_running_command (v1);
	insert (v2);
	insert (v3);
	close_message ();
}

void midi_stream :: insert (char * text) {
	if (text == NULL) return;
	while (* text != '\0') insert (0x7f & * (text++));
}

int midi_stream :: chex (int channel) {
	int extension = channel >> 4;
	channel &= 0xf;
	if (write_channel_extension == extension) return channel;
	write_channel_extension = extension;
	insert (0xf3, extension);
	return channel;
}

void midi_stream :: insert_channel_extension (int extension) {
	if (write_channel_extension == extension) return;
	write_channel_extension = extension;
	insert (0xf3, extension);
}

void midi_stream :: close_message (void) {
	internal_close_message ();
	if (thru == NULL) return;
	thru -> close_message ();
}

void midi_stream :: insert_keyoff (int channel, int key, int velocity) {insert (0x80 + chex (channel), key, velocity);}
void midi_stream :: insert_keyon (int channel, int key, int velocity) {insert (0x90 + chex (channel), key, velocity);}
void midi_stream :: insert_pat (int channel, int key, int value) {insert (0xa0 + chex (channel), key, value);}
void midi_stream :: insert_control (int channel, int control, int value) {insert (0xb0 + chex (channel), control, value);}
void midi_stream :: insert_control (int channel, int control, int msb, int lsb) {
	channel = 0xb0 + chex (channel);
	insert (channel, control + 32, lsb);
	insert (channel, control, msb);
}
void midi_stream :: insert_nrpn (int channel, int msb, int lsb, int msb_data) {
	channel = 0xb0 + chex (channel);
	insert (channel, 99, msb);
	insert (channel, 98, lsb);
	insert (channel, 6, msb_data);
}
void midi_stream :: insert_nrpn (int channel, int msb, int lsb, int msb_data, int lsb_data) {
	channel = 0xb0 + chex (channel);
	insert (channel, 99, msb);
	insert (channel, 98, lsb);
	insert (channel, 38, lsb_data);
	insert (channel, 6, msb_data);
}
void midi_stream :: insert_nrpn_14 (int channel, int msb, int lsb, int data) {
	channel = 0xb0 + chex (channel);
	insert (channel, 99, msb);
	insert (channel, 98, lsb);
	insert (channel, 38, data & 0x7f);
	insert (channel, 6, (data >> 7) & 0x7f);
}
void midi_stream :: INSERT_NRPN (int channel, int msb, int lsb, int msb_data) {
	channel |= 0xb0;
	insert (channel, 99, msb);
	insert (channel, 98, lsb);
	insert (channel, 6, msb_data);
}
void midi_stream :: INSERT_NRPN (int channel, int msb, int lsb, int msb_data, int lsb_data) {
	channel |= 0xb0;
	insert (channel, 99, msb);
	insert (channel, 98, lsb);
	insert (channel, 38, lsb_data);
	insert (channel, 6, msb_data);
}
void midi_stream :: insert_rpn (int channel, int msb, int lsb, int delta) {
	channel = 0xb0 + chex (channel);
	insert (channel, 101, msb);
	insert (channel, 100, lsb);
	if (delta >= 0) insert (channel, 96, delta);
	else insert (channel, 97, - delta);
}
void midi_stream :: insert_programchange (int channel, int program) {insert (0xc0 + chex (channel), program);}
void midi_stream :: insert_cat (int channel, int value) {insert (0xd0 + chex (channel), value);}
void midi_stream :: insert_pitchbend (int channel, int value) {insert (0xe0 + chex (channel), 0, value);}
void midi_stream :: insert_pitchbend (int channel, int msb, int lsb) {insert (0xe0 + chex (channel), lsb, msb);}
void midi_stream :: open_system_exclusive (void) {
	open_generic_system_exclusive ();
	insert_manufacturers_id ();
	insert_product_id ();
}

void midi_stream :: open_generic_system_exclusive (void) {
	insert_command (0xf0);
	last_message = 0xf0;
	checksum = 0;
}

void midi_stream :: insert_checksum (void) {insert (checksum & 0x7f);}
void midi_stream :: insert_data_xor (void) {insert (internal_get_data_xor ());}
void midi_stream :: close_system_exclusive (void) {
	insert_command (0xf7);
	last_message = 0xf7;
	close_message ();
}

void midi_stream :: connect_thru (midi_stream * thru) {this -> thru = thru;}
void midi_stream :: disconnect_thru (void) {this -> thru = NULL;}

int midi_stream :: internal_get (void) {return 0;}
int midi_stream :: internal_get_command (void) {return 0;}
int midi_stream :: get_command (void) {last_get = internal_get_command (); get_checksum = 0; return last_get;}
void midi_stream :: internal_insert (int value) {}
void midi_stream :: internal_insert_command (int value) {internal_insert (value);}
void midi_stream :: internal_insert_running_command (int value) {}
void midi_stream :: internal_mark (void) {}
void midi_stream :: internal_restore (void) {}
int midi_stream :: internal_get_data_xor (void) {return 0;}
void midi_stream :: internal_close_message (void) {}
bool midi_stream :: message_waiting (void) {return false;}
void midi_stream :: mark (void) {
	marked_last_get = last_get;
	marked_get_checksum = get_checksum;
	internal_mark ();
}
void midi_stream :: restore (void) {
	last_get = marked_last_get;
	get_checksum = marked_get_checksum;
	internal_restore ();
}
void midi_stream :: set_channel_extension (int extension) {channel_extension = extension & 0xffff;}
int midi_stream :: get_channel_extension (void) {return channel_extension;}
void midi_stream :: insert_manufacturers_id (void) {
	if (manufacturers_id_2 > 127) {insert (manufacturers_id_1); return;}
	insert (0);
	insert (manufacturers_id_1);
	insert (manufacturers_id_2);
}
void midi_stream :: insert_product_id (void) {
	if (product_id [0] >= 0) insert (product_id [0]);
	if (product_id [1] >= 0) insert (product_id [1]);
	if (product_id [2] >= 0) insert (product_id [2]);
	if (product_id [3] >= 0) insert (product_id [3]);
}
void midi_stream :: insert_product_version (void) {
	insert (product_version [0]);
	insert (product_version [1]);
	insert (product_version [2]);
	insert (product_version [3]);
}
void midi_stream :: set_manufacturers_id (void) {manufacturers_id_1 = 0; manufacturers_id_2 = 0;}
void midi_stream :: set_manufacturers_id (int ind) {manufacturers_id_1 = ind; manufacturers_id_2 = 128;}
void midi_stream :: set_manufacturers_id (int ind, int sub) {manufacturers_id_1 = ind; manufacturers_id_2 = sub;}
void midi_stream :: set_product_id (char * id) {
	product_id [0] = id [0];
	product_id [1] = id [1];
	product_id [2] = id [2];
	product_id [3] = id [3];
}
void midi_stream :: set_product_id (char id1, char id2, char id3, char id4) {
	product_id [0] = id1;
	product_id [1] = id2;
	product_id [2] = id3;
	product_id [3] = id4;
}
void midi_stream :: set_product_version (char * id) {
	product_version [0] = id [0];
	product_version [1] = id [1];
	product_version [2] = id [2];
	product_version [3] = id [3];
}
void midi_stream :: set_product_version (char id1, char id2, char id3, char id4) {
	product_version [0] = id1;
	product_version [1] = id2;
	product_version [2] = id3;
	product_version [3] = id4;
}

buffered_midi_stream :: buffered_midi_stream (int size) {
	data = new char [size];
	this -> size = size;
	from = 0;
	to = 0;
	complete_message = 0;
	marked_position = 0;
	command = 0;
}

buffered_midi_stream :: ~ buffered_midi_stream (void) {
	delete [] data;
}

int buffered_midi_stream :: internal_get (void) {
	int ind = data [from++];
	ind &= 0xff;
	if (from >= size) from = 0;
	return ind;
}

int buffered_midi_stream :: internal_get_command (void) {
	int ind = data [from];
	ind &= 0xff;
	if (ind < 0x80) return command;
	command = internal_get ();
	return command;
}

bool buffered_midi_stream :: message_waiting (void) {return complete_message != from;}
void buffered_midi_stream :: internal_mark (void) {marked_position = from;}
void buffered_midi_stream :: internal_restore (void) {from = marked_position;}

int buffered_midi_stream :: internal_get_data_xor (void) {
	int xorer = 0;
	int ind = complete_message + 1;
	if (ind >= size) ind = 0;
	while (ind != to) {
		xorer ^= data [ind++];
		if (ind >= size) ind = 0;
	}
	return xorer;
}

extern void message (char * head, int ind, int sub);
void buffered_midi_stream :: internal_insert (int value) {
	data [to++] = (char) value;
	if (to >= size) to = 0;
}

void buffered_midi_stream :: internal_close_message (void) {
	complete_message = to;
}

clearable_midi_stream :: clearable_midi_stream (int size)
: buffered_midi_stream (size) {running_command_not_required = true;}

void clearable_midi_stream :: clear (void) {
	from = 0;
	to = 0;
	complete_message = 0;
	marked_position = 0;
	running_command_not_required = false;
}

void clearable_midi_stream :: internal_insert_running_command (int value) {
	if (running_command_not_required) return;
	internal_insert (value);
	running_command_not_required = false;
}

delayed_buffered_midi_stream :: delayed_buffered_midi_stream (int size) : buffered_midi_stream (size) {
	delays = new int [size];
	delays [0] = 0;
}

delayed_buffered_midi_stream :: ~ delayed_buffered_midi_stream (void) {delete [] delays;}

bool delayed_buffered_midi_stream :: message_waiting (void) {
	if (! buffered_midi_stream :: message_waiting ()) return false;
	return delays [from]-- <= 0;
}

void delayed_buffered_midi_stream :: insert_delay (int delay) {delays [to] = delay;}

void delayed_buffered_midi_stream :: internal_insert (int value) {
	buffered_midi_stream :: internal_insert (value);
	delays [to] = 0;
}

void midi_reader :: read (midi_stream * line) {
	int command;
	int channel;
	int data1;
	int data2;
	midi_channel_extension = line -> get_channel_extension ();
	while (line -> message_waiting ()) {
		command = line -> get_command ();
		channel = command & 0x0f;
		command &= 0xf0;
		switch (command) {
		case 0x80:
			midi_keyoff (channel, line -> get ());
			line -> get ();
			break;
		case 0x90:
			data1 = line -> get ();
			data2 = line -> get ();
			if (data2 > 0) midi_keyon (channel, data1, data2);
			else midi_keyoff (channel, data1);
			break;
		case 0xa0:
			data1 = line -> get ();
			midi_pat (channel, data1, line -> get ());
			break;
		case 0xb0:
			data1 = line -> get ();
			midi_control (channel, data1, line -> get ());
			break;
		case 0xc0:
			midi_programchange (channel, line -> get ());
			break;
		case 0xd0:
			midi_cat (channel, line -> get ());
			break;
		case 0xe0:
			data1 = line -> get ();
			midi_pitchbend (channel, data1, line -> get ());
			break;
		default:
			switch (channel) {
			case 1: midi_parameter_request (line -> get ()); break;
			case 3: midi_channel_extension = line -> get (); break;
			case 8: midi_timing_clock (); break;
			case 0xa: midi_start (); break;
			case 0xb: midi_continue (); break;
			case 0xc: midi_stop (); break;
			case 0xe: midi_active_sensing (); active_sensed = active_sensing_delay; break;
			default: midi_system_exclusive (line); break;
			}
			break;
		}
	}
	line -> set_channel_extension (midi_channel_extension);
	if (active_sensed >= 0) {
		if (active_sensed == 0) midi_active_sensed ();
		active_sensed--;
	}
}

void midi_reader :: midi_pat (int channel, int key, int value) {midi_control (channel, 130, value);}
void midi_reader :: midi_cat (int channel, int value) {midi_control (channel, 129, value);}
void midi_reader :: midi_pitchbend (int channel, int v1, int v2) {
	midi_control (channel, 160, v1);
	midi_control (channel, 128, v2);
}
void midi_reader :: midi_keyoff (int channel, int key) {}
void midi_reader :: midi_keyon (int channel, int key, int velocity) {}
void midi_reader :: midi_control (int channel, int controller, int value) {}
void midi_reader :: midi_programchange (int channel, int program) {}
void midi_reader :: midi_system_exclusive (midi_stream * line) {line -> get_f7 ();}
void midi_reader :: midi_parameter_request (int channel) {}
void midi_reader :: midi_timing_clock (void) {}
void midi_reader :: midi_start (void) {}
void midi_reader :: midi_continue (void) {}
void midi_reader :: midi_stop (void) {}
void midi_reader :: midi_active_sensing (void) {}
void midi_reader :: midi_active_sensed (void) {}
bool midi_reader :: is_ready (void) {return true;}
midi_reader :: midi_reader (void) {midi_channel_extension = 0; active_sensed = -1; active_sensing_delay = 48000;}

void midi_nrpn_reader :: midi_control (int channel, int controller, int value) {
	if ((midi_channel_extension != reception_channel_extension && reception_channel_extension != 0x7f) || (midi_channel != channel && midi_channel != 0x10)) return;
	switch (controller) {
	case 99: nrpn_msb = value; break;
	case 98: nrpn_lsb = value; break;
	case 38: data_lsb = value; break;
	case 6: midi_nrpn (nrpn_msb, nrpn_lsb, value, data_lsb); data_lsb = 0; break;
	default: midi_controller (controller, value);
	}
}
void midi_nrpn_reader :: midi_nrpn (int nrpn_msb, int nrpn_lsb, int data_msb, int data_lsb) {}
void midi_nrpn_reader :: midi_controller (int controller, int value) {}
void midi_nrpn_reader :: set_midi_channel (int channel) {
	midi_channel = channel;
	if (midi_channel < 0) midi_channel = 0x7f;
	if (midi_channel > 16) midi_channel = 0x7f;
}
void midi_nrpn_reader :: set_midi_channel_all (void) {midi_channel = 16;}
void midi_nrpn_reader :: set_midi_channel_off (void) {midi_channel = 0x7f;}
void midi_nrpn_reader :: set_midi_channel_extension (int extension) {
	reception_channel_extension = extension;
	if (reception_channel_extension < 0) reception_channel_extension = 0x7f;
	if (reception_channel_extension > 0x7f) reception_channel_extension = 0x7f;
}
void midi_nrpn_reader :: cancel_midi_channel_extension (void) {reception_channel_extension = 0x7f;}
int midi_nrpn_reader :: get_reception_channel (void) {return midi_channel;}
int midi_nrpn_reader :: get_transmission_channel (void) {return midi_channel & 0xf;}
int midi_nrpn_reader :: get_transmission_channel (int extension) {return (extension << 4) + (midi_channel & 0xf);}
int midi_nrpn_reader :: get_channel_extension (void) {return reception_channel_extension;}
midi_nrpn_reader :: midi_nrpn_reader (void) {
	nrpn_msb = 127;
	nrpn_lsb = 127;
	data_lsb = 0;
	set_midi_channel_all ();
	cancel_midi_channel_extension ();
}
