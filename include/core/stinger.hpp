#ifndef  STINGER_HPP
#define  STINGER_HPP

#include <stdint.h>
#include <cstring>

#include "stinger_interface.hpp"
#include "defs.hpp"
#include "core/edge_block.hpp"
#include "core/vertex_block.hpp"
#include "util/mapped_array.hpp"

namespace Stinger {

  /**
  * @brief This is the base STINGER implementation. It inherits and implements the StingerInterface.
  */
  template<typename edgeWeight_t, typename vertexWeight_t, typename vertexType_t>
  class Stinger : public StingerInterface<edgeWeight_t, vertexWeight_t, vertexType_t> {

    public:
      /* Constructors, Destructors */
      Stinger(uint64_t nv, uint64_t numEdgeTypes = 1);
      ~Stinger();

      /* Vertex Methods */
      vertexWeight_t  setWeight		(vertexID_t v, vertexWeight_t);
      vertexType_t    setVertexType	(vertexID_t v, vertexType_t);

      vertexWeight_t  getWeight		(vertexID_t v);
      vertexType_t    getVertexType	(vertexID_t v);
      vertexType_t    getVertexInDegree	(vertexID_t v);
      vertexType_t    getVertexOutDegree(vertexID_t v);

      vertexID_t      maxVertexID();

      /* Edge Methods */
      uint64_t	      insertEdge	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time);
      uint64_t	      incrementEdge 	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time);
      uint64_t	      removeEdge    	(edgeType_t type, vertexID_t from, vertexID_t to);

      uint64_t	      insertEdgePair	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time);
      uint64_t	      incrementEdgePair	(edgeType_t type, vertexID_t from, vertexID_t to, edgeWeight_t weight, timeStamp_t time);
      uint64_t	      removeEdgePair	(edgeType_t type, vertexID_t from, vertexID_t to);

      /* Utility Methods */
      size_t	      graphSize();
      size_t	      allocatedSize();

    private:
      Util::MappedArray<VertexBlock>	LVA;
      Util::MappedArray<EdgeTypeArray>	ETAs;
  };
}

#endif  /*STINGER_HPP*/
