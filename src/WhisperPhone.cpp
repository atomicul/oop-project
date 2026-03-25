#include "whisper_phone.h"
#include <algorithm>

void WhisperPhone::transform_message(std::string &message) {
    std::ranges::transform(message, message.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}
