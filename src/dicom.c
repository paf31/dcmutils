#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dicom.h"

int readTag(FILE *file, TagPtr tag) {
	return fread(tag, sizeof(struct Tag), 1, file);
}

ValueRepresentation parseVR(char *str) {
	if (str[0] == 'A' && str[1] == 'E') {
		return AE;
	} else if (str[0] == 'A' && str[1] == 'S') {
		return AS;
	} else if (str[0] == 'A' && str[1] == 'T') {
		return AT;
	} else if (str[0] == 'C' && str[1] == 'S') {
		return CS;
	} else if (str[0] == 'D' && str[1] == 'A') {
		return DA;
	} else if (str[0] == 'D' && str[1] == 'S') {
		return DS;
	} else if (str[0] == 'D' && str[1] == 'T') {
		return DT;
	} else if (str[0] == 'F' && str[1] == 'L') {
		return FL;
	} else if (str[0] == 'F' && str[1] == 'D') {
		return FD;
	} else if (str[0] == 'I' && str[1] == 'S') {
		return IS;
	} else if (str[0] == 'L' && str[1] == 'O') {
		return LO;
	} else if (str[0] == 'L' && str[1] == 'T') {
		return LT;
	} else if (str[0] == 'O' && str[1] == 'B') {
		return OB;
	} else if (str[0] == 'O' && str[1] == 'F') {
		return OF;
	} else if (str[0] == 'O' && str[1] == 'W') {
		return OW;
	} else if (str[0] == 'P' && str[1] == 'N') {
		return PN;
	} else if (str[0] == 'S' && str[1] == 'H') {
		return SH;
	} else if (str[0] == 'S' && str[1] == 'L') {
		return SL;
	} else if (str[0] == 'S' && str[1] == 'Q') {
		return SQ;
	} else if (str[0] == 'S' && str[1] == 'S') {
		return SS;
	} else if (str[0] == 'S' && str[1] == 'T') {
		return ST;
	} else if (str[0] == 'T' && str[1] == 'M') {
		return TM;
	} else if (str[0] == 'U' && str[1] == 'I') {
		return UI;
	} else if (str[0] == 'U' && str[1] == 'L') {
		return UL;
	} else if (str[0] == 'U' && str[1] == 'N') {
		return UN;
	} else if (str[0] == 'U' && str[1] == 'S') {
		return US;
	} else if (str[0] == 'U' && str[1] == 'T') {
		return UT;
	}
	
	return INVALID_VR;
}

int readDicomHeader(FILE *file) {
	char hdr[4];

	return fread(&hdr, 1, 4, file) == 4 && 
		hdr[0] == 'D' && 
		hdr[1] == 'I' && 
		hdr[2] == 'C' && 
		hdr[3] == 'M';
}

ValueRepresentation readVR(FILE *file) {
	char vrStr[2];

	if (fread(&vrStr, 1, 2, file) == 2) {
		return parseVR(vrStr);
	}

	return INVALID_VR;
}

ValueLength readVL(FILE *file, ValueRepresentation vr) {
	ValueLength vl = 0;

	switch (vr) {
		case OB:
		case OW:
		case OF: 
		case SQ:
		case UT:
		case UN:
			/* Skip two reserved bytes */
			if (fseek(file, 2, SEEK_CUR) == 0) {
				if (fread(&vl, 1, 4, file) == 4) {
					return vl;
				}
			}
			
			return UNABLE_TO_READ_VL;
		default:
			/* Read 2-byte VL */
			if (fread(&vl, 1, 2, file) == 2) {
				return vl;
			}

			return UNABLE_TO_READ_VL;
	}
}

StreamStatus readUnknownVLData(FILE *file, unsigned long bytesToRead, void *buffer, unsigned long *bytesRead) {
	StreamStatus status = STREAM_OK;

	unsigned short *data = (unsigned short *) buffer;

	*bytesRead = 0;

	while ((*bytesRead) < bytesToRead) {
		if (fread(data + (*bytesRead), 2, 2, file) < 2) {
			status = STREAM_EOF;
			break;
		}

		(*bytesRead) += 2;

		if (data[(*bytesRead) - 2] == 0xFFFE && data[(*bytesRead) - 1] == 0xE0DD) {
			status = STREAM_DONE;
			break;
		}
	}

	return status;
}

