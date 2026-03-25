#include "command_interpreter.h"
#include "tui.h"

#include <iostream>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

int main() {
    if (isatty(fileno(stdin)) != 0) {
        Tui tui;
        tui.run();
        return 0;
    }

    CommandInterpreter interpreter(std::cout);
    interpreter.run(std::cin);
}
