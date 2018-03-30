#pragma once

#include <vector>
#include <fftw3.h>
#include "Event.h"
#include "Note.h"

class GTM {
	public:
		GTM(unsigned int fft_size, unsigned int samplerate);
		virtual ~GTM();

		bool isPeak(int index, double threshold, bool dbfs);
		bool hasHarmonics(int initial_index);
		unsigned char freqToNote(double input);
		double processSamples(double *input);
		void analyseSpectrum();

		std::vector<Event> event_list;

	//private:
		fftw_complex *fft_in, *fft_out;
		fftw_plan fft_plan;
		double *dft, *dft_log;

		unsigned int fft_size, samplerate;
		unsigned int slope_threshold, sustain_threshold;

		unsigned short int midi[128];
		std::vector<Note> sustained_notes;
};
