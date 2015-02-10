#ifndef SAF_H
#define SAF_H

#include "face/face.hpp"
#include "fw/strategy.hpp"

#include "boost/shared_ptr.hpp"
#include "safengine.h"

namespace nfd
{
namespace fw
{

class SAF : public nfd::fw::Strategy
{
public:
  SAF(Forwarder &forwarder, const Name &name = STRATEGY_NAME);

  virtual ~SAF();
  virtual void afterReceiveInterest(const nfd::Face& inFace, const ndn::Interest& interest,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry);
  virtual void beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const nfd::Face& inFace, const ndn::Data& data);
  virtual void beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry);

  /*virtual void sendInterest(shared_ptr<pit::Entry> pitEntry, shared_ptr<Face> outFace, bool wantNewNonce = false);
  virtual void rejectPendingInterest(shared_ptr<pit::Entry> pitEntry);*/

  static const Name STRATEGY_NAME;

protected:

  std::vector<int> getAllInFaces(shared_ptr<pit::Entry> pitEntry);
  std::vector<int> getAllOutFaces(shared_ptr<pit::Entry> pitEntry);

  boost::shared_ptr<SAFEngine> engine;
};

}
}
#endif // SAF_H