unsigned long skipUnknownVLData(struct Tag tag, ValueRepresentation vr, struct DCMStream stream, FILE *file) {
	unsigned int bufferSize = 4096;

	unsigned short *buffer = (unsigned short *)malloc(bufferSize * sizeof(unsigned short));

	unsigned long totalBytes = 0;
	unsigned long bytesRead;

	while (stream.read(file, bufferSize, buffer, &bytesRead) == STREAM_OK) {
		totalBytes += bytesRead;
	}

	return totalBytes;
}

unsigned long readSequenceItems(FILE *, struct Tag, ValueLength, struct DCMVisitor visitor);

unsigned long readElement(FILE *file, struct DCMVisitor visitor) {
	struct Tag tag;
	unsigned long bytesRead = 0;

	/* Read the next tag */
	if (readTag(file, &tag) < 1) {
		return UNABLE_TO_READ_TAG;
	}

	bytesRead += 4;

	/* Read the value representation */
	ValueRepresentation vr = readVR(file);

	bytesRead += 4;	

	if (vr == INVALID_VR) {
		return UNABLE_TO_READ_VR;
	}
	
	/* Read the value length */
	ValueLength vl = readVL(file, vr);
	
	bytesRead += 4;

	if (vr == SQ) {
		unsigned long sequenceBytes;

		sequenceBytes = readSequenceItems(file, tag, vl, visitor);

		if (sequenceBytes < 0) {
			return sequenceBytes;
		}

		bytesRead += sequenceBytes;
	} else if (vl != 0xFFFFFFFF) {
		void *bytes = malloc(vl);

		if (fread(bytes, 1, vl, file) < vl) {
			return UNABLE_TO_SKIP_VALUE_BYTES;
		}
		
		if (visitor.visitElement) {
			visitor.visitElement(tag, vr, vl, bytes);
	 	}

		free(bytes);

		bytesRead += vl;
	} else {
		/* Unknown VL */
		struct DCMStream stream;
		stream.read = &readUnknownVLData;

		if (visitor.visitElementUnknownVL) {
			bytesRead += visitor.visitElementUnknownVL(tag, vr, stream, file);
	 	} else {
			bytesRead += skipUnknownVLData(tag, vr, stream, file);
		}
	}

	return bytesRead;
}

unsigned long readSequenceItems(FILE *file, struct Tag sequenceTag, ValueLength vl, struct DCMVisitor visitor) {
	struct Tag tag;
	unsigned long bytesRead = 0;

	if (visitor.enterSequence) {
		visitor.enterSequence(sequenceTag);
	}

	while (vl < 0 || bytesRead < vl) {
		/* Read the next tag */
		if (readTag(file, &tag) < 1) {
			return UNABLE_TO_READ_TAG;
		}

		if (visitor.nextSequenceItem) {
			visitor.nextSequenceItem();
		}
	
		bytesRead += 4;
	
		if (tag.Group != 0xFFFE) {
			return UNEXPECTED_TAG;
		}
	
		if (tag.Element == 0xE000) { /* Item */
			ValueLength itemLength;
			unsigned long itemBytesRead = 0;
	
			if (fread(&itemLength, 1, 4, file) < 4) {
				return UNABLE_TO_READ_ITEM_LENGTH;
			}
	
			bytesRead += 4;
	
			while (itemBytesRead < itemLength) {
				unsigned long elementBytes = readElement(file, visitor);

				if (elementBytes < 0) {
					return elementBytes;
				}

				itemBytesRead += elementBytes;
			}

			bytesRead += itemBytesRead;
		} else if (tag.Element == 0xE0DD) { /* Sequence delimitation item */
			if (fseek(file, 4, SEEK_CUR)) {
				return UNABLE_TO_SKIP_ITEM_LENGTH;
			}
	
			bytesRead += 4;

			/* Done reading */
			break;
		} else {
			return UNEXPECTED_TAG;
		}
	}

	if (visitor.exitSequence) {
		visitor.exitSequence();
	}

	return bytesRead;
}

unsigned long readDicomFile(FILE *file, struct DCMVisitor visitor) {
	unsigned long bytesRead = 0;

	/* Skip first 128 bytes */
	if (fseek(file, 128, SEEK_SET)) {
		return INVALID_HEADER;
	}

	bytesRead += 128;
	
	/* The next 4 bytes should be the DICM header */
		
	if (!readDicomHeader(file)) {
		return NOT_A_DICOM_FILE;
	}

	bytesRead += 4;
	
	while(1) {
		unsigned long elementBytes = readElement(file, visitor);

		if (elementBytes == UNABLE_TO_READ_TAG) {	
			break;
		} else if (elementBytes < 0) {
			return elementBytes;
		}

		bytesRead += elementBytes;
	}

	return bytesRead;
}
