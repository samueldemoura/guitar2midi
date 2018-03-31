#include <sndfile.h>
#include "GTM.h"

#define FFT_SIZE 8196
#define MIDI_TIME_MULT 8 // until proper BPM is used, slow down clip with multiplier

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "=> No audio file specified.\n");
		return -1;
	}

	SF_INFO sf_info = (SF_INFO) {0, 0, 0, 0, 0, 0};
	SNDFILE* sf = sf_open(argv[1], SFM_READ, &sf_info);

	if (sf == NULL) {
		fprintf(stderr, "=> Failed to open audio file.\n%s\n", sf_strerror(sf));
		return -1;
	}

	GTM gtm(FFT_SIZE, sf_info.samplerate);

	// MIDI CSV headers
	printf("0, 0, Header, 0, 1, 96\n"
		   "1, 0, Start_track\n"
		   "1, 0, Title_t, \"test\"\n"
		   "1, 0, Time_signature, 4, 2, 36, 8\n"
		   "1, 0, Time_signature, 4, 2, 36, 8\n"
		);

	// Prepare to read audio samples
	double samples[FFT_SIZE];

	int offset = 0;
	while (sf_read_double(sf, &samples[0], FFT_SIZE)) {
		sf_seek(sf, 2048*offset, SEEK_SET); // TODO: test what chunk size is best

		// Process samples and analyse
		gtm.processSamples(&samples[0]);
		gtm.analyseSpectrum();

		// Handle MIDI events
		while (!gtm.event_list.empty()) {
			Event e = gtm.event_list.back();
			gtm.event_list.pop_back();

			if (e.pitch+12 > 127) continue;

			printf("1, %d, %s, 0, %d, %d\n",
				offset * MIDI_TIME_MULT,
				e.type ? "Note_off_c" : "Note_on_c",
				e.pitch + 12,
				e.velocity
				);
		}

		++offset;
	}

	// MIDI CSV end
	printf("1, %d, End_track\n0, 0, End_of_file\n", offset*MIDI_TIME_MULT + 1);

	return 0;
}
