#ifndef MRATIO_H
#define MRATIO_H

#include "safstatisticmeasure.h"
#include <iostream>
namespace nfd
{
namespace fw
{
class Mratio : public SAFStatisticMeasure
{
public:
  Mratio(std::vector<int> faces);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data);
  virtual void logExpiredInterest(shared_ptr<pit::Entry> pitEntry);
  virtual void logNack(const Face& inFace, const Interest& interest);
  virtual void logRejectedInterest (shared_ptr<pit::Entry> pitEntry, int face_id);

protected:


};

}
}
#endif // MRATIO_H
