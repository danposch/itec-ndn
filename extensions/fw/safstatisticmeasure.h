#ifndef SAFSTATISTICMEASURE_H
#define SAFSTATISTICMEASURE_H

#include "../utils/parameterconfiguration.h"
#include <boost/shared_ptr.hpp>
#include "fw/strategy.hpp"
#include <vector>
#include <map>

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

  virtual void update(double reliability_t);

  virtual std::vector<int> getReliableFaces(int layer, double threshold);
  virtual std::vector<int> getUnreliableFaces(int layer, double threshold);

  double getLinkReliability(int face_id, int layer);
  int getTotalForwardedInterests(int layer){return stats[layer].total_forwarded_requests;}
  double getActualForwardingProbability(int face_id, int layer){return stats[layer].last_actual_forwarding_probs[face_id];}
  double getUnstatisfiedTrafficFractionOfUnreliableFaces(int ilayer){return stats[ilayer].unstatisfied_traffic_fraction_unreliable_faces;}
  double getForwardedInterests(int face_id, int layer){return getActualForwardingProbability (face_id,layer) * getTotalForwardedInterests (layer);}
  double getSumOfReliabilities(std::vector<int> set_of_faces, int layer);
  double getSumOfUnreliabilities(std::vector<int> set_of_faces, int layer);

protected:
  SAFStatisticMeasure(std::vector<int> faces);

  void calculateTotalForwardedRequests(int layer);
  void calculateLinkReliabilities(int layer);
  void calculateUnstatisfiedTrafficFractionOfUnreliableFaces (int layer, double reliability_t);
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
    //double unstatisfied_traffic_fraction;
    //double satisfied_traffic_fraction;
    //double unstatisfied_traffic_fraction_reliable_faces;

    double unstatisfied_traffic_fraction_unreliable_faces;
    int total_forwarded_requests;

    MeasureIntMap statisfied_requests;
    MeasureIntMap unstatisfied_requests;

    MeasureDoubleMap last_reliability;
    MeasureDoubleMap last_actual_forwarding_probs;

    SAFMesureStats()
    {
      total_forwarded_requests = 0;
      unstatisfied_traffic_fraction_unreliable_faces = 0;
    }

    SAFMesureStats(const SAFMesureStats& other)
    {
      //unstatisfied_traffic_fraction = other.unstatisfied_traffic_fraction;
      //unstatisfied_traffic_fraction_reliable_faces = other.unstatisfied_traffic_fraction_reliable_faces;
      //satisfied_traffic_fraction = other.satisfied_traffic_fraction;

      unstatisfied_traffic_fraction_unreliable_faces = other.unstatisfied_traffic_fraction_unreliable_faces;
      total_forwarded_requests = other.total_forwarded_requests;

      statisfied_requests = other.statisfied_requests;
      unstatisfied_requests = other.unstatisfied_requests;

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
