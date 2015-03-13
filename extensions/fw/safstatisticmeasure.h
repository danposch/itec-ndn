#ifndef SAFSTATISTICMEASURE_H
#define SAFSTATISTICMEASURE_H

#include "../utils/parameterconfiguration.h"
#include <boost/shared_ptr.hpp>
#include "fw/strategy.hpp"
#include <vector>
#include <map>
#include "ns3/log.h"
#include <math.h>
#include <list>

#define HISTORY_SIZE 5
#define INIT_VARIANCE 1000

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
  virtual void logRejectedInterest(shared_ptr<pit::Entry> pitEntry, int face_id) = 0;

  virtual void update(std::map<int, double> reliability_t);

  virtual std::vector<int> getReliableFaces(int layer, double reliability_t);
  virtual std::vector<int> getUnreliableFaces(int layer, double reliability_t);

  double getFaceReliability(int face_id, int layer);
  int getTotalForwardedInterests(int layer){return stats[layer].total_forwarded_requests;}
  double getActualForwardingProbability(int face_id, int layer){return stats[layer].last_actual_forwarding_probs[face_id];}
  int getForwardedInterests(int face_id, int layer){return getS(face_id, layer) + getU(face_id, layer);}
  double getSumOfReliabilities(std::vector<int> set_of_faces, int layer);
  double getSumOfUnreliabilities(std::vector<int> set_of_faces, int layer);

  double getSatisfactionVariance(int face_id, int layer){return stats[layer].satisfaction_variance[face_id];}
  double getAlpha(int face_id, int layer);
  double getEMAAlpha(int face_id, int layer);

  int getS(int face_id, int layer){return stats[layer].last_satisfied_requests[face_id];}
  int getU(int face_id, int layer){return stats[layer].last_unsatisfied_requests[face_id];}

  double getUT(int face_id, int layer){return ((double) getU(face_id, layer)) / ((double)getTotalForwardedInterests(layer));}
  double getST(int face_id, int layer){return ((double) getS(face_id, layer)) / ((double)getTotalForwardedInterests(layer));}

  double getRho(int layer);


protected:
  SAFStatisticMeasure(std::vector<int> faces);

  void calculateTotalForwardedRequests(int layer);
  void calculateLinkReliabilities(int layer, double reliability_t);
  void calculateUnsatisfiedTrafficFractionOfUnreliableFaces (int layer, double reliability_t);
  void calculateActualForwardingProbabilities (int layer);
  void updateVariance(int layer);
  void calculateEMAAlpha(int layer);

  std::vector<int> faces;

  typedef std::map
    < int, /*face id*/
     int /*value to store*/
    > MeasureIntMap;

  typedef std::map
    < int, /*face id*/
     double /*value to store*/
    > MeasureDoubleMap;

  typedef std::map
  <int, /*face id*/
  std::list<int> /*int queue*/
  > MeasureIntList;

  struct SAFMesureStats
  {
    /* variables used for logging*/
    MeasureIntMap satisfied_requests;
    MeasureIntMap unsatisfied_requests;


    /*variables holding information*/
    int total_forwarded_requests;

    MeasureDoubleMap last_reliability;
    MeasureDoubleMap last_actual_forwarding_probs;

    MeasureIntMap last_satisfied_requests;
    MeasureIntMap last_unsatisfied_requests;

    MeasureDoubleMap satisfaction_variance;
    MeasureIntList satisfied_requests_history;

    MeasureDoubleMap ema_alpha;

    SAFMesureStats()
    {
      total_forwarded_requests = 0;
    }

    SAFMesureStats(const SAFMesureStats& other)
    {
      satisfied_requests = other.satisfied_requests;
      unsatisfied_requests = other.unsatisfied_requests;

      total_forwarded_requests = other.total_forwarded_requests;

      last_reliability = other.last_reliability;
      last_actual_forwarding_probs = other.last_actual_forwarding_probs;

      last_satisfied_requests = other.last_satisfied_requests;
      last_unsatisfied_requests = other.last_unsatisfied_requests;

      satisfaction_variance = other.satisfaction_variance;
      satisfied_requests_history = other.satisfied_requests_history;

      ema_alpha = other.ema_alpha;
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
