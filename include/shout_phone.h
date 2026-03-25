#ifndef SHOUT_PHONE_H
#define SHOUT_PHONE_H 1

#include "phone.h"

class ShoutPhone : public Phone {
  public:
    using Phone::Phone;

  protected:
    void transform_message(std::string &message) override;
};

#endif
