#include "mdelay.h"
#include "climits"
#include <iostream>
#include "../utils/parameterconfiguration.h"

namespace nfd {
namespace fw {

MDelay::MDelay(std::vector<int> faces, int max_delay_ms) : Mratio(faces)
{
  this->type = MeasureType::MDelay;
  curMaxDelay = boost::chrono::duration<long int, boost::ratio<1l, 1000000000l> >(
              (long) max_delay_ms * 1000000);
}

void MDelay::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  time::steady_clock::TimePoint now = time::steady_clock::now();
  std::list<nfd::pit::OutRecord>::const_iterator outRecord = pitEntry->getOutRecord(inFace);

  time::steady_clock::Duration rtt = boost::chrono::duration<long int, boost::ratio<1l, 1000000000l> >(now - outRecord->getLastRenewed());

  if (rtt > curMaxDelay)
    Mratio::logExpiredInterest(pitEntry);
  else
    Mratio::logSatisfiedInterest(pitEntry,inFace, data);
}

}
}


