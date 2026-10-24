#include "ui.hpp"

#include "benchmark.hpp"
#include "functional_tests.hpp"
#include "smart_pointers.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

namespace {

bool colors_enabled() {
    return std::getenv("NO_COLOR") == nullptr;
}

const char *style(const char *escape) {
    return colors_enabled() ? escape : "";
}

void show_rule() {
    std::cout << style("\033[90m")
              << "  ────────────────────────────────────────────────────────────────\n"
              << style("\033[0m");
}

void show_header() {
    std::cout << '\n' << style("\033[36m")
              << "  ╭──────────────────────────────────────────────────────────────╮\n"
              << "  │  ЛАБОРАТОРНАЯ РАБОТА  /  УМНЫЕ УКАЗАТЕЛИ                     │\n"
              << "  │  C++20  ·  управление памятью  ·  собственные контейнеры     │\n"
              << "  ╰──────────────────────────────────────────────────────────────╯\n"
              << style("\033[0m");
}

void menu_item(int key, const char *title, const char *description) {
    std::cout << "  │  " << style("\033[33;1m") << '[' << key << ']'
              << style("\033[0m") << "  " << style("\033[1;97m") << title
              << style("\033[0m") << '\n'
              << "  │      " << style("\033[90m") << description
              << style("\033[0m") << '\n';
}

void menu_section(const char *title) {
    std::cout << "  │\n  │  " << style("\033[32;1m") << title
              << style("\033[0m") << '\n';
}

void show_menu() {
    std::cout << '\n' << style("\033[36m")
              << "  ╭──────────────────────────────────────────────────────────────╮\n"
              << style("\033[0m");
    menu_section("ДЕМОНСТРАЦИИ");
    menu_item(1, "UnqPtr", "передача владения и автоматическое удаление");
    menu_item(2, "ShrdPtr", "совместное владение и счётчик ссылок");
    menu_item(3, "MemorySpan", "доступ к элементам и арифметика MsPtr");
    menu_item(4, "DynamicArray", "рост контейнера и перемещение элементов");
    menu_item(5, "Все демо", "показать пункты 1–4 подряд");

    menu_section("ПРОВЕРКИ И ИЗМЕРЕНИЯ");
    menu_item(6, "Функциональные тесты", "результат каждого набора проверок");
    menu_item(7, "Бенчмарк", "время и память для разных способов владения");
    std::cout << "  │\n  │  " << style("\033[90m") << "[0] Выход"
              << style("\033[0m") << "\n"
              << style("\033[36m")
              << "  ╰──────────────────────────────────────────────────────────────╯\n"
              << style("\033[0m") << "\n  Ваш выбор: ";
}

void show_action(const char *category, const char *title) {
    std::cout << '\n' << style("\033[36m") << "  ◆  " << category << "  /  "
              << style("\033[1;97m") << title << style("\033[0m") << '\n';
    show_rule();
}

void demonstrate_unique() {
    show_action("ДЕМО 01", "УНИКАЛЬНОЕ ВЛАДЕНИЕ");
    auto source = make_unq<int>(73);
    std::cout << "  UnqPtr<int> создан, значение: " << *source << '\n';
    auto destination = std::move(source);
    std::cout << "  После перемещения: source " << (source ? "владеет объектом" : "пуст")
              << ", destination хранит " << *destination << ".\n"
              << "  Объект освободится при уничтожении последнего владельца.\n";
}

void demonstrate_shared() {
    show_action("ДЕМО 02", "СОВМЕСТНОЕ ВЛАДЕНИЕ");
    auto owner = make_unq<int>(128);
    ShrdPtr<int> first(std::move(owner));
    std::cout << "\n  Владение передано в ShrdPtr; счётчик: " << first.use_count() << '\n';
    {
        auto second = first;
        std::cout << "  Создали копию: счётчик " << first.use_count()
                  << ", значение через копию: " << *second << '\n';
    }
    std::cout << "  Копия уничтожена; счётчик снова " << first.use_count() << ".\n";
}

void demonstrate_memory_span() {
    show_action("ДЕМО 03", "ДОСТУП ПО MsPtr");
    MemorySpan<int> values(4);
    for (std::size_t i = 0; i < values.size(); ++i)
        values.at(i) = static_cast<int>((i + 1) * 10);

    auto first = values.Locate(0);
    auto third = first + 2;
    std::cout << "\n  Элементы по MsPtr: " << *first << " и " << *third
              << "; расстояние между ними: " << third - first << '\n';
    try {
        (void)(third + 2);
    } catch (const InvalidState &error) {
        std::cout << "  Проверка границы сработала: " << error.what() << '\n';
    }
}

void demonstrate_dynamic_array() {
    show_action("ДЕМО 04", "САМОПИСНЫЙ КОНТЕЙНЕР");
    DynamicArray<int> values;
    for (int value : {3, 6, 9, 12})
        values.Append(value);

    std::cout << "\n  Добавили 4 элемента: ";
    for (int i = 0; i < values.GetSize(); ++i)
        std::cout << values[i] << (i + 1 == values.GetSize() ? "" : " · ");
    std::cout << "\n  Размер: " << values.GetSize() << ", вместимость: "
              << values.GetCapacity() << ". При росте буфером владеет UnqPtr<T[]>.\n";
}

void demonstrate_all() {
    demonstrate_unique();
    demonstrate_shared();
    demonstrate_memory_span();
    demonstrate_dynamic_array();
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
            std::cout << "\n  Введите номер пункта меню.\n";
            continue;
        }

        switch (option) {
        case 1: demonstrate_unique(); break;
        case 2: demonstrate_shared(); break;
        case 3: demonstrate_memory_span(); break;
        case 4: demonstrate_dynamic_array(); break;
        case 5: demonstrate_all(); break;
        case 6: run_functional_tests(); break;
        case 7: run_benchmark(); break;
        case 0:
            std::cout << "\n  " << style("\033[36m") << "Спасибо за просмотр. Всего доброго!"
                      << style("\033[0m") << "\n";
            return;
        default:
            std::cout << "\n  Такого пункта нет — выберите номер из меню.\n";
        }
    }
}
