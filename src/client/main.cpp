#include <iostream>

#include "clientConsole.h"

int main(int argc, const char** argv) {
    StdIoConsoleStream stream;
    ClientConsole console(stream);

    while (console.IsShouldExit() == false) {
        console.HandleCommand();
    }

    return 0;
}
