#include "safentry.h"

using namespace nfd;
using namespace nfd::fw;

SAFEntry::SAFEntry(std::vector<int> faces, shared_ptr<fib::Entry> fibEntry)
{
  this->fibEntry = fibEntry;
  this->faces = faces;
  initFaces();

  ftable = boost::shared_ptr<SAFForwardingTable>(new SAFForwardingTable(this->faces, this->preferedFaces));
}

void SAFEntry::initFaces ()
{
  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  for(fib::NextHopList::const_iterator it = nexthops.begin (); it != nexthops.end (); it++)
  {
    std::vector<int>::iterator face = std::find(faces.begin (),faces.end (), (*it).getFace()->getId());
    if(face != faces.end ())
      preferedFaces.push_back (*face);
  }
}

int SAFEntry::determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces)
{
  return ftable->determineNextHop (interest,originInFaces,alreadyTriedFaces);
}
