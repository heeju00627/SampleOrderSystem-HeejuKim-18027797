#include "ConsoleView.h"
#include <iostream>

namespace View {

void ConsoleView::showMessage(const std::string& message) {
    std::cout << message << "\n";
}

void ConsoleView::showError(const std::string& error) {
    std::cerr << "[ERROR] " << error << "\n";
}

std::string ConsoleView::promptInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

} // namespace View
