#ifndef SAFDELAYSTATISTIC_H
#define SAFDELAYSTATISTIC_H

#include "safstatisticmeasure.h"
#include <vector>

namespace nfd {
namespace fw {

class MDelay : public SAFStatisticMeasure
{
public:
  MDelay(std::vector<int> faces, int max_delay_ms);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  virtual void logExpiredInterest(shared_ptr<pit::Entry> pitEntry);
  virtual void logNack(const Face& inFace, const Interest& interest);

protected:
  time::steady_clock::Duration curMaxDelay;
};

}
}
#endif // SAFDELAYSTATISTIC_H
