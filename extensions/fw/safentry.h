#ifndef SAFENTRY_H
#define SAFENTRY_H

#include "safstatisticmeasure.h"
#include "safforwardingtable.h"
#include "fw/strategy.hpp"
#include "mratio.h"

namespace nfd
{
namespace fw
{

class SAFEntry
{
public:
  SAFEntry(std::vector<int> faces, shared_ptr<fib::Entry> fibEntry);

  int determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces);

  void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  void logExpiredInterest(shared_ptr< pit::Entry > pitEntry);

  void update();

protected:

  void initFaces();

  boost::shared_ptr<SAFStatisticMeasure> smeasure;
  boost::shared_ptr<SAFForwardingTable> ftable;

  std::vector<int> faces;
  typedef std::map<
  int/*faceId*/,
  int /*costs*/> PreferedFaceMap;
  PreferedFaceMap preferedFaces;

  shared_ptr<fib::Entry> fibEntry;
};

}
}
#endif // SAFENTRY_H
