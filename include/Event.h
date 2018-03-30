#pragma once

struct Event {
	int type; // 0 is noteOn, 1 is noteOff
	unsigned char pitch; // MIDI note number (0-127)
	unsigned char velocity; // MIDI note velocity (0-127)
	unsigned int noteId; // for internal use
};
