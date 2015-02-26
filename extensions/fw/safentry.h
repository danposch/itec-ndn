#ifndef SAFENTRY_H
#define SAFENTRY_H

#include "safstatisticmeasure.h"
#include "safforwardingtable.h"
#include "fw/strategy.hpp"
#include "mratio.h"
#include "mdelay.h"

namespace nfd
{
namespace fw
{

class SAFEntry
{
public:
  SAFEntry(std::vector<int> faces, shared_ptr<fib::Entry> fibEntry);

  int determineNextHop(const Interest& interes, std::vector<int> alreadyTriedFaces);

  void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  void logExpiredInterest(shared_ptr< pit::Entry > pitEntry);
  void logNack(const Face& inFace, const Interest& interest);
  void logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id);

  void update();

protected:

  void initFaces();
  bool evaluateFallback();

  boost::shared_ptr<SAFStatisticMeasure> smeasure;
  boost::shared_ptr<SAFForwardingTable> ftable;

  std::vector<int> faces;
  typedef std::map<
  int/*faceId*/,
  int /*costs*/> PreferedFaceMap;
  PreferedFaceMap preferedFaces;

  shared_ptr<fib::Entry> fibEntry;

  int fallbackCounter;
};

}
}
#endif // SAFENTRY_H
