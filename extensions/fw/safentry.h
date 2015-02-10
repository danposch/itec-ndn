#ifndef SAFENTRY_H
#define SAFENTRY_H

#include "safstatisticmeasure.h"
#include "safforwardingtable.h"
#include "fw/strategy.hpp"

namespace nfd
{
namespace fw
{

class SAFEntry
{
public:
  SAFEntry(std::vector<int> faces, shared_ptr<fib::Entry> fibEntry);

  int determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces);

protected:

  void initFaces();

  boost::shared_ptr<SAFStatisticMeasure> smeasure;
  boost::shared_ptr<SAFForwardingTable> ftable;

  std::vector<int> faces;
  std::vector<int> preferedFaces;
  shared_ptr<fib::Entry> fibEntry;
};

}
}
#endif // SAFENTRY_H
