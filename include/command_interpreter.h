#ifndef COMMAND_INTERPRETER_H
#define COMMAND_INTERPRETER_H 1

#include "phone_cdc.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class Network;

class CommandInterpreter {
    std::unique_ptr<Network> network{};
    std::vector<PhoneCDC> transmission_log{};
    std::ostream &out;

  public:
    explicit CommandInterpreter(std::ostream &out);
    ~CommandInterpreter();
    void run(std::istream &in);

  private:
    void handleAddPhone(std::istringstream &args);
    void handleSubscribe(std::istringstream &args);
    void handleSend(const std::string &line);
    void handlePrint();
    void handleLog();
    void registerCallbacks(Phone &phone);
};

#endif
