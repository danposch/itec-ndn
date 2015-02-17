#include "limiter.h"

using namespace nfd;
using namespace nfd::fw;

Limiter::Limiter(double maxTokens)
{
  this->maxTokens = maxTokens;
  tokens = 0.0;
  addTokens(maxTokens * INITIAL_TOKENS);
}

Limiter::~Limiter ()
{
}

double Limiter::addTokens (double tokens)
{
  double ret = 0.0;
  this->tokens += tokens;

  if(this->tokens > maxTokens)
  {
    ret = this->tokens - maxTokens;
    this->tokens = maxTokens;
  }
  return ret;
}

bool Limiter::tryConsumeToken()
{
  if(tokens >= 1)
  {
    tokens-=1;
    return true;
  }
  return false;
}

bool Limiter::isFull ()
{
  if(tokens >= maxTokens)
    return true;

  return false;
}

void Limiter::setNewMaxTokenSize(double maxTokens)
{
  this->maxTokens = maxTokens;

  if(tokens > maxTokens)
    tokens = maxTokens;
}
