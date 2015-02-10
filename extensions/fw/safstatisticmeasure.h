#ifndef SAFSTATISTICMEASURE_H
#define SAFSTATISTICMEASURE_H

#include "../utils/parameterconfiguration.h"

#include <vector>

namespace nfd
{
namespace fw
{

class SAFStatisticMeasure
{

  ~SAFStatisticMeasure();

  virtual void forwardedInterestTo() = 0;
  virtual void statisfyingInterestFrom() = 0;
  virtual void expiredInterest() = 0;

protected:
  SAFStatisticMeasure(std::vector<int> faces);

  std::vector<int> faces;

};

}
}
#endif // SAFSATISTICMEASURE_H
