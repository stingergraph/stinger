#ifndef _MONGODB_STREAM_H_
#define _MONGODB_STREAM_H_


#define MONGO_OBJECT &_it
#define MONGO_TOPLEVEL_OBJECT_FIND_BEGIN(_CURSOR, _FIELD) do { \
							    bson_iterator _it; \
							    if (bson_find (&_it, mongo_cursor_bson (&_CURSOR), _FIELD)) {

#define MONGO_TOPLEVEL_OBJECT_FIND_END()		    } \
							  } while (0);

#define MONGO_TOPLEVEL_OBJECT_FIND(_CURSOR, _FIELD) for(int i__ = 1; i__; i__ = 0) for(bson_iterator _it; i__; i__ = 0) \
							    if (bson_find (&_it, mongo_cursor_bson (&_CURSOR), _FIELD)) 

#define MONGO_SUBOBJECT_FIND_BEGIN(_FIELD)  do { \
					      bson _sub; \
					      bson_iterator_subobject_init (&_it, &_sub, 0); \
					      { \
						bson_iterator _it; \
						if (bson_find (&_it, &_sub, _FIELD)) {


#define MONGO_SUBOBJECT_FIND_END()	        } \
					      } \
					    } while (0);

#define MONGO_SUBOBJECT_FORALL_BEGIN(_FIELD)  do { \
						bson _sub; \
						bson_iterator_subobject_init (&_it, &_sub, 0); \
						{ \
						  bson_iterator _it; \
						  if (bson_find (&_it, &_sub, _FIELD)) { \
						    bson_iterator _sub; \
						    bson_iterator_subiterator (&_it, &_sub); \
						    { \
						      bson_iterator _it = _sub; \
						      while (bson_iterator_next(&_it)) {

#define MONGO_SUBOBJECT_FORALL_END()		      } \
						    } \
						  } \
						} \
					      } while (0);


#endif /* _MONGODB_STREAM_H_ */
