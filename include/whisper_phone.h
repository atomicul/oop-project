#ifndef WHISPER_PHONE_H
#define WHISPER_PHONE_H 1

#include "phone.h"

class WhisperPhone : public Phone {
  public:
    using Phone::Phone;

  protected:
    void transform_message(std::string &message) override;
};

#endif
