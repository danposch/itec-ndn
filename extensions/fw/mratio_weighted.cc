#include "mratio_weighted.h"
#include "climits"
#include <iostream>
#include "../utils/parameterconfiguration.h"

namespace nfd {
namespace fw {

MWeightedRatio::MWeightedRatio(std::vector<int> faces, int satisfied_weight, int unsatisfied_weight) : Mratio(faces)
{
  //fprintf(stderr, "MWeightedRatio(%d,%d)\n", satisfied_weight,unsatisfied_weight);
  this->type = MeasureType::MWeightedThrouput;
  this->satisfied_weight = satisfied_weight;
  this->unsatisfied_weight = unsatisfied_weight;
}

void MWeightedRatio::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());
  stats[ilayer].satisfied_requests[inFace.getId ()] += satisfied_weight;
}

void MWeightedRatio::logExpiredInterest(shared_ptr<pit::Entry> pitEntry)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());

  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();
  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    stats[ilayer].unsatisfied_requests[(*it).getFace()->getId()] += unsatisfied_weight;
  }
}

void MWeightedRatio::logNack(const Face &inFace, const Interest &interest)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(interest);
  stats[ilayer].unsatisfied_requests[inFace.getId()] += unsatisfied_weight;
}

void MWeightedRatio::logRejectedInterest (shared_ptr<pit::Entry> pitEntry, int face_id)
{
  int ilayer = SAFStatisticMeasure::determineContentLayer(pitEntry->getInterest());

  if(face_id == DROP_FACE_ID)
    stats[ilayer].satisfied_requests[face_id] += satisfied_weight;
  else
    stats[ilayer].unsatisfied_requests[face_id] += unsatisfied_weight;
}

}
}


