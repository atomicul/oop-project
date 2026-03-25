#include "command_interpreter.h"
#include "network.h"
#include "phone.h"
#include "reverse_phone.h"
#include "shout_phone.h"
#include "whisper_phone.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

CommandInterpreter::CommandInterpreter(std::ostream &out) : out(out) {}

CommandInterpreter::~CommandInterpreter() = default;

void CommandInterpreter::run(std::istream &in) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "create_network") {
            std::string name;
            iss >> name;
            network = std::make_unique<Network>(std::move(name));
            transmission_log.clear();
        } else if (command == "add_phone") {
            handleAddPhone(iss);
        } else if (command == "subscribe") {
            handleSubscribe(iss);
        } else if (command == "send") {
            handleSend(line);
        } else if (command == "print") {
            handlePrint();
        } else if (command == "log") {
            handleLog();
        } else if (command == "exit") {
            break;
        } else {
            out << "Unknown command: " << command << '\n';
        }
    }
}

void CommandInterpreter::handleAddPhone(std::istringstream &args) {
    if (!network) {
        out << "No network created\n";
        return;
    }

    std::string name;
    std::string type;
    args >> name >> type;

    std::unique_ptr<Phone> phone;
    if (type == "ShoutPhone") {
        phone = std::make_unique<ShoutPhone>(name);
    } else if (type == "WhisperPhone") {
        phone = std::make_unique<WhisperPhone>(name);
    } else if (type == "ReversePhone") {
        phone = std::make_unique<ReversePhone>(name);
    } else {
        phone = std::make_unique<Phone>(name);
    }

    auto &ref = network->addPhone(std::move(phone));
    registerCallbacks(ref);
}

void CommandInterpreter::handleSubscribe(std::istringstream &args) {
    if (!network) {
        out << "No network created\n";
        return;
    }

    std::string source_name;
    std::string target_name;
    args >> source_name >> target_name;

    Phone *source = network->findPhone(source_name);
    Phone *target = network->findPhone(target_name);

    if (source == nullptr || target == nullptr) {
        out << "Phone not found\n";
        return;
    }

    source->addSubscriber(target);
}

void CommandInterpreter::handleSend(const std::string &line) {
    if (!network) {
        out << "No network created\n";
        return;
    }

    std::istringstream iss(line);
    std::string skip;
    std::string phone_name;
    iss >> skip >> phone_name;

    std::string message;
    std::getline(iss >> std::ws, message);

    Phone *phone = network->findPhone(phone_name);
    if (phone == nullptr) {
        out << "Phone not found: " << phone_name << '\n';
        return;
    }

    try {
        phone->sendMessage(std::move(message));
    } catch (const std::runtime_error &e) {
        out << "Error: " << e.what() << '\n';
    }
}

void CommandInterpreter::handlePrint() {
    if (!network) {
        out << "No network created\n";
        return;
    }
    out << *network << '\n';
}

void CommandInterpreter::handleLog() {
    if (transmission_log.empty()) {
        out << "No transmissions recorded\n";
        return;
    }
    for (const auto &cdc : transmission_log) {
        out << cdc << '\n';
    }
}

void CommandInterpreter::registerCallbacks(Phone &phone) {
    phone.onMessageTransmission([this](const PhoneCDC &cdc) { transmission_log.push_back(cdc); });
}
