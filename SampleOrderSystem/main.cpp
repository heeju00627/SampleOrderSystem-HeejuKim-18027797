#ifdef _WIN32
#include <windows.h>
#endif

#ifdef UNIT_TEST

// ── 테스트 빌드: UNIT_TEST 정의 시 GTest 러너로 동작 ──────────
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

// ── 앱 빌드: UNIT_TEST 미정의 시 실제 애플리케이션 진입점 ──────
#include <iostream>
#include <memory>
#include <filesystem>

#include "repositories/IRepository.hpp"
#include "repositories/SampleRepository.hpp"
#include "repositories/OrderRepository.hpp"
#include "services/SampleService.hpp"
#include "services/OrderService.hpp"
#include "services/ProductionService.hpp"
#include "clock/SystemClock.h"
#include "Controller/OrderController.h"
#include "Model/Sample.hpp"
#include "Model/Order.hpp"

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    // exe 기준 data/ 경로
    namespace fs = std::filesystem;
    fs::path dataDir = fs::path("data");
    fs::create_directories(dataDir);

    // unique_ptr → shared_ptr<IRepository<T>> 변환 (covariant 이동 허용)
    std::shared_ptr<IRepository<Sample>> sampleRepo =
        makeSampleRepository((dataDir / "samples.json").string());
    std::shared_ptr<IRepository<Order>> orderRepo =
        makeOrderRepository(
            (dataDir / "orders.json").string(),
            (dataDir / "order_seq.json").string());

    auto clock     = std::make_shared<SystemClock>();
    auto sampleSvc = std::make_shared<SampleService>(sampleRepo);
    auto orderSvc  = std::make_shared<OrderService>(sampleRepo, orderRepo);
    auto prodSvc   = std::make_shared<ProductionService>(clock, orderRepo, sampleRepo);

    OrderController controller(sampleSvc, orderSvc, prodSvc);
    controller.run();
    return 0;
}

#endif
