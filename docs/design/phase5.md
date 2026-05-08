# Phase 5 설계 문서: 콘솔 UI 완성

## Agent 참여

| 단계 | Agent | 역할 |
|------|-------|------|
| ② 설계 검토 | **doc-consistency** | 메뉴 구조가 plan.md의 6개 기능과 1:1 매핑되는지 확인 |
| ② 설계 검토 | **system-design** | 전체 메뉴 흐름, 출력 형식, 색상 코딩, 확인 프롬프트 설계안 검토 및 개선안 제시 |
| ③ 구현 | **test** | 재고 상태 판단 함수·잔여 시간 계산 등 순수 로직 TDD 구현 |
| ③ 구현 | **system-design** | ConsoleView 각 출력 메서드 구현 시 UX 조언 및 출력 예시 제공 |
| ④ 커버리지 | **test** | OpenCppCoverage 실행 후 미커버 경로 보완 |
| ⑤ 코드 검토 | **doc-consistency** | 구현된 메뉴 번호·흐름이 설계 문서와 일치하는지 확인 |
| ⑤ 코드 검토 | **system-design** | 실제 콘솔 출력의 가독성 최종 검토 |

---

## 목표

이 Phase가 끝나면 다음이 가능하다.
- 운영자가 콘솔 메뉴를 통해 시스템의 모든 기능을 사용할 수 있다.
- 각 서브메뉴 핸들러 진입 시마다 `applyLazyUpdates()`가 자동으로 호출된다.
- 색상 테이블로 현황이 가독성 있게 표시된다.

---

## 새로 추가·수정되는 파일

