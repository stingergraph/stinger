#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "xmalloc.h"
#include "stinger.h"
#include "csv.h"

int64_t
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

  return length;
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


#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) V_A(X,NULL)

enum csv_fields {
  FIELD_SOURCE,
  FIELD_DEST,
  FIELD_WEIGHT,
  FIELD_TIME,
  FIELD_TYPE
};

int
load_csv_graph (struct stinger * S, const char * filename, int use_numerics)
{
  FILE * fp = fopen (filename, "r");
  if (!fp)
  {
    char errmsg[257];
    snprintf (errmsg, 256, "Opening \"%s\" failed", filename);
    errmsg[256] = '\0';
    perror (errmsg);
    exit (-1);
  }

  char * buf = NULL;
  char ** fields = NULL;
  uint64_t bufSize = 0;
  uint64_t * lengths = NULL;
  uint64_t fieldsSize = 0;
  uint64_t count = 0;
  int64_t line = 0;

  while (!feof(fp))
  {
    int64_t src = 0;
    int64_t dst = 0;
    int64_t wgt = 0;
    int64_t time = 0;
    int64_t type = 0;

    line++;
    readCSVLineDynamic(',', fp, &buf, &bufSize, &fields, &lengths, &fieldsSize, &count);

    if (count <= 1)
      continue;
    if (count < 3) {
      E_A("ERROR: too few elemnts on line %ld", (long) line);
      continue;
    }

    if (!use_numerics)
    {
      /* values are strings */
      stinger_mapping_create (S, fields[FIELD_SOURCE], lengths[FIELD_SOURCE], &src);
      stinger_mapping_create (S, fields[FIELD_DEST], lengths[FIELD_DEST], &dst);
      if (count > 2)
	wgt = atol(fields[FIELD_WEIGHT]);
      if (count > 3)
	time = atol(fields[FIELD_TIME]);
      if (count > 4)
      {
	type = stinger_etype_names_lookup_type (S, fields[FIELD_TYPE]);
	if (type == -1) {
	  stinger_etype_names_create_type (S, fields[FIELD_TYPE], &type);
	}
	if (type == -1) {
	  perror ("Failed to create new edge type");
	  exit(-1);
	}
      }

    } else {
      /* values are integers */
      src = atol(fields[FIELD_SOURCE]);
      dst = atol(fields[FIELD_DEST]);
      if (count > 2)
	wgt = atol(fields[FIELD_WEIGHT]);
      if (count > 3)
	time = atol(fields[FIELD_TIME]);
      if (count > 4)
	type = atol(fields[FIELD_TYPE]);
    }

    //printf("Inserting type=%ld %ld %ld %ld %ld\n", type, src, dst, wgt, time);
    stinger_insert_edge (S, type, src, dst, wgt, time);

  }

  fclose (fp);

  return 0;
}
