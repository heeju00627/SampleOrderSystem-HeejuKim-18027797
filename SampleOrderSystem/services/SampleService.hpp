#pragma once
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

#include "../Model/Sample.hpp"
#include "../repositories/IRepository.hpp"

class SampleService {
public:
    explicit SampleService(std::shared_ptr<IRepository<Sample>> repo)
        : repo_(std::move(repo)) {}

    std::vector<Sample> findAll() const {
        return repo_->getAll();
    }

    std::optional<Sample> findById(const std::string& sampleId) const {
        return repo_->getById(sampleId);
    }

    std::vector<Sample> search(const std::string& keyword) const {
        auto all = repo_->getAll();
        std::vector<Sample> result;
        for (const auto& s : all) {
            if (s.sampleId.find(keyword) != std::string::npos ||
                s.name.find(keyword)     != std::string::npos) {
                result.push_back(s);
            }
        }
        return result;
    }

    void create(Sample& sample) {
        validate(sample);
        if (repo_->getById(sample.sampleId).has_value())
            throw std::invalid_argument("이미 존재하는 시료 ID: " + sample.sampleId);
        repo_->add(sample);
    }

    void updateStock(const std::string& sampleId, int delta) {
        auto opt = repo_->getById(sampleId);
        if (!opt) throw std::invalid_argument("시료를 찾을 수 없음: " + sampleId);
        Sample s = *opt;
        s.stockQty += delta;
        if (s.stockQty < 0) s.stockQty = 0;
        repo_->update(s);
    }

private:
    std::shared_ptr<IRepository<Sample>> repo_;

    void validate(const Sample& s) const {
        if (s.avgProductionTime < 0.1 || s.avgProductionTime > 2.0)
            throw std::invalid_argument("avgProductionTime은 0.1 이상 2.0 이하여야 합니다.");
        if (s.yield <= 0.0 || s.yield > 1.0)
            throw std::invalid_argument("yield는 0.0 초과 1.0 이하여야 합니다.");
        if (s.stockQty < 0)
            throw std::invalid_argument("stockQty는 0 이상이어야 합니다.");
        if (s.sampleId.empty())
            throw std::invalid_argument("sampleId는 비어 있을 수 없습니다.");
    }
};
