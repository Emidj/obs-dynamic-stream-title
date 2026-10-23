#include "compatibility.hpp"
#include <iostream>
int main()
{
 struct Case {std::uint32_t version;bool accepted;};
 constexpr Case cases[]={{0,false},{0x1f010001,false},{0x20000000,true},
  {0x20000006,true},{0x20010000,true},{0x20020002,true},
  {0x20ffffff,true},{0x21000000,false}};
 for(const auto &entry:cases)
  if(dst::supportedObsVersion(entry.version)!=entry.accepted)return 1;
 std::cout<<"PASS: OBS 32.x range including boundary versions\n";
}
