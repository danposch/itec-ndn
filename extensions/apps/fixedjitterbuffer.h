#ifndef FIXEDJITTERBUFFER_H
#define FIXEDJITTERBUFFER_H

#include <map>
#include "ns3-dev/ns3/simulator.h"

namespace ns3 {
namespace ndn {

class FixedJitterBuffer
{
public:
  FixedJitterBuffer(uint32_t bufferDelay, double fragment_frequency, uint32_t reference_time_point);

  void addFragmentToBuffer(uint32_t seq);
  void tryConsumeFragment(uint32_t seq);

protected:
  typedef std::map<
  uint32_t /*seq_number*/,
  uint32_t /*received at sim time*/
  > JitterBuffer;

  JitterBuffer buffer;

  uint32_t maxBufferDelay;
  uint32_t reference_time_point;
  uint32_t fragment_duration;

};

}
}
#endif // FIXEDJITTERBUFFER_H
