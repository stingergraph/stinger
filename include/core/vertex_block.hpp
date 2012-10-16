#ifndef  VERTEX_BLOCK_HPP
#define  VERTEX_BLOCK_HPP

#include "defs.hpp"

namespace Stinger {
  template<typename vertexWeight_t, typename vertexType_t>
  class VertexBlock {
    vertexType_t    type;
    vertexWeight_t  weight;
    degree_t	    inDegree;
    degree_t	    outDegree;
    ebIndex_t	    edges;
  };
}

#endif  /*VERTEX_BLOCK_HPP*/
