#pragma once
#include "IRepository.hpp"
#include <JsonLib/json.hpp>
#include <functional>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stdexcept>

template<typename T>
class JsonRepository : public IRepository<T> {
public:
    // setId: 엔티티에 ID를 부여하는 람다. 직접 생성·설정까지 책임짐 (Sample은 no-op, Order는 sequence 생성)
    using SetIdFn   = std::function<void(T&)>;
    using GetIdFn   = std::function<std::string(const T&)>;
    using ToJsonFn  = std::function<json::Value(const T&)>;
    using FromJsonFn= std::function<T(const json::Value&)>;

    JsonRepository(std::string filePath,
                   SetIdFn    setId,
                   GetIdFn    getId,
                   ToJsonFn   toJson,
                   FromJsonFn fromJson)
        : filePath_(std::move(filePath))
        , setId_(std::move(setId))
        , getId_(std::move(getId))
        , toJson_(std::move(toJson))
        , fromJson_(std::move(fromJson))
    {
        namespace fs = std::filesystem;
        fs::path p(filePath_);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        if (!fs::exists(filePath_)) { std::ofstream f(filePath_); f << "[]"; }
    }

    std::vector<T> getAll() const override {
        json::Value root = json::Value::parseFile(filePath_);
        std::vector<T> result;
        result.reserve(root.size());
        for (const auto& item : root)
            result.push_back(fromJson_(item));
        return result;
    }

    std::optional<T> getById(const std::string& id) const override {
        for (const auto& item : getAll())
            if (getId_(item) == id) return item;
        return std::nullopt;
    }

    void add(T& entity) override {
        setId_(entity);          // no-op(Sample) 또는 orderId 생성(Order)
        auto list = getAll();
        list.push_back(entity);
        persist(list);
    }

    void update(const T& entity) override {
        auto list = getAll();
        for (auto& item : list) {
            if (getId_(item) == getId_(entity)) {
                item = entity;
                persist(list);
                return;
            }
        }
        throw std::runtime_error("ID '" + getId_(entity) + "' 항목을 찾을 수 없습니다.");
    }

    bool remove(const std::string& id) override {
        auto list = getAll();
        auto it = std::remove_if(list.begin(), list.end(),
            [&](const T& item) { return getId_(item) == id; });
        if (it == list.end()) return false;
        list.erase(it, list.end());
        persist(list);
        return true;
    }

private:
    std::string filePath_;
    SetIdFn     setId_;
    GetIdFn     getId_;
    ToJsonFn    toJson_;
    FromJsonFn  fromJson_;

    void persist(const std::vector<T>& list) const {
        json::Value arr = json::makeArray();
        for (const auto& item : list) arr.push_back(toJson_(item));
        arr.save(filePath_, 2);
    }
};
