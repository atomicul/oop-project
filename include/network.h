#ifndef NETWORK_H
#define NETWORK_H 1

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class Phone;

class Network {
    std::string name{};
    std::vector<std::unique_ptr<Phone>> phones{};

  public:
    explicit Network(std::string name);

    Phone &addPhone(std::unique_ptr<Phone> phone);
    [[nodiscard]] Phone *findPhone(const std::string &phone_name) const;
    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] auto begin() { return phones.begin(); }
    [[nodiscard]] auto end() { return phones.end(); }
    [[nodiscard]] auto begin() const { return phones.cbegin(); }
    [[nodiscard]] auto end() const { return phones.cend(); }

    friend std::ostream &operator<<(std::ostream &os, const Network &network);
};

#endif
