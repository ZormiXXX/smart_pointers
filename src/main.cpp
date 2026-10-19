#include "benchmark.hpp"
#include "functional_tests.hpp"
#include "ui.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        run_ui();
        return 0;
    }

    const std::string_view mode(argv[1]);
    if (mode == "--test") {
        run_functional_tests();
        return 0;
    }
    if (mode == "--benchmark") {
        run_benchmark();
        return 0;
    }
    if (mode == "--help") {
        std::cout << "Использование: hw1 [--test|--benchmark|--help]\n";
        return 0;
    }

    std::cerr << "Неизвестный параметр: " << mode << '\n';
    return 2;
}
