#ifndef PHONE_H
#define PHONE_H 1

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "phone_cdc.h"

class Phone {
    std::string _name{};
    std::vector<Phone *> subscribers{};
    std::vector<std::function<void(const PhoneCDC &)>> on_transmission_callbacks{};

  public:
    explicit Phone(std::string name, std::vector<Phone *> subscribers = {});

    [[nodiscard]] const std::string &name() const;

    void addSubscriber(Phone *p);
    void sendMessage(std::string message);
    void onMessageTransmission(std::function<void(const PhoneCDC &)> callback);

    friend std::ostream &operator<<(std::ostream &os, const Phone &phone);

    virtual ~Phone() = default;

  protected:
    virtual void transform_message(std::string &message);

  private:
    void sendMessage(std::string message, std::vector<Phone *> &trace);
};

#endif