| 파일 | 위치 | 설명 |
|------|------|------|
| `OrderController.h` | `Controller/` | 헤더 전용. 메인 루프 + 메뉴 디스패치 |
| `UIHelpers.hpp` | `Controller/` | StockStatus, formatRemainingTime 순수 함수 (OS 독립) |
| `ConsoleUI.h` (수정) | `View/` | `getYNInput`, `displayWidth` 보정, 컬럼별 너비 `printTable` 추가 |
| `main.cpp` (수정) | `SampleOrderSystem/` | 실제 앱 진입점 (#else 분기) 구현 |

---

## 전체 메뉴 구조

```
[S-Semi 생산주문관리 시스템]
──────────────────────────────
1. 시료 관리
2. 시료 주문
3. 주문 승인/거절
4. 모니터링
5. 생산 라인 조회
6. 출고 처리
0. 종료
```

### 서브메뉴 공통 규칙

- 모든 서브메뉴는 **작업 완료 후 `pressEnterToContinue()` → 메인 메뉴로 자동 복귀**
- 서브메뉴가 있는 화면(1번)에서는 `[0] 돌아가기` 항목을 표시
- **각 서브메뉴 핸들러 진입 시마다 `applyLazyUpdates()` 호출**
- 빈 목록은 `printEmpty()` 출력

---

## 메뉴별 세부 흐름

### 1. 시료 관리

```
서브메뉴:
  [1] 시료 목록 조회
  [2] 시료 검색
  [3] 시료 등록
  [0] 돌아가기

1-1. 시료 목록
     → 테이블: sampleId | 이름 | 평균생산시간(min/ea) | 수율 | 재고 | 상태

1-2. 시료 검색
     → 키워드 입력 → sampleId·이름 포함 검색 → 결과 테이블 표시

1-3. 시료 등록
     → sampleId, 이름, 평균생산시간(double, min/ea), 수율(double) 입력
     → 유효성 검사 실패 시 오류 메시지 후 메인 메뉴 복귀
     → 성공:
        [OK] 시료 등록 완료: S0001
```

### 2. 시료 주문

```
→ 시료 목록 표시
→ 시료ID 입력 (0 또는 빈 입력 시 취소 후 복귀)
→ 고객명, 주문수량 입력
→ 성공:
   [OK] 주문 접수 완료: ORD-20260508-0001 (reserved)
```

### 3. 주문 승인/거절

```
→ reserved 주문 목록 표시 (번호 | orderId | 시료ID | 고객명 | 수량 | 접수일시)
→ 번호 선택 (0=취소, 0 입력 시 메인 복귀)
→ 선택 후 해당 시료 재고 현황 즉시 표시:
     [재고 현황] 시료명  현재재고: N개  /  주문수량: M개
       재고 충분 → "재고 충분 (즉시 confirmed)"  (Green)
       재고 부족 → "재고 부족 (생산 필요, producing)"  (Yellow)
→ [Y] 승인 / [N] 거절 / [0] 취소 선택

  승인:
    재고 충분 →
      [OK] 승인 완료 — 출고 대기 (confirmed)
           재고: 100개 → 50개

    재고 부족 →
      [OK] 승인 완료 — 생산 등록 (producing)
           실생산량: 33개  /  예상완료: 2026-05-08T14:30:00

  거절:
    → "정말 거절하시겠습니까? [Y/N]" 확인 프롬프트 (파괴적 작업)
    → Y 선택 시: [OK] 거절 완료 (rejected)
    → N 선택 시: 취소 후 메인 복귀

  취소 (0):
    → 승인·거절 없이 메인 메뉴로 복귀
```

### 4. 모니터링

서브메뉴 루프: 1·2 조회 후 메인 메뉴로 자동 복귀하지 않고 서브메뉴로 돌아온다.
0 입력 시 메인 메뉴로 복귀. 반복 조회 가능.

```
4-1. 주문 상태별 목록
     각 상태별 섹션 헤더 + 테이블 순차 출력 (rejected 제외)

     reserved / confirmed / released:
       테이블: 주문ID | 시료ID | 고객명 | 주문량

     producing (생산 중):
       테이블: 주문ID | 시료ID | 고객명 | 주문량 | 실생산량 | 예상완료
       (예상완료 미설정 시 "(대기중)" 표시)

4-2. 시료별 재고 현황
     테이블: 시료ID | 이름 | 재고 | 예약수요 | 충족률 | 상태

     - 충족률 = 재고 / 예약수요 × 100% (예약수요=0이면 "-")
     - 충족률이 아닌 "잔여율"이 아님에 주의: 수요를 얼마나 충당할 수 있는지를 나타냄

     재고 상태 판단 (아래 순서로 우선 평가):
       ① 재고 = 0            → 고갈 (Red)
       ② 0 < 재고 < 예약수요 → 부족 (Yellow)
       ③ 재고 >= 예약수요     → 여유 (Green)

     ※ 예약수요: reserved 상태 주문수량 합산.
        confirmed/producing 주문은 approve() 시점에 stockQty에서 이미 차감됐으므로
        이중 집계를 방지하기 위해 수요 계산에서 제외한다.
```

### 5. 생산 라인 조회

```
5-1. 현재 생산 중인 주문
     → 없으면 "(생산 중인 주문 없음)" 출력
     → 있으면:
        테이블: orderId | 시료 | 고객 | 수량 | 시작시각 | 예상완료
        + 잔여시간: 테이블 아래 별도 행으로 색상 출력

     잔여시간 표시: "Xh Ym" (60분 이상) 또는 "Ym" (60분 미만), 분 단위 ceiling(올림)
     잔여시간 색상:
       60분 초과 → Green
       60분 이하 → Yellow
       0분 (완료 대기) → Red

5-2. 생산 대기 중 주문 (FIFO 순서)
     → 테이블: 순번 | orderId | 시료 | 고객 | 수량 | 대기 진입시각
```

### 6. 출고 처리

```
→ confirmed 주문 목록 표시 (번호 | orderId | 시료ID | 고객명 | 수량)
→ 번호 선택 (0=취소)
→ "출고 처리하시겠습니까? [Y/N]" 확인 프롬프트
→ Y 선택 시: [OK] 출고 완료: ORD-20260508-0001 (released)
→ N 선택 시: 취소
```

---

## 색상 코딩 기준

### 재고 상태

| 상태 | 색상 |
|------|------|
| 여유 | Green |
| 부족 | Yellow |
| 고갈 | Red |

### 주문 상태

| 상태 | 색상 |
|------|------|
| reserved | White |
| confirmed | Green |
| producing | Cyan |
| released | Gray |
| rejected | Red |

---

## ConsoleView 추가 헬퍼

`ConsoleUI.h`의 `getIntInput`, `getStringInput` 외에 아래를 추가한다.

```cpp
// ConsoleUI.h에 추가
inline bool getYNInput(const std::string& prompt) {
    while (true) {
        std::string ans = getStringInput(prompt + " [Y/N]: ");
        if (ans == "Y" || ans == "y") return true;
        if (ans == "N" || ans == "n") return false;
        printError("Y 또는 N을 입력해주세요.");
    }
}
```

---

## OrderController

`IController` 인터페이스를 구현하는 헤더 전용 클래스.

```
OrderController::run():
  while (실행 중):
    메인 메뉴 출력
    번호 입력 수신
    handleCommand(번호)

handleCommand(번호):
  applyLazyUpdates()   ← 모든 메뉴 진입 시 호출
  switch(번호):
    1 → handleSampleMenu()
    2 → handlePlaceOrder()
    3 → handleApproveReject()
    4 → handleMonitor()
    5 → handleProductionLine()
    6 → handleRelease()
    0 → 종료
```

---

## TDD 테스트 (순수 로직)

| # | 시나리오 | 검증 방법 |
|---|----------|-----------|
| 1 | 재고=0 → 고갈 | StockStatus 순수 함수 |
| 2 | 0 < 재고 < 합계 → 부족 | StockStatus 순수 함수 |
| 3 | 재고 >= 합계 → 여유 | StockStatus 순수 함수 |
| 4 | 재고 = 합계 → 여유 (차감 완료, 잔여 없음이 아닌 충족 상태) | StockStatus 순수 함수 |
| 5 | 재고=0, 합계=0 → 고갈 (재고 없음 우선) | StockStatus 순수 함수 |
| 6 | 잔여시간 포맷: 143분 → "2h 23m" | formatRemainingTime 순수 함수 |
| 7 | 잔여시간 포맷: 60분 → "1h 0m" | formatRemainingTime 순수 함수 |
| 8 | 잔여시간 포맷: 45분 → "45m" | formatRemainingTime 순수 함수 |
| 9 | 잔여시간 포맷: 0분 → "0m" | formatRemainingTime 순수 함수 |

나머지 UI 동작은 Phase 6 통합 검증에서 수동으로 확인한다.
