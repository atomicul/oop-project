#ifndef PHONE_H
#define PHONE_H 1

#include <functional>
#include <iosfwd>
#include <string>
#include <unordered_set>

#include "phone_cdc.h"

class Phone {
    std::string _name{};
    std::unordered_set<Phone *> subscribers{};
    std::vector<std::function<void(const PhoneCDC &)>> on_transmission_callbacks{};

  public:
    explicit Phone(std::string name, std::unordered_set<Phone *> subscribers = {});
    Phone(Phone &&other) = delete;
    Phone &operator=(Phone &&other) = delete;
    Phone(const Phone &other) = delete;
    Phone &operator=(const Phone &other) = delete;

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
