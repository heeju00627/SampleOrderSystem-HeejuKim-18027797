#pragma once
#include <string>

namespace View {

class IView {
public:
    virtual ~IView() = default;
    virtual void showMessage(const std::string& message) = 0;
    virtual void showError(const std::string& error) = 0;
    virtual std::string promptInput(const std::string& prompt) = 0;
};

} // namespace View
