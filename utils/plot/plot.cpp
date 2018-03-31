#include <math.h>
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
	sf_seek(sf, 512*16, SEEK_SET);

	// Read samples from audio file
	double samples[FFT_SIZE];
	sf_read_double(sf, &samples[0], FFT_SIZE);

	// Do FFT
	gtm.processSamples(&samples[0]);

	// Print values for plot
	for (unsigned int i = 0; i < FFT_SIZE/2; ++i) {
		printf("%f\n", argv[2][0] == 'y' ? gtm.dft_dbfs[i] : gtm.dft[i]);
	}

	// Look for peaks
	unsigned int search_min = (55) / (44100.f/FFT_SIZE);
	unsigned int search_max = (1060) / (44100.f/FFT_SIZE);
	unsigned int since_last_peak = 999;
	unsigned int min_peak_distance = 5;

	for (unsigned int i = search_min; i < search_max; ++i) {
		if (gtm.isPeak(i, gtm.sd_raw * 3.5f, false) && since_last_peak > min_peak_distance) {
			since_last_peak = 0;

			fprintf(stderr,
				"===> Found peak at index %d (amp: %.2f)\n",
				i, gtm.dft_dbfs[i]
				);

			fprintf(stderr,
				"===> Is fundamental? %d\n\n",
				gtm.hasHarmonics(i)
				);
		} else ++since_last_peak;
	}

	// Print other info
	fprintf(stderr, "\n"
		"Search: %d to %d\n"
		"Mean: %f\n"
		"Standard deviation: %f\n",
		search_min,
		search_max,
		(argv[2][0] == 'y') ? gtm.mean_dbfs : gtm.mean_raw,
		(argv[2][0] == 'y') ? gtm.sd_dbfs : gtm.sd_raw);

	return 0;
}
