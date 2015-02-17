#include "saf.h"

using namespace nfd;
using namespace nfd::fw;

const Name SAF::STRATEGY_NAME("ndn:/localhost/nfd/strategy/saf");

SAF::SAF(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  //fprintf(stderr, "SAF activated\n");
  const FaceTable& ft = getFaceTable();
  int prefixComponets = 0;
  engine = boost::shared_ptr<SAFEngine>(new SAFEngine(ft, prefixComponets));
}

SAF::~SAF()
{
}

void SAF::afterReceiveInterest(const Face& inFace, const Interest& interest ,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  /* WARNING!!! interest != pitEntry->interest*/
  /* interst could be /NACK/suffix, while pitEntry is /suffix*/

  //find + exclue inface(s) and already tried outface(s)
  std::vector<int> originInFaces = getAllInFaces(pitEntry);
  std::vector<int> alreadyTriedFaces = getAllOutFaces(pitEntry);

  std::string prefix = interest.getName().get(0).toUri();
  if(prefix.compare("NACK") == 0)
  {
    //fprintf(stderr, "SAF::NACK: %s\n", interest.getName().toUri().c_str());
    engine->logNack(inFace, pitEntry->getInterest());

    /*std::string n = interest.getName().toUri();
    n = n.substr(5,std::string::npos);
    shared_ptr<Interest> without_nack_prefix = make_shared<Interest>(*(interest.shared_from_this()));
    without_nack_prefix->setName(n);*/

    int nextHop = engine->determineNextHop(pitEntry->getInterest(), originInFaces, alreadyTriedFaces, fibEntry);
    if(nextHop == DROP_FACE_ID)
    {
      rejectPendingInterest(pitEntry);
    }
    else
    {
      if(engine->tryForwardInterest (pitEntry->getInterest(), getFaceTable ().get (nextHop)))
        sendInterest(pitEntry, getFaceTable ().get (nextHop));
      else
        rejectPendingInterest(pitEntry);
    }
    return;
  }

  int nextHop = engine->determineNextHop(interest, originInFaces, alreadyTriedFaces, fibEntry);
  if(nextHop == DROP_FACE_ID)
  {
    rejectPendingInterest(pitEntry);
  }
  else
  {
    if(engine->tryForwardInterest (interest, getFaceTable ().get (nextHop)))
      sendInterest(pitEntry, getFaceTable ().get (nextHop));
    else
    {
      rejectPendingInterest(pitEntry);
    }
  }
}

void SAF::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  engine->logSatisfiedInterest (pitEntry, inFace, data);
  Strategy::beforeSatisfyInterest (pitEntry,inFace, data);
}

void SAF::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{
  //fprintf(stderr, "beforeExpirePendingInterest\n");
  engine->logExpiredInterest(pitEntry);
  Strategy::beforeExpirePendingInterest (pitEntry);
}

std::vector<int> SAF::getAllInFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::InRecordCollection records = pitEntry->getInRecords();

  for(nfd::pit::InRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    if(! (*it).getFace()->isLocal())
      faces.push_back((*it).getFace()->getId());
  }
  return faces;
}

std::vector<int> SAF::getAllOutFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();

  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
    faces.push_back((*it).getFace()->getId());

  return faces;
}

/*void SAF::sendInterest(shared_ptr<pit::Entry> pitEntry, shared_ptr<Face> outFace, bool wantNewNonce = false)
{

}

void SAF::rejectPendingInterest(shared_ptr<pit::Entry> pitEntry)
{

}*/

