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

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "S-Semi 생산주문관리 시스템\n";
    return 0;
}

#endif
