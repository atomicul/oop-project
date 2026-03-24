#include "network.h"
#include "phone.h"

#include <algorithm>
#include <ranges>
#include <sstream>

namespace vw = std::ranges::views;

Network::Network(std::string name) : name(std::move(name)) {}

Phone &Network::addPhone(std::unique_ptr<Phone> phone) {
    phones.push_back(std::move(phone));
    return *phones.back();
}

Phone *Network::findPhone(const std::string &phone_name) const {
    auto it = std::ranges::find_if(phones, [&](const auto &p) { return p->name() == phone_name; });
    if (it == phones.end()) {
        return nullptr;
    }
    return it->get();
}

std::size_t Network::size() const { return phones.size(); }

std::ostream &operator<<(std::ostream &os, const Network &network) {
    std::stringstream phones_csv{};
    if (!network.phones.empty()) {
        for (const auto &p : network.phones | vw::take(network.phones.size() - 1)) {
            phones_csv << *p << ", ";
        }
        phones_csv << *network.phones.back();
    }

    return os << "Network { " << "Name=" << network.name << ", " << "Phones=[" << phones_csv.str()
              << "] }";
}
