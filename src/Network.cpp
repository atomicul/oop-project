#include "network.h"
#include "phone.h"
#include "phone_cdc.h"

#include <algorithm>
#include <mutex>
#include <ranges>
#include <sstream>

namespace vw = std::ranges::views;

Network::Network(std::string name) : name(std::move(name)) {}

void Network::addPhone(std::unique_ptr<Phone> phone) {
    const std::lock_guard lock(mtx);
    phone->onMessageTransmission([this](const PhoneCDC &cdc) {
        const auto &current = cdc.trace().back()->name();
        for (const auto &l : listeners) {
            if (l.filter_name.empty() || l.filter_name == current) {
                l.cb(cdc);
            }
        }
    });
    phones.push_back(std::move(phone));
}

std::size_t Network::size() const {
    const std::lock_guard lock(mtx);
    return phones.size();
}

Phone *Network::findPhoneUnlocked(std::string_view phone_name) {
    auto it = std::ranges::find_if(phones, [&](const auto &p) { return p->name() == phone_name; });
    return it == phones.end() ? nullptr : it->get();
}

Network::ListenerToken Network::onPhoneTransmission(std::function<void(const PhoneCDC &)> cb) {
    return onPhoneTransmission({}, std::move(cb));
}

Network::ListenerToken Network::onPhoneTransmission(std::string filter_name,
                                                    std::function<void(const PhoneCDC &)> cb) {
    const std::lock_guard lock(mtx);
    const ListenerToken token = next_token++;
    listeners.push_back({token, std::move(filter_name), std::move(cb)});
    return token;
}

bool Network::removeListener(ListenerToken token) {
    const std::lock_guard lock(mtx);
    auto it = std::ranges::find_if(listeners, [&](const auto &l) { return l.token == token; });
    if (it == listeners.end()) {
        return false;
    }
    listeners.erase(it);
    return true;
}

std::ostream &operator<<(std::ostream &os, const Network &network) {
    const std::lock_guard lock(network.mtx);
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
