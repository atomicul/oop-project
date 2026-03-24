#ifndef REVERSE_PHONE_H
#define REVERSE_PHONE_H 1

#include "phone.h"

class ReversePhone : public Phone {
  public:
    using Phone::Phone;

  protected:
    void transform_message(std::string &message) override;
};

#endif
