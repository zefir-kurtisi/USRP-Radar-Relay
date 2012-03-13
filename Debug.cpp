#include "Debug.hpp"

#if NO_DEBUG
std::ostream DLOG(0);
#else
std::ostream& DLOG = std::clog;
#endif

std::ostream& DINFO = std::cout;
std::ostream& DWARN = std::cout;
std::ostream& DERR = std::cerr;
