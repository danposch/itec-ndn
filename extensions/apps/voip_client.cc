#include "voip_client.h"

#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE("ndn.VoIPClient");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(VoIPClient);

TypeId
VoIPClient::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::VoIPClient")
      .SetGroupName("Ndn")
      .SetParent<ConsumerCbrNoRtx>()
      .AddConstructor<VoIPClient>()
      .AddAttribute("LookaheadLiftime", "The Time (ms) an Interest is send before the Data will be generated at the Producer", IntegerValue(1000),
                    MakeIntegerAccessor(&VoIPClient::lookahead_lifetime), MakeIntegerChecker<int32_t>())
      .AddAttribute("BurstLogFile", "The Time (ms) an Interest is send before the Data will be generated at the Producer", StringValue(""),
                    MakeStringAccessor(&VoIPClient::burstLogFile), MakeStringChecker())
      /*.AddAttribute("JitterBufferSize", "jitter buffer size in ms", IntegerValue(50),
                    MakeIntegerAccessor(&VoIPClient::jitterBufferSize), MakeIntegerChecker<int32_t>())*/

      ;
  return tid;
}

VoIPClient::VoIPClient() : ConsumerCbrNoRtx()
{
  //first_packet = true;
  //fragmentToConsume = 0;
}

VoIPClient::~VoIPClient()
{
}

void VoIPClient::StopApplication ()
{
  if(!burstLogFile.empty ())
  {
    std::ofstream f (burstLogFile);
    if(f.is_open())
    {
      f << "seq_nr, lost\n";
      for(uint32_t i = 0 ; i < m_seq; i++)
      {
        f << boost::lexical_cast<std::string>(i)
        << ","
        <<  boost::lexical_cast<std::string>(lmap[i])
        << "\n";
      }
      f.close ();
    }
    else
      fprintf(stderr, "could not open file: %s\n", burstLogFile.c_str ());
  }
  ConsumerCbrNoRtx::StopApplication ();
}

void VoIPClient::SendPacket()
{
  if (!m_active)
    return;

  /*if(first_packet)
  {
    jbuffer = shared_ptr<FixedJitterBuffer>(
          new FixedJitterBuffer(jitterBufferSize,m_frequency,ns3::Simulator::Now ().ToInteger (ns3::Time::MS) + lookahead_lifetime));
    first_packet = false;
  }*/

  if (m_seqMax != std::numeric_limits<uint32_t>::max())
  {
    if (m_seq >= m_seqMax) {
      return; // we are totally done
    }
  }

  uint32_t seq = m_seq++;

  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);

  //lets append the sim time when the data packet shall be availabe at the producer
  //Time::From(ns3::Simulator::Now().To(ns3::Time::MS) + lookahead_lifetime, ns3::Time::MS)
  //nameWithSequence->appendTimestamp();
  nameWithSequence->appendNumber(ns3::Simulator::Now ().ToInteger (ns3::Time::MS) + lookahead_lifetime);

  lmap[seq] = true;

  nameWithSequence->appendSequenceNumber(seq);

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*nameWithSequence);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds()+lookahead_lifetime);
  interest->setInterestLifetime(interestLifeTime);

  //fprintf(stderr, "Sending Interest %s\ \t %llu \n", nameWithSequence->toUri().c_str(), Simulator::Now ().ToInteger (ns3::Time::MS));

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  ScheduleNextPacket();
}

void VoIPClient::OnData(shared_ptr<const Data> data)
{
  ConsumerCbrNoRtx::OnData(data); // tracing inside
  uint32_t seq = data->getName().at(-1).toSequenceNumber();

  lmap[seq] = false;
  //jbuffer->addFragmentToBuffer(seq);
}

} // namespace ndn
} // namespace ns3
