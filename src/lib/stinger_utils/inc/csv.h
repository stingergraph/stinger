#ifndef  CSV_H
#define  CSV_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdio.h>

int64_t
readCSVLineDynamic(char delim, FILE * file, char ** buf, uint64_t * bufSize, char *** fields, uint64_t ** lengths, uint64_t * fieldsSize, uint64_t * count);

void
splitLineCSVDynamic(char delim, char * line, uint64_t lineSize, char ** buf, uint64_t * bufSize, char *** fields, uint64_t ** lengths, uint64_t * fieldsSize, uint64_t * count);

void
splitLineCSVDynamicInPlace(char delim, char * line, uint64_t lineSize, char *** fields, uint64_t ** lengths, uint64_t * fieldsSize, uint64_t * count);

int
getIndex(char * string, char ** fields, uint64_t * lengths, uint64_t count);

void
printLine(char ** fields, uint64_t * lengths, uint64_t count);

void
csvIfIDExistsint8(FILE * fp, char delim, struct stinger * S, uint64_t nv, int8_t * values);

void
csvIfIDExistsint64(FILE * fp, char delim, struct stinger * s, uint64_t nv, int64_t * values);

void
csvIfIDExistsfloat(FILE * fp, char delim, struct stinger * s, uint64_t nv, float * values);

void
csvIfIDExistsdouble(FILE * fp, char delim, struct stinger * s, uint64_t nv, double * values);

int
load_csv_graph (struct stinger * S, char * filename, int use_numerics);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*CSV_H*/
