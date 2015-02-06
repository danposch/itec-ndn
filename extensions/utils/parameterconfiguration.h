#ifndef PARAMETERCONFIGURATION_H
#define PARAMETERCONFIGURATION_H

#include <cstddef>
#include <map>
#include <string>

//default values can be overriden:

#define P_ALPHA 0.35

#define P_X_DROPPING 0.4

#define P_PROBING_TRAFFIC 0.30  //test 0.30
#define P_SHIFT_THRESHOLD 0.05  //before 0.1
#define P_SHIFT_TRAFFIC 0.35    //test 0.40

#define P_UPDATE_INTERVALL 0.5
#define P_MAX_LAYERS 1
#define P_DROP_FACE_ID -1

#define P_RELIABILITY_THRESHOLD 0.8 // before 0.7

namespace ns3
{
namespace ndn
{

class ParameterConfiguration
{
public:
  static ParameterConfiguration* getInstance();

  void setParameter(std::string param_name, double value);

  double getParameter(std::string para_name);

protected:  
  ParameterConfiguration();

  static ParameterConfiguration* instance;

  std::map<
  std::string /*param name*/,
  double /*param value*/
  > typedef ParameterMap;

  ParameterMap pmap;
};

}
}

#endif // PARAMETERCONFIGURATION_H
