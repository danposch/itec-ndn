/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef NDN_VOIP_CLIENT_H
#define NDN_VOIP_CLIENT_H

#include "consumer-cbr-noRtx.h"
#include "fixedjitterbuffer.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class VoIPClient : public ConsumerCbrNoRtx {
public:
  static TypeId GetTypeId();

  VoIPClient();
  virtual ~VoIPClient();

  virtual void StopApplication ();

  virtual void SendPacket();
  virtual void OnData(shared_ptr<const Data> contentObject);

protected:

  typedef std::map<
  uint32_t /*seq_number*/,
  bool /*true if packet lost*/
  > LossMap;

  LossMap lmap;
  std::string burstLogFile;

  //shared_ptr<FixedJitterBuffer> jbuffer;

  //uint32_t fragmentToConsume;
  int lookahead_lifetime;
  //int jitterBufferSize;
  //bool first_packet;
};

} // namespace ndn
} // namespace ns3

#endif
