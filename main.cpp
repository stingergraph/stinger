#include "util/array.hpp"
#include "util/_mapped_array.hpp"
#include "util/smart_ptr.hpp"
#include <stdint.h>
#include <iostream>

int
main(int argc, char *argv[]) 
{
  Stinger::Util::Array<int> a = Stinger::Util::Array<int>(20);

  Stinger::Util::sequence(a.rawPointer(), a.sizeElements());

  for(uint64_t i = 0; i < 20; i++)
    std::cout << a[i] << std::endl;

  std::cout << Stinger::Util::reduceSum(a.rawPointer(), a.sizeElements()) << std::endl;

  Stinger::Util::prefixSum(a.rawPointer(), a.sizeElements());

  for(uint64_t i = 0; i < 20; i++)
    std::cout << a[i] << std::endl;

  Stinger::Util::SmartPtr<int> p(new int);
  p() = 30;
  std::cout << p.get();
  p.set(new int);
  p() = 45;
  std::cout << p.get();
  p.clear();
}
