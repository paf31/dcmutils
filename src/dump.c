#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dicom.h"

char* vrTable[] = {
	"AE", "AS", "AT", "CS", "DA", "DS", "DT", "FL", "FD", "IS", "LO", "LT", "OB", "OF", "OW",
	"PN", "SH", "SL", "SQ", "SS", "ST", "TM", "UI", "UL", "UN", "US", "UT", "INVALID" 
};

void visitElement(struct Tag tag, ValueRepresentation vr, ValueLength vl, void *bytes) {
	char *chars;

	printf("(%04x, %04x)\tVR=%s\tVL=%u\t", tag.Group, tag.Element, vrTable[vr], vl);

	switch(vr) {
		case AE:
		case AS: 
		case CS: 
		case DA: 
		case DS: 
 		case DT: 
		case FL: 
		case FD: 
		case IS: 
		case LO: 
		case LT: 
		case PN: 
		case SH: 
		case SQ:
		case ST: 
		case TM: 
		case UI: 
		case UT: 
			chars = (char*)malloc(vl + 1);
			memcpy(chars, bytes, vl);
			chars[vl] = 0;
			printf("%s", chars);
			break;
		case SS: 
			printf("%hd", *(signed short*)bytes);
			break;
		case SL: 
			printf("%ld", *(signed long*)bytes);
			break;
		case US:
			printf("%hu", *(unsigned short*)bytes);
			break;
		case UL: 
			printf("%lu", *(unsigned long*)bytes);
			break;
		case AT: 
		case UN: 
			printf("[Not Read]");
			break; 
		case OB:
		case OF: 
		case OW:
			printf("[Binary Data]");
			break;
		case INVALID_VR:
			printf("[Invalid VR]");
			break;
	}

	printf("\n");
}

unsigned long visitElementUnknownVL(struct Tag tag, ValueRepresentation vr, struct DCMStream stream, FILE *file) {
	unsigned long bytesSkipped = skipUnknownVLData(tag, vr, stream, file);

	printf("(%04x, %04x)\tVR=%s\t[Unknown VL Data]\tLength=%lu\n", tag.Group, tag.Element, vrTable[vr], bytesSkipped);

	return bytesSkipped;
}

void enterSequence (struct Tag tag) {
	printf("Sequence start\n");
}

void exitSequence () {
	printf("Sequence end\n");
}

void nextSequenceItem () {
	printf("Sequence item\n");
}

int main(int argc, char* args[]) {
	FILE *file;
	
	if ((file = fopen(args[1], "r")) == NULL){
		printf("Cannot open file %s.\n", args[1]);
	} else {
		struct DCMVisitor visitor;

		visitor.visitElement = &visitElement;
		visitor.visitElementUnknownVL = &visitElementUnknownVL;
		visitor.enterSequence = &enterSequence;
		visitor.exitSequence = &exitSequence;
		visitor.nextSequenceItem = &nextSequenceItem;

		printf("Reading file %s.\n", args[1]);
		
		unsigned long bytesRead = readDicomFile(file, visitor);

		printf("Read %lu bytes\n", bytesRead);

		fclose(file);
	}
	
	return 0;
}
