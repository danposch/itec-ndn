#ifndef SAFSTATISTICMEASURE_H
#define SAFSTATISTICMEASURE_H

#include "../utils/parameterconfiguration.h"
#include <boost/shared_ptr.hpp>
#include "fw/strategy.hpp"
#include <vector>
#include <map>
#include "ns3/log.h"

namespace nfd
{
namespace fw
{

class SAFStatisticMeasure
{
public:
  ~SAFStatisticMeasure();

  static int determineContentLayer(const Interest& interest);

  virtual void logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data) = 0;
  virtual void logExpiredInterest(shared_ptr<pit::Entry> pitEntry) = 0;
  virtual void logNack(const Face& inFace, const Interest& interest) = 0;
  virtual void logRejectedInterest(shared_ptr<pit::Entry> pitEntry) = 0;

  virtual void update(std::map<int, double> reliability_t);

  virtual std::vector<int> getReliableFaces(int layer, double reliability_t);
  virtual std::vector<int> getUnreliableFaces(int layer, double reliability_t);

  double getLinkReliability(int face_id, int layer);
  int getTotalForwardedInterests(int layer){return stats[layer].total_forwarded_requests;}
  double getActualForwardingProbability(int face_id, int layer){return stats[layer].last_actual_forwarding_probs[face_id];}
  double getUnsatisfiedTrafficFractionOfUnreliableFaces(int ilayer){return stats[ilayer].unsatisfied_traffic_fraction_unreliable_faces;}
  double getForwardedInterests(int face_id, int layer){return getActualForwardingProbability (face_id,layer) * getTotalForwardedInterests (layer);}
  double getSumOfReliabilities(std::vector<int> set_of_faces, int layer);
  double getSumOfUnreliabilities(std::vector<int> set_of_faces, int layer);

protected:
  SAFStatisticMeasure(std::vector<int> faces);

  void calculateTotalForwardedRequests(int layer);
  void calculateLinkReliabilities(int layer, double reliability_t);
  void calculateUnsatisfiedTrafficFractionOfUnreliableFaces (int layer, double reliability_t);
  void calculateActualForwardingProbabilities (int layer);

  std::vector<int> faces;

  typedef std::map
    < int, /*face id*/
     int /*value to store*/
    > MeasureIntMap;

  typedef std::map
    < int, /*face id*/
     double /*value to store*/
    > MeasureDoubleMap;

  struct SAFMesureStats
  {
    //double unsatisfied_traffic_fraction;
    //double satisfied_traffic_fraction;
    //double unsatisfied_traffic_fraction_reliable_faces;

    double unsatisfied_traffic_fraction_unreliable_faces;
    int total_forwarded_requests;

    MeasureIntMap satisfied_requests;
    MeasureIntMap unsatisfied_requests;

    MeasureDoubleMap last_reliability;
    MeasureDoubleMap last_actual_forwarding_probs;

    SAFMesureStats()
    {
      total_forwarded_requests = 0;
      unsatisfied_traffic_fraction_unreliable_faces = 0;
    }

    SAFMesureStats(const SAFMesureStats& other)
    {
      //unsatisfied_traffic_fraction = other.unsatisfied_traffic_fraction;
      //unsatisfied_traffic_fraction_reliable_faces = other.unsatisfied_traffic_fraction_reliable_faces;
      //satisfied_traffic_fraction = other.satisfied_traffic_fraction;

      unsatisfied_traffic_fraction_unreliable_faces = other.unsatisfied_traffic_fraction_unreliable_faces;
      total_forwarded_requests = other.total_forwarded_requests;

      satisfied_requests = other.satisfied_requests;
      unsatisfied_requests = other.unsatisfied_requests;

      last_reliability = other.last_reliability;
      last_actual_forwarding_probs = other.last_actual_forwarding_probs;
    }
  };

  typedef std::map
  <int, /*content layer*/
  SAFMesureStats
  >SAFMesureMap;

  SAFMesureMap stats;

};

}
}
#endif // SAFSATISTICMEASURE_H
