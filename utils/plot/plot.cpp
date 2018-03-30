#include <sndfile.h>
#include "GTM.h"

#define FFT_SIZE 8196

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: plot [AUDIO FILE] [DBFS READING? y/n].\n");
		return -1;
	}

	// Open sound file
	SF_INFO sf_info = (SF_INFO) {0, 0, 0, 0, 0, 0};
	SNDFILE* sf = sf_open(argv[1], SFM_READ, &sf_info);

	if (sf == NULL) {
		fprintf(stderr, "Failed to open sound file.\n%s\n", sf_strerror(sf));
		return -1;
	}

	// Initialize main functionality
	GTM gtm(FFT_SIZE, sf_info.samplerate);

	// Seek ahead to avoid initial transient
	sf_seek(sf, 1024, SEEK_SET);

	// Read samples from audio file
	double samples[FFT_SIZE];
	sf_read_double(sf, &samples[0], FFT_SIZE);

	// Do FFT
	fprintf(stderr, "Average power: %f", gtm.processSamples(&samples[0]));

	// Print values
	for (unsigned int i = 0; i < FFT_SIZE/2; ++i) {
		printf("%f\n", argv[2][0] == 'y' ? gtm.dft_log[i] : gtm.dft[i]);
	}

	return 0;
}
