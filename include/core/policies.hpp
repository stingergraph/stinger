#ifndef  POLICIES_HPP
#define  POLICIES_HPP


namespace Stinger {
  namespace Core {
    namespace Policies {
      template <
	class AllocationPolicy,
	class ThreadPolicy
      >
      class Int64WeightPolicy :
	public AllocationPolicy,
	public ThreadPolicy
      {
      }
    }
  }
}


#endif  /*POLICIES_HPP*/
