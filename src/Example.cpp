#include "../include/Example.h"
#include <iostream>
// This also works if you do not want `../`, but some editors might not like it
// #include "Example.h"

void Example::f() const { std::cout << "private function f: " << x << "\n"; }

void Example::g() {
    ++y;
    f();
    std::cout << "public function g: " << y << "\n";
}
