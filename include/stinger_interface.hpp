#ifndef  STINGER_INTERFACE_HPP
#define  STINGER_INTERFACE_HPP

#include <stdint.h>
#include <cstring>

#include "defs.hpp"

namespace Stinger {

  /**
  * @brief This is a definition of an abstract graph interface class.  If analytics use this generic interface and underlying
  *	   datastructures implement it, swapping between different datastructures should be simple.
  */
  template<typename edgeWeight_t, typename vertexWeight_t, typename vertexType_t>
  class StingerInterface {

    public:
      /* Constructors, Destructors */
      virtual ~StingerInterface() = 0;

      /* Vertex Methods */
      virtual vertexWeight_t  setWeight		(vertexID_t v, vertexWeight_t) = 0;
      virtual vertexType_t    setVertexType	(vertexID_t v, vertexType_t) = 0;

      virtual vertexWeight_t  getWeight		(vertexID_t v) = 0;
      virtual vertexType_t    getVertexType	(vertexID_t v) = 0;
      virtual vertexType_t    getVertexInDegree	(vertexID_t v) = 0;
      virtual vertexType_t    getVertexOutDegree(vertexID_t v) = 0;

      virtual vertexID_t      maxVertexID();

      /* Edge Methods */
      virtual uint64_t	      insertEdge	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time) = 0;
      virtual uint64_t	      incrementEdge 	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time) = 0;
      virtual uint64_t	      removeEdge    	(edgeType_t type, vertexID_t from, vertexID_t to) = 0;

      virtual uint64_t	      insertEdgePair	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time) = 0;
      virtual uint64_t	      incrementEdgePair	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time) = 0;
      virtual uint64_t	      removeEdgePair	(edgeType_t type, vertexID_t from, vertexID_t to) = 0;

      /* Utility Methods */
      virtual size_t	      graphSize() = 0;
      virtual size_t	      allocatedSize() = 0;
  };
}

#endif  /*STINGER_INTERFACE_HPP*/
