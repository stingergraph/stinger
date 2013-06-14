#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "xmalloc.h"
#include "stinger.h"
#include "stinger-physmap.h"
#include "csv.h"

void
readCSVLineDynamic(char delim, FILE * file, char ** buf, uint64_t * bufSize, char *** fields, uint64_t ** lengths, uint64_t * fieldsSize, uint64_t * count) 
{
  char	   * locBuf         = *buf;
  uint64_t   locBufSize     = *bufSize;
  char    ** locFields      = *fields;
  uint64_t * locLengths	    = *lengths;
  uint64_t   locFieldsSize  = *fieldsSize;
  uint64_t   locCount       = *count;

  if((!locBuf) || (locBufSize <= 0)) {
    locBufSize = 4096;
    locBuf = xmalloc(sizeof(char) * locBufSize);
  }

  if((!locFields) || (locFieldsSize <= 0)) {
    locFieldsSize = 5;
    locFields  = xmalloc(sizeof(char *) * locFieldsSize);
    locLengths = xmalloc(sizeof(uint64_t)    * locFieldsSize);
  }
  locCount = 1;
  locLengths[0] = 0;
  locFields[0] = locBuf;

  int in_quote = 0;
  char cur = '\0';
  uint64_t length = 0;
  uint64_t start  = 0;
  uint64_t field  = 0;
  while((cur != '\n') && (cur != EOF)) {
    if((length == locBufSize) && (cur != EOF)) {
      locBufSize *= 2;
      locBuf= xrealloc(locBuf, sizeof(char) * locBufSize);
    }
    cur = getc(file);
    if((cur != delim) && (cur != '\n')) {
      locBuf[length++] = cur;
    } else {
      locBuf[length++] = '\0';
      locLengths[field] = length - 1 - start;
      start = length;
      field++;
      if(cur != EOF) {
	if(field == locFieldsSize) {
	  locFieldsSize*= 2;
	  locFields = xrealloc(locFields, sizeof(char *) * locFieldsSize);
	  locLengths = xrealloc(locLengths, sizeof(uint64_t) * locFieldsSize);
	}
	locFields[field] = locBuf + length;
      }
    }
  }
  if(length == locBufSize) {
    locBufSize += 1;
    locBuf = xrealloc(locBuf, sizeof(char) * locBufSize);
  }
  locCount = field;

  *buf	      = locBuf;
  *bufSize    = locBufSize;
  *fields     = locFields;
  *lengths    = locLengths;
  *fieldsSize = locFieldsSize;
  *count      = locCount;
  *lengths    = locLengths;
}

void
splitLineCSVDynamic(char delim, char * line, uint64_t lineSize, char ** buf, uint64_t * bufSize, char *** fields, uint64_t ** lengths, uint64_t * fieldsSize, uint64_t * count) 
{
  char	   * locBuf         = *buf;
  uint64_t   locBufSize     = *bufSize;
  char    ** locFields      = *fields;
  uint64_t * locLengths	    = *lengths;
  uint64_t   locFieldsSize  = *fieldsSize;
  uint64_t   locCount       = *count;

  if((!locBuf) || (locBufSize <= 0)) {
    locBufSize = 4096;
    locBuf = xmalloc(sizeof(char) * locBufSize);
  }

  if((!locFields) || (locFieldsSize <= 0)) {
    locFieldsSize = 5;
    locFields  = xmalloc(sizeof(char *) * locFieldsSize);
    locLengths = xmalloc(sizeof(uint64_t)    * locFieldsSize);
  }
  locCount = 1;
  locLengths[0] = 0;
  locFields[0] = locBuf;

  char cur = '\0';
  uint64_t length = 0;
  uint64_t start  = 0;
  uint64_t field  = 0;
  while((cur != '\n') && (length < lineSize)) {
    if((length == locBufSize) && (cur != EOF)) {
      locBufSize *= 2;
      locBuf= xrealloc(locBuf, sizeof(char) * locBufSize);
    }
    cur = line[length];
    if((cur != delim) && (cur != '\n')) {
      locBuf[length++] = cur;
    } 
    if((cur == delim) || (cur == '\n') || (length == lineSize)){
      locBuf[length++] = '\0';
      locLengths[field] = length - 1 - start;
      start = length;
      field++;
      if(cur != EOF) {
	if(field == locFieldsSize) {
	  locFieldsSize*= 2;
	  locFields = xrealloc(locFields, sizeof(char *) * locFieldsSize);
	  locLengths = xrealloc(locLengths, sizeof(uint64_t) * locFieldsSize);
	}
	locFields[field] = locBuf + length;
      }
    }
  }
  if(length == locBufSize) {
    locBufSize += 1;
    locBuf = xrealloc(locBuf, sizeof(char) * locBufSize);
  }
  locCount = field;

  *buf	      = locBuf;
  *bufSize    = locBufSize;
  *fields     = locFields;
  *lengths    = locLengths;
  *fieldsSize = locFieldsSize;
  *count      = locCount;
  *lengths    = locLengths;
}

