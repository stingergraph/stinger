#ifndef  SMART_PTR_HPP
#define  SMART_PTR_HPP

namespace Stinger {
  namespace Util {
    template<typename type_t>
    class SmartPtr{
      public:
	SmartPtr()		{ptr = NULL;}
	SmartPtr(type_t * init)	{ptr = init;}
	~SmartPtr()		{clear();}

	SmartPtr(SmartPtr<type_t> &orig)  {ptr = orig.ptr; orig.clear();}

	SmartPtr<type_t> operator= (SmartPtr<type_t> &rhs) {ptr = rhs.ptr; rhs.clear(); return this;}
	SmartPtr<type_t> operator= (type_t * rhs)	  {set(rhs); return this;}
	inline type_t & operator() ()		  {return *ptr;}

	inline type_t & get ()		  {return *ptr;}
	inline type_t & set (type_t * p)  {clear(); ptr = p; return *ptr; }
	inline void clear()		  {if(ptr){free(ptr); ptr = NULL;}}

      private:
	type_t * ptr;
    };
  }
}

#endif  /*SMART_PTR_HPP*/
