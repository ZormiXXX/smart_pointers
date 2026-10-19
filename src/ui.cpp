#include "ui.hpp"

#include "benchmark.hpp"
#include "functional_tests.hpp"
#include "smart_pointers.hpp"

#include <iostream>
#include <limits>
#include <utility>

namespace {

void show_header() {
    std::cout << "\n"
                 "====================================================\n"
                 "        Лабораторная работа: умные указатели\n"
                 "       UnqPtr · ShrdPtr · MemorySpan · MsPtr\n"
                 "====================================================\n";
}

void show_menu() {
    std::cout << "\n"
                 "  Демонстрации\n"
                 "  1  Уникальное владение       (UnqPtr)\n"
                 "  2  Совместное владение       (ShrdPtr)\n"
                 "  3  Контейнер и арифметика    (MemorySpan / MsPtr)\n"
                 "\n"
                 "  Проверка\n"
                 "  4  Функциональные тесты\n"
                 "  5  Замер времени и памяти\n"
                 "  0  Выход\n"
                 "\n"
                 "Ваш выбор: ";
}

void demonstrate_unique() {
    auto source = make_unq<int>(73);
    std::cout << "Создан UnqPtr, значение: " << *source << '\n';
    auto destination = std::move(source);
    std::cout << "После перемещения источник " << (source ? "не пуст" : "пуст")
              << ", значение нового владельца: " << *destination << '\n';
    std::cout << "Объект удалится автоматически при выходе владельца из области видимости.\n";
}

void demonstrate_shared() {
    auto owner = make_unq<int>(128);
    ShrdPtr<int> first(std::move(owner));
    std::cout << "Владение передано первому дескриптору.\n"
              << "Исходный UnqPtr: " << (owner ? "не пуст" : "пуст")
              << "; число дескрипторов: " << first.use_count() << '\n';
    {
        auto second = first;
        std::cout << "Создана копия: дескрипторов " << first.use_count()
                  << "; значение объекта: " << *second << '\n';
    }
    std::cout << "После уничтожения копии дескрипторов осталось: " << first.use_count() << '\n';
}

void demonstrate_memory_span() {
    MemorySpan<int> values(4);
    for (std::size_t i = 0; i < values.size(); ++i)
        values.at(i) = static_cast<int>((i + 1) * 10);

    auto first = values.Locate(0);
    auto third = first + 2;
    std::cout << "MsPtr указывает на значения " << *first << " и " << *third
              << "; расстояние между ними: " << third - first << '\n';

    try {
        (void)(third + 2);
    } catch (const InvalidState &error) {
        std::cout << "Выход за границы запрещён: " << error.what() << '\n';
    }
}

} // namespace

void run_ui() {
    show_header();
    while (true) {
        show_menu();
        int option = -1;
        if (!(std::cin >> option)) {
            if (std::cin.eof())
                break;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Введите номер пункта меню.\n";
            continue;
        }

        switch (option) {
        case 1:
            demonstrate_unique();
            break;
        case 2:
            demonstrate_shared();
            break;
        case 3:
            demonstrate_memory_span();
            break;
        case 4:
            run_functional_tests();
            break;
        case 5:
            run_benchmark();
            break;
        case 0:
            std::cout << "До встречи!\n";
            return;
        default:
            std::cout << "Такого пункта нет. Выберите номер из меню.\n";
        }
    }
}
