# guitar2midi
Convert audio files containing DI guitar input into MIDI files.

## Building:
Run ```make``` in the root directory of this repository after all the dependencies are installed.

## Requires:
- [fftw3](http://fftw.org/)
- [libsndfile](http://www.mega-nerd.com/libsndfile/)
- [midicsv](https://www.fourmilab.ch/webtools/midicsv/)

## Usage:
The binary itself can be called with: ```guitar2midi [AUDIO FILE]```

This will print a CSV file to stdout, which can be piped into csvmidi to be converted into a MIDI file. You can use the supplied bash script to do this: ```./guitar2midi.sh [AUDIO FILE]```

## DISCLAIMER:
This is a very early work in progress for a university project. The pitch detection algorithm is so bad it might as well be useless and the code is full of sins. Proceed with caution.

