#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "snapshot.h"

#include "network.h"
#include "phone.h"
#include "reverse_phone.h"
#include "shout_phone.h"
#include "whisper_phone.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
// Propagation order across multiple subscribers is not guaranteed, so sort the
// collected lines before comparing against the snapshot.
std::string sortLines(const std::string &input) {
    std::vector<std::string> lines;
    std::istringstream iss(input);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    std::ranges::sort(lines);
    std::string out;
    for (const auto &l : lines) {
        out += l;
        out += '\n';
    }
    return out;
}
} // namespace

TEST_CASE("chain propagation: shout -> whisper -> reverse") {
    ShoutPhone alice("Alice");
    WhisperPhone bob("Bob");
    ReversePhone charlie("Charlie");

    alice.addSubscriber(&bob);
    bob.addSubscriber(&charlie);

    std::ostringstream collected;

    alice.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    bob.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    charlie.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });

    alice.sendMessage("Hello World");

    snapshot::check_string(sortLines(collected.str()), "chain_shout_whisper_reverse");
}

TEST_CASE("fan-out: shout -> whisper + reverse") {
    ShoutPhone hub("Hub");
    WhisperPhone left("Left");
    ReversePhone right("Right");

    hub.addSubscriber(&left);
    hub.addSubscriber(&right);

    std::ostringstream collected;

    hub.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    left.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    right.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });

    hub.sendMessage("Test Message");

    snapshot::check_string(sortLines(collected.str()), "fanout_shout_to_whisper_and_reverse");
}

TEST_CASE("network with mixed phones and propagation") {
    Network net("TestNet");

    auto alice = std::make_unique<ShoutPhone>("Alice");
    auto bob = std::make_unique<WhisperPhone>("Bob");
    auto charlie = std::make_unique<ReversePhone>("Charlie");
    auto dave = std::make_unique<Phone>("Dave");

    alice->addSubscriber(bob.get());
    bob->addSubscriber(charlie.get());
    charlie->addSubscriber(dave.get());

    net.addPhone(std::move(alice));
    net.addPhone(std::move(bob));
    net.addPhone(std::move(charlie));
    net.addPhone(std::move(dave));

    snapshot::check(net, "network_mixed_phones");

    std::ostringstream collected;
    net.onPhoneTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });

    net.withPhone("Alice", [&](Phone &a) { a.sendMessage("Snapshot Test"); });

    snapshot::check_string(sortLines(collected.str()), "network_propagation");
}
