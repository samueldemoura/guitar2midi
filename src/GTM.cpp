#include <math.h>
#include <fftw3.h>
#include "GTM.h"

/* Create plan and allocate memory. */
GTM::GTM(unsigned int fft_size, unsigned int samplerate) {
	this->fft_size = fft_size;
	this->samplerate = samplerate;

	dft = new double[fft_size/2];
	dft_dbfs = new double[fft_size/2];
	fft_in = new fftw_complex[fft_size];
	fft_out = new fftw_complex[fft_size];
	fft_plan = fftw_plan_dft_1d(fft_size, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
}

/* Destructor. */
GTM::~GTM() {
	delete[] dft;
	delete[] dft_dbfs;
	delete[] fft_in;
	delete[] fft_out;
	fftw_destroy_plan(fft_plan);
	fftw_cleanup();
}

/** Determine if there's a peak at the specified DFT index.
 *  If dbfs is true, compare use full-scale decibels. If not,
 *  use raw power values.
 */
bool GTM::isPeak(int index, double threshold, bool dbfs) {
	int search_window = 8; // HARDCODED FOR NOW
	double prev = 0;
	double next = 0;
	double current = dbfs ? dft_dbfs[index] : dft[index];

	for (int i = -search_window; i < 0; ++i) {
		prev += dbfs ? dft_dbfs[index + i] : dft[index + i];
	} prev /= search_window;

	for (int i = 1; i <= search_window; ++i) {
		next += dbfs ? dft_dbfs[index + i] : dft[index + i];
	} next /= search_window;

	return current-prev > threshold && current-next > threshold;
}

/** Determine if this is the fundamental of a note.
 *  This will assume initial_index is the dft index
 *  of a fundamental and look for the next harmonics.
 *  If the next few harmonics are present, this is
 *  probably a note fundamental and the function will
 *  return true.
 */
bool GTM::hasHarmonics(int initial_index) {
	int missing_harmonics = 0;

	/** First, check for a subharmonic. If there's a strong enough
	 *  subharmonic, chances are this is not a fundamental.
	 */
	if (dft_dbfs[initial_index/2] > dft_dbfs[initial_index] * 0.5f) {
		return false;
	}

	for (int h = 2; h < 7; ++h) {
		/** Ugly hack: because of rounding and quantization errors,
		 *  sometimes we look for a harmonic in a position too high/low.
		 *  To "fix" this, don't just look at the expected position,
		 *  but at a small search window around it.
		 *
		 *  TODO: Figure out if we should look below or above instead
		 *  of looking at both. Test how this behaves in higher octaves.
		 */
		int harmonic_index = initial_index*h;
		int search_window = 5;
		double expected_harm_dropoff[5] = {1, 1, 0.2, 1, 0.2};

		int harmonic_offset = 0;
		for (int i = 0; i <= search_window; ++i) {
			if (dft_dbfs[harmonic_index + i] > dft_dbfs[harmonic_index + harmonic_offset]) {
				harmonic_offset = i;
			}
		} harmonic_index += harmonic_offset;

		// Is the harmonic a peak?
		bool test = isPeak(harmonic_index, sd_dbfs * expected_harm_dropoff[h - 2], true);
		
		if (!test) ++missing_harmonics;

		//#ifdef DEBUG
		fprintf(stderr, "=> i%d harm %d (id %d): exp %.2f got %.2f (pass? %d)\n",
			initial_index,
			h,
			harmonic_index,
			dft_dbfs[initial_index] + expected_harm_dropoff[h - 2],
			dft_dbfs[harmonic_index],
			test
			);
		//#endif
	}

	return (missing_harmonics < 2);
}

/* Convert frequency number into MIDI note. */
unsigned char GTM::freqToNote(double input) {
	int A4 = 440;
	int A4_midi = 57;

	double r = pow(2, (1.f/12.f));
	double cent = pow(2, (1.f/1200.f));

	int r_index = 0;
	int cent_index = 0;
	int side;

	double f = A4;

	if (input >= f) {
		while (input >= f*r) {
			f = f*r;
			r_index++;
		}

		while (input > f*cent) {
			f = f*cent;
			cent_index++;
		}

		if ( (f*cent - input) < (input - f) ) {
			cent_index++;
		}

		if (cent_index > 50) {
			r_index++;
			cent_index = 100 - cent_index;

			side = (cent_index == 0);
		} else {
			side = 1;
		}
	}

	else {
		while (input <= (f/r)) {
			f = f/r;
			r_index--;
		}

		while (input < (f/cent)) {
			f = f/cent;
			cent_index++;
		}

		if ( (input - f/cent) < (f - input) ) {
			cent_index++;
		}

		if (cent_index >= 50) {
			r_index--;
			cent_index = 100 - cent_index;
			side = 1;
		} else {
			side = (cent_index == 0);
		}
	}

	return A4_midi + r_index;
}

/** Apply FFT and do power analysis.
 *  The length of the output array must be at least FFT_SIZE/2.
 */
void GTM::processSamples(double *input) {
	// Copy samples into FFTW input array
	for (unsigned int i = 0; i < fft_size; i++) {
		fft_in[i][0] = input[i];
		fft_in[i][1] = 0;
	}

	// Apply hann window (disabled for now -- too many artifacts)
	/*for (unsigned int i = 0; i < fft_size; i++) {
		double multiplier = 0.5 * (1 - cos(2*M_PI*i/2047));
		fft_in[i][0] = multiplier * fft_in[i][0];
	}*/

	// Apply FFT
	fftw_execute(fft_plan);

	peak_raw = 0;
	peak_dbfs = 0;
	mean_raw = 0;
	mean_dbfs = 0;

	// Convert imaginary part, get peak value, calculate raw mean
	for (unsigned int i = 0; i < fft_size / 2; ++i) {
		dft[i] = sqrt(
			pow(fft_out[i][0], 2) + pow(fft_out[i][1], 2)
			);

		if (dft[i] > peak_raw) peak_raw = dft[i];
		mean_raw += dft[i];
	} mean_raw /= (fft_size / 2);

	// Convert into dbfs, calculate dbfs mean, calculate raw standard deviation
	for (unsigned int i = 0; i < fft_size / 2; ++i) {
		dft_dbfs[i] = 20*log10(abs(dft[i]) / peak_raw);
		mean_dbfs += dft_dbfs[i];

		if (dft_dbfs[i] > peak_dbfs) peak_dbfs = dft_dbfs[i];
		sd_raw += pow(dft[i] - mean_raw, 2);
	} mean_dbfs /= (fft_size / 2);
	sd_raw /= (fft_size / 2) - 1;
	sd_raw = sqrt(sd_raw);

	// Calculate dbfs standard deviation
	for (unsigned int i = 0; i < fft_size / 2; ++i) {
		sd_dbfs += pow(dft_dbfs[i] - mean_dbfs, 2);
	} sd_dbfs /= (fft_size / 2) - 1;
	sd_dbfs = sqrt(sd_dbfs);
}

/** Analyse DFT result.
 *  This will look for peak values and try to determine if
 *  they are note fundamentals.
 */
void GTM::analyseSpectrum() {
	// Tune-able search parameters
	unsigned int search_min = (55) / (samplerate/fft_size);   // ~C2
	unsigned int search_max = (1060) / (samplerate/fft_size); // ~D6
	unsigned int since_last_peak = 999; // ugly
	unsigned int min_peak_distance = 5;
	unsigned int min_power_value = 500;

	for (unsigned int i = search_min; i < search_max; ++i) {
		/** In this first part of the process, we look for
		 *  new sustained notes.
		 */

		// TODO: Don't hardcode this threshold multiplier
		if (isPeak(i, sd_raw * 3.5f, false) && since_last_peak > min_peak_distance) {
			since_last_peak = 0;
			double freq = (i) * ((double)samplerate / fft_size);

			if (hasHarmonics(i) && peak_raw > min_power_value) {
				unsigned char note = freqToNote(freq);

				if (midi[note] == 0) {
					// TODO: Convert power to velocity instead of hardcoding
					midi[note] = 127;

					// Send noteOn event to event list
					Event e = {0, note, static_cast<unsigned char>(midi[note]), 0};
					event_list.push_back(e);

					// Keep track of sustained notes
					Note n = {i, note};
					sustained_notes.push_back(n);
				}
			}
		} else ++since_last_peak;

		/** Now, we go through the list of previously detected
		 *  notes and check if they're still being sustained.
		 */
		std::vector<Note>::iterator it = sustained_notes.begin();
		while (it != sustained_notes.end()) {
			Note note = *it;

			// TODO: Change threshold to a constant (was -16.f)
			if (dft_dbfs[note.index] < (-sd_dbfs*2.f) ) {
				// Update local array
				midi[note.midi_value] = 0;

				// Remove this note from the vector
				it = sustained_notes.erase(it);

				// Send noteOff event to event list
				Event e = {1, note.midi_value, 0, 0};
				event_list.push_back(e);
			} else {
				++it;
			}
		}
	}
}
