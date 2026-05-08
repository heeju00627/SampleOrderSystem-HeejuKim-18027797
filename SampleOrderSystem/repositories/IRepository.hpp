#pragma once
#include <vector>
#include <optional>
#include <string>

// Phase 2에서 Sample(string sampleId)과 Order(string orderId)에 맞게
// getId/getById/remove의 ID 타입을 std::string으로 교체한다.
template<typename T>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::vector<T>         getAll()                    const = 0;
    virtual std::optional<T>       getById(const std::string& id) const = 0;
    virtual void                   add(T& entity)                    = 0;
    virtual void                   update(const T& entity)           = 0;
    virtual bool                   remove(const std::string& id)     = 0;
};
