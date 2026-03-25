#include "shout_phone.h"
#include <algorithm>

void ShoutPhone::transform_message(std::string &message) {
    std::ranges::transform(message, message.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
}
