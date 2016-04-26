#ifndef ADAPTATIONENGINE_H
#define ADAPTATIONENGINE_H

#include "safstatisticmeasure.h"
#include "safforwardingtable.h"

namespace nfd
{
namespace fw
{

class AdaptationEngine
{

public:
  AdaptationEngine(std::vector<int> faces);
  void registerSMeasure(std::string prefix, boost::shared_ptr<SAFStatisticMeasure> smeasure, boost::shared_ptr<SAFForwardingTable> ftable);
  void setAndUpdateWeights();
  void updateFaces(std::vector<int> faces);

protected:
  std::vector<int> faces;

  struct stats
  {
    boost::shared_ptr<SAFStatisticMeasure> smeasure;
    boost::shared_ptr<SAFForwardingTable> ftable;
  };

  std::vector<std::string> sortRelevantsByPriority(std::vector<std::string> vec);

  typedef std::map<
  std::string/*prefix*/,
  stats>
  SMeasureMap;

  SMeasureMap smap;
};

}
}
#endif // ADAPTATIONENGINE_H
