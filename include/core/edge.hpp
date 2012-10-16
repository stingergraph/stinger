#ifndef  EDGE_HPP
#define  EDGE_HPP

#include "defs.hpp"
#include "core/edge_block.hpp"

typedef int64_t edgeType_t;
typedef uint64_t vertexID_t;
typedef int64_t timeStamp_t;
namespace Stinger {

  template<typename edgeWeight_t> 
  /**
  * @brief Wrapper used to give data to a reader / visitor functor.  Internally, edges are stored in EdgeBlocks.
  */
  struct Edge {
    public:
      Edge();
      Edge(EdgeBlock & eb, ebIndex_t index, bool isReference = true);
  
      edgeType_t    type();
      vertexID_t    source();
      vertexID_t    destination();
      edgeWeight_t  weight();
      timeStamp_t   created();
      timeStamp_t   modified();
    
    private:
      bool	    isReference;

      edgeType_t    type;
      vertexID_t    source;
      vertexID_t    dest;
      edgeWeight_t  weight;
      timeStamp_t   created;
      timeStamp_t   modified;

      EdgeBlock &   eb;
      ebIndex_t	    index;
  };
}

#endif  /*EDGE_HPP*/
