#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <limits>

#ifdef _WIN32
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace ConsoleUI {

enum class Color { Default, Red, Green, Yellow, Cyan, White, Gray };

inline void setColor(Color c) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD attr;
    switch (c) {
        case Color::Red:    attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case Color::Green:  attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case Color::Yellow: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case Color::Cyan:   attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case Color::White:  attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case Color::Gray:   attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        default:            attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    SetConsoleTextAttribute(h, attr);
#else
    switch (c) {
        case Color::Red:    std::cout << "\033[1;31m"; break;
        case Color::Green:  std::cout << "\033[1;32m"; break;
        case Color::Yellow: std::cout << "\033[1;33m"; break;
        case Color::Cyan:   std::cout << "\033[1;36m"; break;
        case Color::White:  std::cout << "\033[1;37m"; break;
        case Color::Gray:   std::cout << "\033[0;37m"; break;
        default:            std::cout << "\033[0m";    break;
    }
#endif
}

inline void resetColor() { setColor(Color::Default); }

inline void printColored(const std::string& text, Color c) {
    setColor(c); std::cout << text; resetColor();
}

inline void printLine(char ch = '-', int width = 60) {
    setColor(Color::Gray);
    std::cout << std::string(width, ch) << '\n';
    resetColor();
}

inline void printHeader(const std::string& title) {
    std::cout << '\n';
    printLine('=');
    setColor(Color::Cyan);
    int padding = (60 - static_cast<int>(title.size())) / 2;
    std::cout << std::string(std::max(0, padding), ' ') << title << '\n';
    resetColor();
    printLine('=');
}

inline void printSubHeader(const std::string& title) {
    std::cout << '\n';
    printLine('-');
    setColor(Color::Yellow);
    std::cout << "  " << title << '\n';
    resetColor();
    printLine('-');
}

inline void printSuccess(const std::string& msg) {
    printColored("  [OK] " + msg + "\n", Color::Green);
}

inline void printError(const std::string& msg) {
    printColored("  [ERROR] " + msg + "\n", Color::Red);
}

inline void printInfo(const std::string& msg) {
    printColored("  " + msg + "\n", Color::Gray);
}

inline void printEmpty() {
    printColored("  (데이터 없음)\n", Color::Gray);
}

inline int getIntInput(const std::string& prompt) {
    int val;
    while (true) {
        setColor(Color::White);
        std::cout << prompt;
        resetColor();
        if (std::cin >> val) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        printError("숫자를 입력해주세요.");
    }
}

inline std::string getStringInput(const std::string& prompt) {
    std::string val;
    setColor(Color::White);
    std::cout << prompt;
    resetColor();
    std::getline(std::cin, val);
    return val;
}

inline void pressEnterToContinue() {
    printColored("\n  Enter를 눌러 계속...", Color::Gray);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

inline void printTable(const std::vector<std::string>& headers,
                       const std::vector<std::vector<std::string>>& rows,
                       int colWidth = 15) {
    setColor(Color::Cyan);
    std::cout << "  ";
    for (const auto& h : headers)
        std::cout << std::left << std::setw(colWidth) << h;
    std::cout << '\n';
    resetColor();
    printLine('-');
    if (rows.empty()) { printEmpty(); return; }
    bool even = false;
    for (const auto& row : rows) {
        setColor(even ? Color::White : Color::Gray);
        std::cout << "  ";
        for (size_t i = 0; i < headers.size(); ++i) {
            std::string cell = (i < row.size()) ? row[i] : "";
            if (cell.size() > static_cast<size_t>(colWidth - 1))
                cell = cell.substr(0, colWidth - 4) + "...";
            std::cout << std::left << std::setw(colWidth) << cell;
        }
        std::cout << '\n';
        even = !even;
    }
    resetColor();
}

inline void printMenuItem(int num, const std::string& label, bool isExit = false) {
    if (isExit) {
        setColor(Color::Red);
        std::cout << "  [" << num << "] " << label << '\n';
    } else {
        setColor(Color::Yellow);
        std::cout << "  [" << num << "] ";
        resetColor();
        std::cout << label << '\n';
    }
    resetColor();
}

} // namespace ConsoleUI
