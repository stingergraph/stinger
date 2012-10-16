#ifndef  EDGE_BLOCK_HPP
#define  EDGE_BLOCK_HPP

#include "defs.hpp"

namespace Stinger {

  template<size_t edgesPerBlock, typename edgeWeight_t> 
  class EdgeBlock {
    public:
      EdgeBlock();

    private:
      /* block metadata */
      ebIndex_t	    next;
      vertexID_t    vertex;
      edgeType_t    type;
      ebIndex_t	    high;
      ebIndex_t	    numEdges;
      timeStamp_t   smallTimeStamp;
      timeStamp_t   largeTimeStamp;

      /* edges */
      vertexID_t    neighbor[edgesPerBlock];
      edgeWeight_t  weight  [edgesPerBlock];
      timeStamp_t   created [edgesPerBlock];
      timeStamp_t   modified[edgesPerBlock];
  };
}

#endif  /*EDGE_BLOCK_HPP*/
