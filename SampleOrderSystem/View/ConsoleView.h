#pragma once
#include "IView.h"

namespace View {

class ConsoleView : public IView {
public:
    void showMessage(const std::string& message) override;
    void showError(const std::string& error) override;
    std::string promptInput(const std::string& prompt) override;
};

} // namespace View
