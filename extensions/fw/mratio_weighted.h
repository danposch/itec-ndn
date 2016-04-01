#ifndef SAFWEIGHTEDRATIO_H
#define SAFWEIGHTEDRATIO_H

#include "mratio.h"
#include <vector>

namespace nfd {
namespace fw {

class MWeightedRatio : public Mratio
{
public:
  MWeightedRatio(std::vector<int> faces, int satisfied_weight, int unsatisfied_weight);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  virtual void logExpiredInterest(shared_ptr<pit::Entry> pitEntry);
  virtual void logNack(const Face& inFace, const Interest& interest);
  virtual void logRejectedInterest (shared_ptr<pit::Entry> pitEntry, int face_id);

protected:
  int  satisfied_weight;
  int unsatisfied_weight;
};

}
}
#endif // SAFDELAYSTATISTIC_H
