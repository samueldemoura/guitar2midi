if [ -z "$*" ]; then echo "No audio file specified."; exit; fi

filename=$1
echo ${filename%.aif}

make
./bin/guitar2midi $filename | csvmidi > ${filename%.aif}.mid
