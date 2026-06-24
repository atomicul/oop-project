#include "phone.h"

#include <algorithm>
#include <format>
#include <ranges>
#include <stdexcept>

namespace vw = std::ranges::views;

Phone::Phone(std::string name, std::unordered_set<Phone *> subscribers)
    : _name(std::move(name)), subscribers(std::move(subscribers)) {}

const std::string &Phone::name() const { return _name; }

void Phone::addSubscriber(Phone *p) { subscribers.insert(p); }

void Phone::sendMessage(std::string message) {
    std::vector<Phone *> trace{};

    this->sendMessage(std::move(message), trace);
}

void Phone::sendMessage(std::string message, std::vector<Phone *> &trace) {
    auto it = std::ranges::find(trace, this);

    if (it != trace.end()) {
        throw std::runtime_error("Detected cycle within phone graph");
    }

    trace.push_back(this);

    if (on_transmission_callbacks.empty()) {
        transform_message(message);
    } else {
        std::string before = message;
        transform_message(message);

        PhoneCDC cdc{before, message, trace};
        for (const auto &cb : on_transmission_callbacks) {
            cb(cdc);
        }
    }

    for (Phone *p : subscribers) {
        p->sendMessage(message, trace);
    }
    trace.pop_back();
}

void Phone::transform_message(std::string &) {}

std::ostream &operator<<(std::ostream &os, const Phone &phone) {
    return os << "Phone { " << "Name=\"" << phone._name << "\", " << "Subscribers="
              << std::format("{}", phone.subscribers |
                                       std::views::transform([](const Phone *p) { return p->name(); }))
              << " }";
}
