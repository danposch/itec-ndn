#ifndef SAF_H
#define SAF_H

#include "face/face.hpp"
#include "fw/strategy.hpp"

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
};

}
}
#endif // SAF_H
