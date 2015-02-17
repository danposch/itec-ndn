#ifndef LIMITER_H
#define LIMITER_H

#define INITIAL_TOKENS 0.25

namespace nfd
{
namespace fw
{
class Limiter
{
public:
  Limiter(double maxTokens = INITIAL_TOKENS);
  ~Limiter();

  virtual double addTokens(double tokens);
  virtual bool tryConsumeToken();
  virtual bool isFull();
  virtual void setNewMaxTokenSize(double maxTokens);

protected:

  //all tokens are in interests a X bytes
  double tokens;
  double maxTokens;

};

}
}
#endif // LIMITER_H
