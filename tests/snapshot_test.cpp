#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "snapshot.h"

#include "network.h"
#include "phone.h"
#include "reverse_phone.h"
#include "shout_phone.h"
#include "whisper_phone.h"

#include <memory>
#include <sstream>

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

    snapshot::check_string(collected.str(), "chain_shout_whisper_reverse");
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

    snapshot::check_string(collected.str(), "fanout_shout_to_whisper_and_reverse");
}

TEST_CASE("network with mixed phones and propagation") {
    Network net("TestNet");

    auto &alice = net.addPhone(std::make_unique<ShoutPhone>("Alice"));
    auto &bob = net.addPhone(std::make_unique<WhisperPhone>("Bob"));
    auto &charlie = net.addPhone(std::make_unique<ReversePhone>("Charlie"));
    auto &dave = net.addPhone(std::make_unique<Phone>("Dave"));

    alice.addSubscriber(&bob);
    bob.addSubscriber(&charlie);
    charlie.addSubscriber(&dave);

    snapshot::check(net, "network_mixed_phones");

    std::ostringstream collected;

    alice.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    bob.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    charlie.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });
    dave.onMessageTransmission([&](const PhoneCDC &cdc) { collected << cdc << '\n'; });

    alice.sendMessage("Snapshot Test");

    snapshot::check_string(collected.str(), "network_propagation");
}
