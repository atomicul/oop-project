#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "network.h"
#include "phone.h"
#include "phone_cdc.h"
#include "shout_phone.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

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

TEST_CASE("network withPhone returns false for missing phone and skips callback") {
    Network net("TestNet");
    net.addPhone(std::make_unique<Phone>("Alice"));

    bool called = false;
    CHECK(net.withPhone("NonExistent", [&](Phone &) { called = true; }) == false);
    CHECK(called == false);
}

TEST_CASE("network withPhone exposes matching phone exactly once") {
    Network net("TestNet");
    net.addPhone(std::make_unique<Phone>("Alice"));
    net.addPhone(std::make_unique<Phone>("Bob"));

    int call_count = 0;
    std::string seen_name;
    bool ok = net.withPhone("Alice", [&](const Phone &p) {
        ++call_count;
        seen_name = p.name();
    });

    CHECK(ok);
    CHECK(call_count == 1);
    CHECK(seen_name == "Alice");
}

TEST_CASE("network withPhone serializes concurrent access to the same phone") {
    Network net("TestNet");
    net.addPhone(std::make_unique<Phone>("Alice"));

    constexpr int threads = 8;
    constexpr int per_thread = 1000;
    int counter = 0;
    std::atomic<int> ran{0};

    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back([&] {
            for (int j = 0; j < per_thread; ++j) {
                net.withPhone("Alice", [&](Phone &) { ++counter; });
            }
            ++ran;
        });
    }
    for (auto &t : workers) {
        t.join();
    }

    CHECK(ran.load() == threads);
    CHECK(counter == threads * per_thread);
}

TEST_CASE("network onPhoneTransmission without filter sees every phone's CDC") {
    Network net("TestNet");
    auto alice = std::make_unique<Phone>("Alice");
    auto bob = std::make_unique<Phone>("Bob");
    alice->addSubscriber(bob.get());
    net.addPhone(std::move(alice));
    net.addPhone(std::move(bob));

    std::vector<std::string> seen;
    [[maybe_unused]] auto tok = net.onPhoneTransmission(
        [&](const PhoneCDC &cdc) { seen.push_back(cdc.trace().back()->name()); });

    net.withPhone("Alice", [&](Phone &a) { a.sendMessage("hi"); });

    REQUIRE(seen.size() == 2);
    CHECK(seen[0] == "Alice");
    CHECK(seen[1] == "Bob");
}

TEST_CASE("network onPhoneTransmission with name filter only fires for that phone") {
    Network net("TestNet");
    auto alice = std::make_unique<Phone>("Alice");
    auto bob = std::make_unique<Phone>("Bob");
    alice->addSubscriber(bob.get());
    net.addPhone(std::move(alice));
    net.addPhone(std::move(bob));

    int bob_hits = 0;
    int alice_hits = 0;
    [[maybe_unused]] auto tok_bob =
        net.onPhoneTransmission("Bob", [&](const PhoneCDC &) { ++bob_hits; });
    [[maybe_unused]] auto tok_alice =
        net.onPhoneTransmission("Alice", [&](const PhoneCDC &) { ++alice_hits; });

    net.withPhone("Alice", [&](Phone &a) { a.sendMessage("hi"); });

    CHECK(alice_hits == 1);
    CHECK(bob_hits == 1);
}

TEST_CASE("network removeListener stops the callback from firing") {
    Network net("TestNet");
    net.addPhone(std::make_unique<Phone>("Alice"));

    int hits = 0;
    const auto tok = net.onPhoneTransmission([&](const PhoneCDC &) { ++hits; });

    net.withPhone("Alice", [](Phone &a) { a.sendMessage("once"); });
    CHECK(hits == 1);

    CHECK(net.removeListener(tok));

    net.withPhone("Alice", [](Phone &a) { a.sendMessage("twice"); });
    CHECK(hits == 1);
}

TEST_CASE("network removeListener affects only the matching listener (filtered overload)") {
    Network net("TestNet");
    net.addPhone(std::make_unique<Phone>("Alice"));
    net.addPhone(std::make_unique<Phone>("Bob"));

    int alice_hits = 0;
    int bob_hits = 0;
    const auto tok_alice =
        net.onPhoneTransmission("Alice", [&](const PhoneCDC &) { ++alice_hits; });
    [[maybe_unused]] auto tok_bob =
        net.onPhoneTransmission("Bob", [&](const PhoneCDC &) { ++bob_hits; });

    CHECK(net.removeListener(tok_alice));

    net.withPhone("Alice", [](Phone &a) { a.sendMessage("hi"); });
    net.withPhone("Bob", [](Phone &b) { b.sendMessage("hi"); });

    CHECK(alice_hits == 0);
    CHECK(bob_hits == 1);
}

TEST_CASE("network removeListener returns false for an unknown token") {
    Network net("TestNet");
    CHECK_FALSE(net.removeListener(0));
    CHECK_FALSE(net.removeListener(424242));
}

TEST_CASE("network removeListener: removing twice fails the second time") {
    Network net("TestNet");
    const auto tok = net.onPhoneTransmission([](const PhoneCDC &) {});

    CHECK(net.removeListener(tok));
    CHECK_FALSE(net.removeListener(tok));
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
