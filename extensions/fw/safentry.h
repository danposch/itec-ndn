#ifndef SAFENTRY_H
#define SAFENTRY_H

#include "safstatisticmeasure.h"
#include "safforwardingtable.h"

namespace nfd
{
namespace fw
{

class SAFEntry
{
public:
  SAFEntry(std::vector<int> faces);

  int determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces);

protected:
  boost::shared_ptr<SAFStatisticMeasure> smeasure;
  boost::shared_ptr<SAFForwardingTable> ftable;
};

}
}
#endif // SAFENTRY_H
