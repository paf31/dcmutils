all: dicom dump dcmpng

dicom:
	gcc -c -Wall -o dicom.o src/dicom.c -I include

dump:
	gcc -Wall -o dump src/dump.c dicom.o -I include

dcmpng:
	gcc -Wall -o dcmpng -lm -lpng -lz src/dcmpng.c dicom.o -I include
