#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <limits>

#ifdef _WIN32
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

inline bool getYNInput(const std::string& prompt) {
    while (true) {
        std::string ans = getStringInput(prompt + " [Y/N]: ");
        if (ans == "Y" || ans == "y") return true;
        if (ans == "N" || ans == "n") return false;
        printError("Y 또는 N을 입력해주세요.");
    }
}

inline void pressEnterToContinue() {
    printColored("\n  Enter를 눌러 계속...", Color::Gray);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// UTF-8 문자열의 터미널 표시 너비 계산 (한글 등 3바이트 문자 = 2 표시폭)
inline int displayWidth(const std::string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if      (c < 0x80) { i += 1; w += 1; }
        else if (c < 0xE0) { i += 2; w += 1; }
        else if (c < 0xF0) { i += 3; w += 2; } // CJK(한글 포함)
        else               { i += 4; w += 2; }
    }
    return w;
}

// display-width 기준으로 셀 잘라내기
inline std::string truncateToDisplay(const std::string& s, int maxDisplay) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int cw, cb;
        if      (c < 0x80) { cw = 1; cb = 1; }
        else if (c < 0xE0) { cw = 1; cb = 2; }
        else if (c < 0xF0) { cw = 2; cb = 3; }
        else               { cw = 2; cb = 4; }
        if (w + cw > maxDisplay) return s.substr(0, i) + "..";
        w += cw; i += cb;
    }
    return s;
}

inline void printTable(const std::vector<std::string>& headers,
                       const std::vector<std::vector<std::string>>& rows,
                       int colWidth = 15) {
    // setw는 바이트 기준이므로 한글 포함 시 보정값(byteLen - dispWidth)을 더함
    auto adjustedSetw = [&](const std::string& s) {
        return colWidth + (static_cast<int>(s.size()) - displayWidth(s));
    };

    setColor(Color::Cyan);
    std::cout << "  ";
    for (const auto& h : headers)
        std::cout << std::left << std::setw(adjustedSetw(h)) << h;
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
            // display-width 기준으로 잘라내기
            if (displayWidth(cell) > colWidth - 1)
                cell = truncateToDisplay(cell, colWidth - 3) + "..";
            std::cout << std::left << std::setw(adjustedSetw(cell)) << cell;
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
