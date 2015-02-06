#include "saf.h"

//NFD_LOG_INIT("SAF");

using namespace nfd;
using namespace nfd::itec;

const Name SAF::STRATEGY_NAME("ndn:/localhost/nfd/strategy/saf");

SAF::SAF(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  fprintf(stderr, "SAF activated\n");
}

SAF::~SAF()
{
}

void SAF::afterReceiveInterest(const Face& inFace, const Interest& interest,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  fprintf(stderr, "received: %s\n", interest.getName ().toUri ().c_str ());
}

void SAF::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
}

void SAF::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{
}

