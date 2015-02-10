#include "safstatisticmeasure.h"

using namespace nfd;
using namespace nfd::fw;

SAFStatisticMeasure::SAFStatisticMeasure(std::vector<int> faces)
{
  this->faces = faces;

  // initalize
  for(int layer=0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer ++) // for each layer
  {
    for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
    {
      stats[layer].unstatisfied_requests[*it] = 0;
      stats[layer].statisfied_requests[*it] = 0;

      stats[layer].last_reliability[*it] = 0;
      stats[layer].last_actual_forwarding_probs[*it] = 0;
    }
  }
}

SAFStatisticMeasure::~SAFStatisticMeasure()
{
}

void SAFStatisticMeasure::update (double reliability_t)
{
  for(int layer=0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer ++) // for each layer
  {
    // clear old computed values
    stats[layer].unstatisfied_traffic_fraction_unreliable_faces = 0;
    stats[layer].last_reliability.clear();
    stats[layer].last_actual_forwarding_probs.clear();

    //calculate new values
    calculateTotalForwardedRequests(layer);
    calculateLinkReliabilities (layer);
    calculateUnstatisfiedTrafficFractionOfUnreliableFaces (layer, reliability_t);
    calculateActualForwardingProbabilities (layer);

    //clear collected information
    stats[layer].unstatisfied_requests.clear ();
    stats[layer].statisfied_requests.clear ();

    //initialize for next period
    for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
    {
      stats[layer].unstatisfied_requests[*it] = 0;
      stats[layer].statisfied_requests[*it] = 0;
    }
  }
}

void SAFStatisticMeasure::calculateTotalForwardedRequests(int layer)
{
  stats[layer].total_forwarded_requests = 0;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    stats[layer].total_forwarded_requests += stats[layer].unstatisfied_requests[*it] + stats[layer].statisfied_requests[*it];
  }
}

void SAFStatisticMeasure::calculateLinkReliabilities(int layer)
{
  //NS_LOG_DEBUG("Calculating link reliability for layer " << layer);
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each face
  {
    if(stats[layer].unstatisfied_requests[*it] == 0)
      stats[layer].last_reliability[*it] = 1.0;
    else
      stats[layer].last_reliability[*it] =
          (double)stats[layer].statisfied_requests[*it] / ((double)(stats[layer].unstatisfied_requests[*it] + stats[layer].statisfied_requests[*it]));

    /*NS_LOG_DEBUG("Reliabilty for Face(" << *it << ")=" << stats[layer].last_reliability[*it] << "      in total "
        << stats[layer].unstatisfied_requests[*it] + stats[layer].statisfied_requests[*it] << " interest forwarded");*/
  }
}

void SAFStatisticMeasure::calculateUnstatisfiedTrafficFractionOfUnreliableFaces(int layer, double reliability_t)
{
  if(stats[layer].total_forwarded_requests == 0)
  {
    stats[layer].unstatisfied_traffic_fraction_unreliable_faces = 0;
    return;
  }

  std::vector<int> u_faces = getUnreliableFaces(layer,reliability_t);

  double utf = 0.0;
  for(std::vector<int>::iterator it = u_faces.begin(); it != u_faces.end(); ++it)
  {
    utf += ( (double) (stats[layer].unstatisfied_requests[*it])) / getTotalForwardedInterests(layer);
  }
  stats[layer].unstatisfied_traffic_fraction_unreliable_faces = utf;
}

void SAFStatisticMeasure::calculateActualForwardingProbabilities (int layer)
{
  double sum = stats[layer].total_forwarded_requests;

  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(sum == 0)
      stats[layer].last_actual_forwarding_probs[*it] = 0;
    else
      stats[layer].last_actual_forwarding_probs[*it] =
        (stats[layer].unstatisfied_requests[*it] + stats[layer].statisfied_requests[*it]) / sum;
  }
}

std::vector<int> SAFStatisticMeasure::getReliableFaces(int layer, double threshold)
{
  std::vector<int> reliable;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(*it == DROP_FACE_ID)
      continue;

    if(getLinkReliability(*it, layer) >= threshold)
      reliable.push_back (*it);
  }
  return reliable;
}

std::vector<int>  SAFStatisticMeasure::getUnreliableFaces(int layer, double threshold)
{
  std::vector<int> unreliable;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(*it == DROP_FACE_ID)
      continue;

    if(getLinkReliability(*it, layer) < threshold)
      unreliable.push_back (*it);
  }
  return unreliable;
}

double SAFStatisticMeasure::getLinkReliability(int face_id, int layer)
{
  return stats[layer].last_reliability[face_id];
}

double SAFStatisticMeasure::getSumOfReliabilities(std::vector<int> set_of_faces, int layer)
{
  //sum up reliabilities
  double sum = 0.0;
  for(std::vector<int>::iterator it = set_of_faces.begin(); it != set_of_faces.end(); ++it)
  {
    sum += getLinkReliability (*it, layer);
  }
  return sum;
}

double SAFStatisticMeasure::getSumOfUnreliabilities(std::vector<int> set_of_faces, int layer)
{
  //sum up reliabilities
  double sum = 0.0;
  for(std::vector<int>::iterator it = set_of_faces.begin(); it != set_of_faces.end(); ++it)
  {
    sum += 1 - getLinkReliability (*it, layer);
  }
  return sum;
}

int SAFStatisticMeasure::determineContentLayer(const Interest& interest)
{
  //TODO implement.
  return 0;
}
