#include "fixedjitterbuffer.h"

namespace ns3 {
namespace ndn {


FixedJitterBuffer::FixedJitterBuffer(uint32_t bufferDelay, double fragment_frequency, uint32_t reference_time_point)
{
  this->fragment_duration = (uint32_t) (1000.0 / fragment_frequency); //ms
  this->maxBufferDelay = bufferDelay; //ms
  this->reference_time_point = reference_time_point; //ms
}

void FixedJitterBuffer::addFragmentToBuffer(uint32_t seq)
{
  //check if added before timeout event.
  uint32_t currentTime = ns3::Simulator::Now ().ToInteger (ns3::Time::MS);
  uint32_t playbackTime = reference_time_point + (seq * fragment_duration) + maxBufferDelay;

  fprintf(stderr, "current time = %d\n", currentTime);
  fprintf(stderr, "packetPlaybackTime = %d\n",playbackTime);

  if(currentTime < playbackTime)
  {
    //enque
    buffer[seq] = currentTime;
    fprintf(stderr,"packet stored\n");
    //todo store delay
  }
  else
  {
    //packet to late
    fprintf(stderr,"packet to late\n");
  }

}

void FixedJitterBuffer::tryConsumeFragment(uint32_t seq)
{

}

}
}
