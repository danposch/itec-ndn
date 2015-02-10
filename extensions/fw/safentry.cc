#include "safentry.h"

using namespace nfd;
using namespace nfd::fw;

SAFEntry::SAFEntry(std::vector<int> faces)
{
}

int SAFEntry::determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces)
{
  return 0;
}
