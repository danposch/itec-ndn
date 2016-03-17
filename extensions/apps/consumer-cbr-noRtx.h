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

#ifndef NDN_ConsumerCbrNoRtx_H
#define NDN_ConsumerCbrNoRtx_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-app.hpp"

#include "ns3/random-variable.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class ConsumerCbrNoRtx : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  ConsumerCbrNoRtx();
  virtual ~ConsumerCbrNoRtx();

  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject);

  /**
   * @brief Actually send packet
   */
  virtual void
  SendPacket();

  /**
   * @brief An event that is fired just before an Interest packet is actually send out (send is
   *inevitable)
   *
   * The reason for "before" even is that in certain cases (when it is possible to satisfy from the
   *local cache),
   * the send call will immediately return data, and if "after" even was used, this after would be
   *called after
   * all processing of incoming data, potentially producing unexpected results.
   */
  virtual void
  WillSendOutInterest(uint32_t sequenceNumber);

protected:
  // from App
  virtual void
  StartApplication();

  virtual void
  StopApplication();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void ScheduleNextPacket();

  /**
     * @brief Set type of frequency randomization
     * @param value Either 'none', 'uniform', or 'exponential'
     */
    void
    SetRandomize(const std::string& value);

    /**
     * @brief Get type of frequency randomization
     * @returns either 'none', 'uniform', or 'exponential'
     */
    std::string
    GetRandomize() const;

protected:

  double m_frequency; // Frequency of interest packets (in hertz)
  bool m_firstTime;
  RandomVariable* m_random;
  std::string m_randomType;

  UniformVariable m_rand; ///< @brief nonce generator

  uint32_t m_seq;      ///< @brief currently requested sequence number
  uint32_t m_seqMax;   ///< @brief maximum number of sequence number
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event

  Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator

  Time m_offTime;          ///< \brief Time interval between packets
  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet

  /// @cond include_hidden

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
  };
  /// @endcond

  /// @cond include_hidden
  class i_seq {
  };
  class i_timestamp {
  };
  /// @endcond

  /// @cond include_hidden
  /**
   * \struct This struct contains a multi-index for the set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

  /// @endcond
};

} // namespace ndn
} // namespace ns3

#endif
