#include <math.h>
#include <fftw3.h>
#include "GTM.h"

/* Create plan and allocate memory. */
GTM::GTM(unsigned int fft_size, unsigned int samplerate) {
	this->slope_threshold = 450;   // HARDCODED FOR NOW
	this->sustain_threshold = 5; // HARDCODED FOR NOW
	this->fft_size = fft_size;
	this->samplerate = samplerate;

	dft = new double[fft_size/2];
	dft_log = new double[fft_size/2];
	fft_in = new fftw_complex[fft_size];
	fft_out = new fftw_complex[fft_size];
	fft_plan = fftw_plan_dft_1d(fft_size, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
}

/* Destructor. */
GTM::~GTM() {
	// TODO: free() our arrays
	fftw_destroy_plan(fft_plan);
	fftw_cleanup();
}

/** Determine if there's a peak at the specified DFT index.
 *  If dbfs is true, compare use full-scale decibels. If not,
 *  use raw power values.
 */
bool GTM::isPeak(int index, double threshold, bool dbfs) {
	//int search_window = 1;
	double prev = 0;
	double next = 0;
	double current = dbfs ? dft_log[index] : dft[index];

	prev = dbfs ? dft_log[index-1] : dft[index-1];
	next = dbfs ? dft_log[index+1] : dft[index+1];

	if (prev > current || next > current) return false;

	/*for (int i = -search_window; i < 0; ++i) {
		prev += dbfs ? dft_log[index + i] : dft[index + i];
	} prev /= search_window;

	for (int i = 1; i <= search_window; ++i) {
		next += dbfs ? dft_log[index + i] : dft[index + i];
	} next /= search_window;*/

	return (current-prev > threshold && current-next > threshold);
}

/** Determine if this is the fundamental of a note.
 *  This will assume initial_index is the dft index
 *  of a fundamental and look for the next harmonics.
 *  If the next few harmonics are present, this is
 *  probably a note fundamental and the function will
 *  return true.
 */
bool GTM::hasHarmonics(int initial_index) {
	bool test;
	int missing_harmonics = 0;

	for (int h = 2; h < 6; ++h) {
		/** Ugly hack: because of rounding and quantization errors,
		 *  sometimes we look for a harmonic in a position too high/low.
		 *  To "fix" this, don't just look at the expected position,
		 *  but at a small search window around it.
		 *
		 *  TODO: Figure out if we should look below or above instead
		 *  of looking at both. Test how this behaves in higher octaves.
		 */
		int harmonic_index = initial_index*h;
		int search_window = 2;
		double expected_harm_amp[4] = {-28, -14, -34, -28};

		for (int i = -search_window; i <= search_window; ++i) {
			if (dft_log[harmonic_index + i] > dft_log[harmonic_index]) {
				harmonic_index = harmonic_index + i;
			}
		}

		// Is the harmonic loud enough?
		test = dft_log[harmonic_index] >= ( dft_log[initial_index] + expected_harm_amp[h - 2]);
		if (!test) ++missing_harmonics;

		#ifdef DEBUG
		fprintf(stderr, "Index #%d harmonic #%d: expected %.2f got %.2f (pass? %d)\n",
			initial_index,
			h,
			dft_log[initial_index] + expected_harm_amp[h - 2],
			dft_log[harmonic_index],
			test
			);
		#endif
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
 *  Returns average of all bands.
 */
double GTM::processSamples(double *input) {
	// Copy samples into FFTW input array
	for (unsigned int i = 0; i < fft_size; i++) {
		fft_in[i][0] = input[i];
		fft_in[i][1] = 0;
	}

	// Apply hann window
	for (unsigned int i = 0; i < fft_size; i++) {
		double multiplier = 0.5 * (1 - cos(2*M_PI*i/2047));
		fft_in[i][0] = multiplier * fft_in[i][0];
	}

	// Apply FFT
	fftw_execute(fft_plan);

	// TODO: Figure out if it's worth calculating this
	// for each batch of samples.
	double peak_amplitude = 0; //1500;

	// Convert imaginary part, calculate power and dbfs
	double average_power = 0;
	for (unsigned int i = 0; i < fft_size/2; ++i) {
		dft[i] = sqrt(
			pow(fft_out[i][0], 2) + pow(fft_out[i][1], 2)
			);

		if (dft[i] > peak_amplitude) peak_amplitude = dft[i];

		average_power += dft[i];
	} average_power /= (fft_size / 2);

	for (unsigned int i = 0; i < fft_size/2; ++i) {
		dft_log[i] = 20*log10(abs(dft[i]) / peak_amplitude);
	}

	return average_power;
}

/** Analyse DFT result.
 *  This will look for peak values and try to determine if
 *  they are note fundamentals.
 */
void GTM::analyseSpectrum() {
	// TODO: Instead of hardcoding start value,
	// add constant for minimum search frequency.
	for (unsigned int i = 10; i < fft_size/3 - 1; ++i) {
		/** In this first part of the process, we look for
		 *  new sustained notes.
		 */

		// Previous and next values are smaller than current value
		if (isPeak(i, slope_threshold, false)) {
			double freq = (i) * ((double)samplerate / fft_size);
			//freq = ceil(freq);

			if (hasHarmonics(i)) {
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
		}

		/** Now, we go through the list of previously detected
		 *  notes and check if they're still being sustained.
		 */
		std::vector<Note>::iterator it = sustained_notes.begin();
		while (it != sustained_notes.end()) {
			Note note = *it;

			//if (dft[note.index] < sustain_threshold) {
			// TODO: Change threshold to a constant
			if (dft_log[note.index] < -18.f) {
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
