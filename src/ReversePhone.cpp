#include "reverse_phone.h"
#include <algorithm>

void ReversePhone::transform_message(std::string &message) { std::ranges::reverse(message); }
