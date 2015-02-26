#ifndef SAFENGINE_H
#define SAFENGINE_H

#include "fw/face-table.hpp"
#include "ns3/event-id.h"
#include <vector>
#include "limits/facelimitmanager.h"
#include "ns3/names.h"
#include "ns3/log.h"
#include "safentry.h"

namespace nfd
{
namespace fw
{

class SAFEngine
{
public:
  SAFEngine(const nfd::FaceTable& table, unsigned int prefixComponentNumber);

  int determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces, shared_ptr<fib::Entry> fibEntry);
  bool tryForwardInterest(const Interest& interest, shared_ptr<Face>);

  void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  void logExpiredInterest(shared_ptr< pit::Entry > pitEntry);
  void logNack(const Face& inFace, const Interest& interest);
  void logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id);

protected:
  void initFaces(const nfd::FaceTable& table);
  std::string extractContentPrefix(nfd::Name name);
  void determineNodeName(const nfd::FaceTable& table);
  std::vector<int> faces;

  void update();

  typedef std::map
    < std::string, /*content-prefix*/
      boost::shared_ptr<SAFEntry> /*forwarding prob. table*/
    > SAFEntryMap;

  SAFEntryMap entryMap;

  typedef std::map
    < int, /*face ID*/
      boost::shared_ptr<FaceLimitManager> /*face limit manager*/
    > FaceLimitMap;

  FaceLimitMap fbMap;

  ns3::EventId updateEventFWT;

  unsigned int prefixComponentNumber;
  std::string nodeName;
};


}
}
#endif // SAFENGINE_H
