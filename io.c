/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2024 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 *    Version 2.0
 *
 */
#include "vpc.h"

void DoRead(UCHAR* inFile)
{
	FILE* f = fopen(inFile, "r");
	if (f == NULL) {
		gcError++; fprintf(stderr,"Error: unable to open the file(%s).\n", inFile);
		return;
	}
	ReadLines(f);
	fclose(f);
}

void DoWrite(UCHAR* otFile)
{
	int i;
	FILE* f = fopen(otFile, "w");
	if (f == NULL) {
		gcError++; fprintf(stderr,"Error: unable to open the file(%s).\n", otFile);
		return;
	}
	PrintTitle(f);
	PrintFormat(f);
	PrintPrecision(f);
	PrintRound(f);
	PrintIterations(f);
	for (i = 0; i < 26; ++i) {
		OutputVariableTitle(f,'a'+i);
		PrintVariable(f, 'a' + i);
	}
	fclose(f);
}

