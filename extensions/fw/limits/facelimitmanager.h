#ifndef FACELIMITMANAGER_H
#define FACELIMITMANAGER_H

#define DATA_PACKET_SIZE 4096
#define INTEREST_PACKET_SIZE 50
#define TOKEN_FILL_INTERVALL 10 //ms
#define BUCKET_SIZE 25.0

#include "fw/face-table.hpp"
#include "boost/shared_ptr.hpp"
#include <limits>

#include "ns3-dev/ns3/point-to-point-module.h"
#include "ns3-dev/ns3/network-module.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ndnSIM/model/ndn-net-device-face.hpp"

#include "limiter.h"

namespace nfd
{
namespace fw
{
class FaceLimitManager
{
public:
  FaceLimitManager(shared_ptr< Face > face);

  bool addNewPrefix(std::string content_prefix);
  bool tryForwardInterest(std::string prefix);

protected:

  void newToken();
  std::vector<std::string> getAllNonFullBuckets();

  shared_ptr< Face > face;

  /* map for storing forwarding stats for all faces */
  typedef std::map
    < std::string, /*prefix*/
      boost::shared_ptr<Limiter> /*Limiter*/
    > LimitMap;

  LimitMap bMap;

  uint64_t getPhysicalBitrate(shared_ptr< Face > face);
  double tokenGenRate;
  ns3::EventId newTokenEvent;
};

}
}

#endif // FACELIMITMANAGER_H