void
splitLineCSVDynamicInPlace(char delim, char * line, uint64_t lineSize, char *** fields, uint64_t ** lengths, uint64_t * fieldsSize, uint64_t * count) 
{
  char    ** locFields      = *fields;
  uint64_t * locLengths	    = *lengths;
  uint64_t   locFieldsSize  = *fieldsSize;
  uint64_t   locCount       = *count;

  if((!locFields) || (locFieldsSize <= 0)) {
    locFieldsSize = 5;
    locFields  = xmalloc(sizeof(char *) * locFieldsSize);
    locLengths = xmalloc(sizeof(uint64_t)    * locFieldsSize);
  }
  locCount = 1;
  locLengths[0] = 0;
  locFields[0] = line;

  char cur = '\0';
  uint64_t length = 0;
  uint64_t start  = 0;
  uint64_t field  = 0;
  while((cur != '\n') && (length < lineSize)) {
    cur = line[length];
    if((cur != delim) && (cur != '\n')) {
      length++;
    } 
    if((cur == delim) || (cur == '\n') || (length == lineSize)){
      line[length++] = '\0';
      locLengths[field] = length - 1 - start;
      start = length;
      field++;
      if(cur != EOF) {
	if(field == locFieldsSize) {
	  locFieldsSize*= 2;
	  locFields = xrealloc(locFields, sizeof(char *) * locFieldsSize);
	  locLengths = xrealloc(locLengths, sizeof(uint64_t) * locFieldsSize);
	}
	locFields[field] = line + length;
      }
    }
  }
  locCount = field;

  *fields     = locFields;
  *lengths    = locLengths;
  *fieldsSize = locFieldsSize;
  *count      = locCount;
  *lengths    = locLengths;
}

int
getIndex(char * string, char ** fields, uint64_t * lengths, uint64_t count) {
  int rtn = 0;
  for(; rtn < count; rtn++) {
    if(0 == strncmp(string, fields[rtn], lengths[rtn]))
      return rtn;
  }
  return -1;
}

void
printLine(char ** fields, uint64_t * lengths, uint64_t count) {
  uint64_t i = 0;
  for(; i < count; i++) {
    printf("field[%ld] (%ld) = %s\n", i, lengths[i], fields[i]);
  }
}

void
csvIfIDExistsint8(FILE * fp, char delim, struct stinger * S, uint64_t nv, int8_t * values) {
    for(uint64_t v = 0; v < nv; v++) {
      if(stinger_vtype_get(S, v) != 0) {
	uint64_t len;
	char * name;
	stinger_mapping_physid_direct(S, v, &name, &len);
      char * type = stinger_vtype_names_lookup_name(S, stinger_vtype_get(S, v));
	if(len && name) {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%ld\n", v, delim, (int)len, name, delim, type ? type : "", delim, stinger_vtype(S,v), delim, (long)values[v]);
	} else {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%ld\n", v, delim, 0, "", delim, type ? type : "", delim, stinger_vtype(S,v), delim, (long)values[v]);
      }
    }
  }
}

void
csvIfIDExistsint64(FILE * fp, char delim, struct stinger * S, uint64_t nv, int64_t * values) {
    for(uint64_t v = 0; v < nv; v++) {
      if(stinger_vtype_get(S, v) != 0) {
	uint64_t len;
	char * name;
	stinger_mapping_physid_direct(S, v, &name, &len);
      char * type = stinger_vtype_names_lookup_name(S, stinger_vtype_get(S, v));
	if(len && name) {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%ld\n", v, delim, (int)len, name, delim, type ? type : "", delim, stinger_vtype(S,v), delim, values[v]);
	} else {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%ld\n", v, delim, 0, "", delim, type ? type : "", delim, stinger_vtype(S,v), delim, values[v]);
      }
    }
  }
}

void
csvIfIDExistsfloat(FILE * fp, char delim, struct stinger * S, uint64_t nv, float * values) {
    for(uint64_t v = 0; v < nv; v++) {
      if(stinger_vtype_get(S, v) != 0) {
	uint64_t len;
	char * name;
	stinger_mapping_physid_direct(S, v, &name, &len);
      char * type = stinger_vtype_names_lookup_name(S, stinger_vtype_get(S, v));
	if(len && name) {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%f\n", v, delim, (int)len, name, delim, type ? type : "", delim, stinger_vtype(S,v), delim, values[v]);
	} else {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%f\n", v, delim, 0, "", delim, type ? type : "", delim, stinger_vtype(S,v), delim, values[v]);
      }
    }
  }
}

void
csvIfIDExistsdouble(FILE * fp, char delim, struct stinger * S, uint64_t nv, double * values) {
    for(uint64_t v = 0; v < nv; v++) {
      if(stinger_vtype_get(S, v) != 0) {
	uint64_t len;
	char * name;
      char * type = stinger_vtype_names_lookup_name(S, stinger_vtype_get(S, v));
	stinger_mapping_physid_direct(S, v, &name, &len);
	if(len && name) {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%lf\n", v, delim, (int)len, name, delim, type ? type : "", delim, stinger_vtype(S,v), delim, values[v]);
	} else {
	fprintf(fp, "%ld%c%.*s%c%s%c%ld%c%lf\n", v, delim, 0, "", delim, type ? type : "", delim, stinger_vtype(S,v), delim, values[v]);
      }
    }
  }
}
