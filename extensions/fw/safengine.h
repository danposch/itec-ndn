#ifndef SAFENGINE_H
#define SAFENGINE_H

#include "fw/face-table.hpp"
#include "ns3/event-id.h"
#include <vector>

#include "safentry.h"

namespace nfd
{
namespace fw
{

class SAFEngine
{
public:
  SAFEngine(const nfd::FaceTable& table, unsigned int prefixComponentNumber);

  int determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces, shared_ptr<fib::Entry> fibEntry);

  void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  void logExpiredInterest(shared_ptr< pit::Entry > pitEntry);

protected:
  void initFaces(const nfd::FaceTable& table);
  std::string extractContentPrefix(nfd::Name name);
  std::vector<int> faces;

  void update();

  typedef std::map
    < std::string, /*content-prefix*/
      boost::shared_ptr<SAFEntry> /*forwarding prob. table*/
    > SAFEntryMap;

  SAFEntryMap entryMap;

  ns3::EventId updateEventFWT;

  unsigned int prefixComponentNumber;
};


}
}
#endif // SAFENGINE_H
