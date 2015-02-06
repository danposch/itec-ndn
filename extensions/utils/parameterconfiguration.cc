#include "parameterconfiguration.h"

using namespace ns3::ndn;

ParameterConfiguration* ParameterConfiguration::instance = NULL;

ParameterConfiguration::ParameterConfiguration()
{
  setParameter ("ALPHA", P_ALPHA);
  setParameter ("X_DROPPING", P_X_DROPPING);
  setParameter ("PROBING_TRAFFIC", P_PROBING_TRAFFIC);
  setParameter ("SHIFT_THRESHOLD", P_SHIFT_THRESHOLD);
  setParameter ("SHIFT_TRAFFIC", P_SHIFT_TRAFFIC);
  setParameter ("UPDATE_INTERVALL", P_UPDATE_INTERVALL);
  setParameter ("MAX_LAYERS", P_MAX_LAYERS);
  setParameter ("DROP_FACE_ID", P_DROP_FACE_ID);

  //setParameter ("RELIABILITY_THRESHOLD", P_RELIABILITY_THRESHOLD);

  setParameter ("RELIABILITY_THRESHOLD_MIN", P_RELIABILITY_THRESHOLD);
  setParameter ("RELIABILITY_THRESHOLD_MAX", 1.0);
}


void ParameterConfiguration::setParameter(std::string para_name, double value)
{
  pmap[para_name] = value;
}

double ParameterConfiguration::getParameter(std::string para_name)
{
  return pmap[para_name];
}

ParameterConfiguration *ParameterConfiguration::getInstance()
{
  if(instance == NULL)
    instance = new ParameterConfiguration();

  return instance;
}
