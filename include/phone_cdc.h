#ifndef PHONE_CDC_H
#define PHONE_CDC_H 1

#include <iostream>
#include <string>
#include <vector>

class Phone;

class PhoneCDC final {
    std::string before_message{};
    std::string after_message{};
    std::vector<Phone *> _trace{};

  public:
    PhoneCDC(std::string before_message, std::string after_message, std::vector<Phone *> trace);

    PhoneCDC(const PhoneCDC &other);
    PhoneCDC &operator=(PhoneCDC other);
    ~PhoneCDC();

    [[nodiscard]] const std::string &beforeMessage() const;
    [[nodiscard]] const std::string &afterMessage() const;
    [[nodiscard]] const std::vector<Phone *> &trace() const;

    void setBeforeMessage(std::string msg);
    void setAfterMessage(std::string msg);
    void setTrace(std::vector<Phone *> new_trace);

    friend std::ostream &operator<<(std::ostream &os, const PhoneCDC &cdc);
    friend void swap(PhoneCDC &lhs, PhoneCDC &rhs);
};

#endif
