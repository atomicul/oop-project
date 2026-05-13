#pragma once

#include "network.h"
#include "phone_cdc.h"
#include "reverse_phone.h"
#include "server.h"
#include "shout_phone.h"
#include "whisper_phone.h"

#include <format>
#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>

class ClientConnection final : public TcpConnectionBase {
  private:
    std::shared_ptr<Network> network;
    std::optional<std::pair<std::string, Network::ListenerToken>> attached_phone{};

  public:
    ClientConnection(UniqueSocketFd fd, std::shared_ptr<Network> network)
        : TcpConnectionBase(std::move(fd)), network(std::move(network)) {}

    ~ClientConnection() override {
        close();
        wait();
        if (attached_phone) {
            network->removeListener(attached_phone->second);
        }
    }

  protected:
    void receive(std::string data) override {
        if (attached_phone) {
            handleAttached(std::move(data));
            return;
        }

        std::stringstream ss{data};
        std::string command;
        ss >> command;

        try {
            send(commands.at(command)(ss));
        } catch (const std::out_of_range &) {
            send(std::format("Error: Command must be one of: {}\n", commands | std::views::keys));
        }
    }

  private:
    void handleAttached(std::string data) {
        while (!data.empty() && (data.back() == '\n' || data.back() == '\r')) {
            data.pop_back();
        }

        if (data.empty()) {
            const std::string name = attached_phone->first;
            network->removeListener(attached_phone->second);
            attached_phone.reset();
            send(std::format("Detached from \"{}\".\n", name));
            return;
        }

        const std::string current = attached_phone->first;
        const bool ok =
            network->withPhone(current, [&](Phone &p) { p.sendMessage(std::move(data)); });
        if (!ok) {
            send(std::format("Error: Phone \"{}\" no longer exists\n", current));
        }
    }

    std::unordered_map<std::string, std::function<std::string(std::istream &is)>> commands{
        {"create",
         [this](std::istream &is) {
             const std::unordered_map<std::string,
                                      std::function<std::unique_ptr<Phone>(std::string)>>
                 types{
                     {"shout",
                      [](std::string name) {
                          return std::make_unique<ShoutPhone>(std::move(name));
                      }},
                     {"whisper",
                      [](std::string name) {
                          return std::make_unique<WhisperPhone>(std::move(name));
                      }},
                     {"reverse",
                      [](std::string name) {
                          return std::make_unique<ReversePhone>(std::move(name));
                      }},
                     {"plain",
                      [](std::string name) { return std::make_unique<Phone>(std::move(name)); }},
                 };

             std::string name;
             is >> name;

             if (name.empty()) {
                 return std::string{"USAGE: create <name> [<type>]\n"};
             }

             std::string type;
             is >> type;

             std::unique_ptr<Phone> phone{};
             if (type.empty()) {
                 phone = std::make_unique<Phone>(std::move(name));
             } else {
                 try {
                     phone = types.at(type)(std::move(name));
                 } catch (const std::out_of_range &) {
                     return std::format("Error: Key must be one of {}\n", types | std::views::keys);
                 }
             }

             network->addPhone(std::move(phone));
             return std::string{"OK\n"};
         }},
        {"subscribe",
         [this](std::istream &is) -> std::string {
             std::string phone_name;
             std::string subscriber_name;
             is >> phone_name >> subscriber_name;

             if (phone_name.empty() || subscriber_name.empty()) {
                 return std::string{"USAGE: subscribe <phone> <subscriber>\n"};
             }
             if (phone_name == subscriber_name) {
                 return std::string{"Error: A phone cannot subscribe to itself\n"};
             }

             const bool ok = network->withPhone(phone_name, subscriber_name,
                                                [](Phone &p, Phone &s) { p.addSubscriber(&s); });
             if (!ok) {
                 return std::format("Error: Phone \"{}\" or \"{}\" not found\n", phone_name,
                                    subscriber_name);
             }
             return std::string{"OK\n"};
         }},
        {"attach", [this](std::istream &is) -> std::string {
             std::string name;
             is >> name;
             if (name.empty()) {
                 return std::string{"USAGE: attach <phone>\n"};
             }
             if (attached_phone) {
                 return std::format("Error: Already attached to \"{}\"\n", attached_phone->first);
             }

             const bool exists = network->withPhone(name, [](Phone &) {});
             if (!exists) {
                 return std::format("Error: Phone \"{}\" not found\n", name);
             }

             auto listener_token = network->onPhoneTransmission(name, [this](const PhoneCDC &cdc) {
                 if (cdc.trace().size() < 2) {
                     return;
                 }

                 send(std::format("{}\n", cdc.beforeMessage()));
             });

             attached_phone = std::pair{name, listener_token};
             return std::format("Attached to \"{}\". Send an empty line to detach.\n", name);
         }}};
};
