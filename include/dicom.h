#ifndef DICOM_H
#define DICOM_H

#include <stdio.h>

#define UNABLE_TO_READ_TAG		-1
#define UNABLE_TO_READ_VR		-2
#define UNABLE_TO_SKIP_VALUE_BYTES	-3
#define UNABLE_TO_READ_UNKNOWN_VL_DATA	-4
#define UNEXPECTED_TAG			-5
#define UNABLE_TO_READ_ITEM_LENGTH	-6
#define UNABLE_TO_SKIP_ITEM_LENGTH	-7
#define INVALID_HEADER			-8
#define NOT_A_DICOM_FILE		-9
#define UNABLE_TO_READ_VL		-10

typedef struct Tag {
	unsigned short Group;
	unsigned short Element;
} *TagPtr;

typedef unsigned int ValueLength;

enum { 
	AE, 
	AS, 
	AT, 
	CS, 
	DA, 
	DS, 
	DT, 
	FL, 
	FD, 
	IS, 
	LO, 
	LT, 
	OB, 
	OF, 
	OW, 
	PN, 
	SH, 
	SL, 
	SQ, 
	SS, 
	ST, 
	TM, 
	UI, 
	UL, 
	UN, 
	US, 
	UT, 
	INVALID_VR
};

typedef int ValueRepresentation;

enum {
	STREAM_OK,
	STREAM_DONE,
	STREAM_EOF
};

typedef int StreamStatus;

typedef struct DCMStream {
	StreamStatus (* read) (FILE *, unsigned long, void *, unsigned long *);
} *DCMStreamPtr;

typedef struct DCMVisitor {
  	void (* visitElement) (struct Tag, ValueRepresentation, ValueLength, void *);
	unsigned long (* visitElementUnknownVL) (struct Tag, ValueRepresentation, struct DCMStream, FILE*);
	void (* enterSequence) (struct Tag);
	void (* exitSequence) ();
	void (* nextSequenceItem) ();
} *DCMVisitorPtr;

unsigned long readDicomFile(FILE *, struct DCMVisitor);

unsigned long skipUnknownVLData(struct Tag, ValueRepresentation, struct DCMStream, FILE*);

#endif
