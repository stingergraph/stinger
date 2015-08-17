
typedef struct {
  int64_t timerecentmin;
  int64_t timerecentmax;
  int64_t timefirstmin;
  int64_t timefirstmax;
  int64_t weightmax;
  int64_t weightmin;
  int64_t vtype;
  int64_t etype;
} global_config;

int
matches(struct stinger * S, global_config * config, int64_t type, int64_t dest, int64_t weight, int64_t timefirst, int64_t timerecent);

int
parse_config (struct stinger * S,
	      int64_t timerecentmin,
	      int64_t timerecentmax,
	      int64_t timefirstmin,
	      int64_t timefirstmax,
	      int64_t weightmax,
	      int64_t weightmin,
	      char * vtype,
	      char * etype,
	      global_config * conf);
