#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "network.h"
#include "phone.h"
#include "phone_cdc.h"
#include "shout_phone.h"

#include <memory>
#include <stdexcept>

TEST_CASE("cycle detection throws") {
    Phone alice("Alice");
    Phone bob("Bob");

    alice.addSubscriber(&bob);
    bob.addSubscriber(&alice);

    CHECK_THROWS_AS(alice.sendMessage("hi"), std::runtime_error);
}

TEST_CASE("send message with no subscribers completes") {
    Phone alice("Alice");
    CHECK_NOTHROW(alice.sendMessage("hello"));
}

TEST_CASE("identity transform: CDC shows before == after") {
    Phone alice("Alice");

    std::string before;
    std::string after;

    alice.onMessageTransmission([&](const PhoneCDC &cdc) {
        before = cdc.beforeMessage();
        after = cdc.afterMessage();
    });

    alice.sendMessage("unchanged");

    CHECK(before == "unchanged");
    CHECK(after == "unchanged");
}

TEST_CASE("shout phone transforms to uppercase") {
    ShoutPhone shout("Shout");

    std::string after;
    shout.onMessageTransmission([&](const PhoneCDC &cdc) { after = cdc.afterMessage(); });

    shout.sendMessage("hello");

    CHECK(after == "HELLO");
}

TEST_CASE("network findPhone") {
    Network net("TestNet");
    net.addPhone(std::make_unique<Phone>("Alice"));
    net.addPhone(std::make_unique<Phone>("Bob"));

    CHECK(net.findPhone("Alice") != nullptr);
    CHECK(net.findPhone("Alice")->name() == "Alice");
    CHECK(net.findPhone("NonExistent") == nullptr);
}

TEST_CASE("network size") {
    Network net("TestNet");
    CHECK(net.size() == 0);

    net.addPhone(std::make_unique<Phone>("Alice"));
    CHECK(net.size() == 1);

    net.addPhone(std::make_unique<Phone>("Bob"));
    CHECK(net.size() == 2);
}

TEST_CASE("CDC trace grows with propagation depth") {
    Phone alice("Alice");
    Phone bob("Bob");
    Phone charlie("Charlie");

    alice.addSubscriber(&bob);
    bob.addSubscriber(&charlie);

    std::vector<std::size_t> trace_sizes;

    alice.onMessageTransmission(
        [&](const PhoneCDC &cdc) { trace_sizes.push_back(cdc.trace().size()); });
    bob.onMessageTransmission(
        [&](const PhoneCDC &cdc) { trace_sizes.push_back(cdc.trace().size()); });
    charlie.onMessageTransmission(
        [&](const PhoneCDC &cdc) { trace_sizes.push_back(cdc.trace().size()); });

    alice.sendMessage("test");

    REQUIRE(trace_sizes.size() == 3);
    CHECK(trace_sizes[0] == 1);
    CHECK(trace_sizes[1] == 2);
    CHECK(trace_sizes[2] == 3);
}
