#ifndef  VERTEX_BLOCK_HPP
#define  VERTEX_BLOCK_HPP

#include "defs.hpp"
#include "return.hpp""

namespace Stinger {
  namespace Core {
    template <
      class WeightPolicy, 
      class TypePolicy, 
      class TimePolicy, 
      class DegreePolicy, 
      class EdgeAccessPolicy,
      class AllocationPolicy,
      class ThreadPolicy
    >
    class Vertex : 
      public WeightPolicy    <AllocationPolicy, ThreadPolicy>,
      public TypePolicy      <AllocationPolicy, ThreadPolicy>,
      public TimePolicy      <AllocationPolicy, ThreadPolicy>,
      public DegreePolicy    <AllocationPolicy, ThreadPolicy>,
      public EdgeAccessPolicy<AllocationPolicy, ThreadPolicy>,
      public NamePolicy<AllocationPolicy, ThreadPolicy>,
      public AllocationPolicy,
      public ThreadPolicy
    {
    };
  }
}

#endif  /*VERTEX_BLOCK_HPP*/
