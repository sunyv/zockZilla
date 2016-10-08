
#include "../include/IMatcher.h"

namespace Lib
{
namespace Controller
{
namespace API
{

   std::ostream& operator<<( std::ostream& os, IMatch const& match )
   {
      return os << match.toString();
   }
   
   bool operator<( IMatch const& a, IMatch const& b )
   {  
      return a.getName() < b.getName(); 
   }
   
   std::ostream& operator<<( std::ostream& os, IMatcher const& matcher )
   {
      return os << matcher.toString();
   }

}}}