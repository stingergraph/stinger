#include "stinger_core/stinger.h"
#include "pystinger.h"

#define between(v,x,n) (((v) < (x)) && ((v)>(n)))
int
matches(struct stinger * S, global_config * config, int64_t type, int64_t dest, int64_t weight, int64_t timefirst, int64_t timerecent)
{
  int rtn = config->etype == -1 ? (1) : (type == config->etype);
  rtn &= config->vtype == -1 ? (1) : (stinger_vtype(S,dest) == config->vtype);
  rtn &= between(weight, config->weightmax, config->weightmin);
  rtn &= between(timefirst, config->timefirstmax, config->timefirstmin);
  rtn &= between(timerecent, config->timerecentmax, config->timerecentmin);
  return rtn;
}


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
	      global_config * conf)
{
  conf->timefirstmax = INT64_MAX;
  conf->timefirstmin = INT64_MIN;
  conf->timerecentmax = INT64_MAX;
  conf->timerecentmin = INT64_MIN;
  conf->weightmax= INT64_MAX;
  conf->weightmin= INT64_MIN;
  conf->vtype = -1;
  conf->etype = -1;

  if (S == NULL) return -1;

  
  if (timerecentmin >= 0) {
    conf->timerecentmin = timerecentmin;
  }
  if (timerecentmax >= 0)
    conf->timerecentmax = timerecentmax;
  
  if (timefirstmin >= 0)
    conf->timefirstmin = timefirstmin;
  if (timefirstmax >= 0)
    conf->timefirstmax = timefirstmax;

  if (weightmax >= 0)
    conf->weightmax = weightmax;
  if (weightmin >= 0)
    conf->weightmin = weightmin;

  if (vtype != NULL) {
    conf->vtype = stinger_vtype_names_lookup_type (S, vtype);
  }

  if (etype != NULL) {
    conf->etype = stinger_etype_names_lookup_type (S, etype);
  }

  return 0;
}

