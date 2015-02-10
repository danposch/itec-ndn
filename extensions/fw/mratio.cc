#include "mratio.h"

using namespace nfd;
using namespace nfd::fw;

Mratio::Mratio(std::vector<int> faces) : SAFStatisticMeasure(faces)
{
}

void Mratio::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());
  stats[ilayer].satisfied_requests[inFace.getId ()] += 1;
}

void Mratio::logExpiredInterest(shared_ptr<pit::Entry> pitEntry)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());

  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();
  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    stats[ilayer].unsatisfied_requests[(*it).getFace()->getId()] += 1;
  }
}
