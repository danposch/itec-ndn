/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "voip_producer.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"

#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.VoIPProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(VoIPProducer);

TypeId
VoIPProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::VoIPProducer")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<VoIPProducer>()
      ;
  return tid;
}

VoIPProducer::VoIPProducer() : Producer ()
{
  removePendingEvent = ns3::Simulator::Schedule(ns3::Seconds (1), &VoIPProducer::removeExpiredEvents, this);
}

void VoIPProducer::removeExpiredEvents()
{
  std::vector<ns3::EventId>::iterator it = pending_events.begin ();

  while(it != pending_events.end ())
  {
    if(it->IsExpired())
      it = pending_events.erase (it);
    else
      it++;
  }
  removePendingEvent = ns3::Simulator::Schedule(ns3::Seconds (1), &VoIPProducer::removeExpiredEvents, this);
}

VoIPProducer::~VoIPProducer()
{
  removeExpiredEvents();
  ns3::Simulator::Cancel (removePendingEvent);

  std::vector<ns3::EventId>::iterator it = pending_events.begin ();

  while(it != pending_events.end ())
  {
    ns3::Simulator::Cancel (*it);
    it = pending_events.erase (it);
  }
}

void VoIPProducer::OnInterest(shared_ptr<const Interest> interest)
{

  //fprintf(stderr, "VoIPProducer::OnInterest %s\n",interest->getName().toUri().c_str());

  ndn::Name name = interest->getName();
  uint64_t time = name[-2].toNumber();
  uint64_t now = Simulator::Now ().ToInteger(ns3::Time::MS);

  if(now < time)
  {
    //fprintf(stderr, "Scheduled: %llu\n", time-now);
    //TODO STORE EVENT AND CANCEL ON DECONSTRUCT
    pending_events.push_back (ns3::Simulator::Schedule(ns3::MilliSeconds(time-now), &Producer::OnInterest, this, interest));
  }
  else
  {
    //fprintf(stderr, "Scheduled: NOW  (now = %llu, time = %llu)\n", now, time);
    Producer::OnInterest (interest); // we are late schedule immediately
  }
}

} // namespace ndn
} // namespace ns3
