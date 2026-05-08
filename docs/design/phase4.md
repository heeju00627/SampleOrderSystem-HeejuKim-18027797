# Phase 4 설계 문서: 생산 라인 시뮬레이터

## Agent 참여

| 단계 | Agent | 역할 |
|------|-------|------|
| ② 설계 검토 | **doc-consistency** | `applyLazyUpdates()` 알고리즘이 plan.md의 Lazy 시간 처리 원칙과 일치하는지, IClock 인터페이스가 Phase 3 서비스와 충돌 없는지 확인 |
| ③ 구현 | **test** | MockClock을 활용해 시간 경과 시나리오를 TDD로 구현. 연쇄 완료·FIFO 순서 케이스 집중 |
| ④ 코드 검토 | **doc-consistency** | 멱등성 보장 여부 확인, 타임스탬프 파싱 유틸리티 위치가 설계와 일치하는지 확인 |

---

## 목표

이 Phase가 끝나면 다음이 가능하다.
- 프로그램 시작 시(또는 메뉴 진입 시) 완료된 `producing` 주문이 자동으로 `confirmed`로 전이된다.
- 대기열(FIFO)의 다음 주문도 연쇄적으로 생산 시작 처리된다.
- 시간 처리 로직이 테스트용 가상 시계로 교체 가능해서 시간을 앞당겨 TDD 검증이 가능하다.

---

## 새로 추가되는 파일

| 파일 | 위치 | 설명 |
|------|------|------|
| `IClock.h` | `clock/` | 현재 시각 조회 인터페이스 |
| `SystemClock.h` | `clock/` | `std::chrono::system_clock` 구현체 |
| `MockClock.h` | `clock/` | 테스트용 시각 조작 구현체 |
| `ProductionService.hpp/.cpp` | `services/` | Lazy 갱신, FIFO 큐 관리 |

---

## 시간 추상화 (IClock)

시간에 의존하는 코드가 `IClock`만 바라보도록 해서, 실제 운영에서는 `SystemClock`, TDD에서는 `MockClock`을 주입한다.

```
IClock
  now() → time_point      현재 시각 반환

SystemClock : IClock
  now() → system_clock::now()

MockClock : IClock
  setNow(time_point)      테스트에서 원하는 시각을 설정
  advance(minutes)        분 단위로 시각을 앞당김
  now() → 설정된 시각
```

시각은 ISO 8601 문자열로 JSON에 저장하고, 메모리에서는 `std::chrono::system_clock::time_point`로 다룬다.

---

## ProductionService

### applyLazyUpdates() — 핵심 함수

프로그램 시작 시 및 각 메뉴 진입 시마다 호출한다.

```
applyLazyUpdates():
  1. 'producing' 상태이고 productionStartedAt이 설정된 주문 목록 조회
     → activeOrder (대기열 앞, 실제 생산 중인 주문)

  2. activeOrder.estimatedCompletionAt <= now() 이면:
       a. activeOrder.status = confirmed
       b. 해당 시료 stockQty += productionQty  (생산된 수량 재고 추가)
       c. 주문 저장

  3. 남은 'producing' 주문 중 productionStartedAt이 비어 있는 것을 queuedAt 순 정렬
     → 대기 중인 주문 목록

  4. 비어 있는 생산 슬롯(activeOrder가 없거나 방금 완료됨)에 대해:
       a. 대기 목록의 첫 번째 주문을 꺼냄
       b. productionStartedAt = max(now(), 직전 완료 시각)
            ※ 여러 주문이 연쇄 완료될 때 시각이 역행하지 않도록
       c. estimatedCompletionAt = productionStartedAt + totalProductionMinutes
       d. 주문 저장

  5. 2~4를 더 이상 완료할 주문이 없을 때까지 반복
```

> `producing` 상태이지만 `productionStartedAt`이 비어 있는 주문 = 대기열 대기 중  
> `producing` 상태이고 `productionStartedAt`이 설정된 주문 = 실제 생산 중

### startNextInQueue()

`applyLazyUpdates()` 내부에서 호출하는 내부 함수. 외부에서 직접 호출하지 않는다.

---

## 생산 라인 대기열 시각화 지원

`ProductionService`는 다음 조회 기능도 제공한다.

| 메서드 | 반환 |
|--------|------|
| `getActiveOrder()` | 현재 생산 중인 주문 (없으면 nullopt) |
| `getQueue()` | 대기 중 주문 목록 (queuedAt 오름차순) |
| `getRemainingMinutes(orderId)` | 잔여 생산 시간 (분) |

---

## TDD 테스트 케이스

모든 테스트에서 `MockClock`을 사용한다.

| # | 시나리오 | 예상 결과 |
|---|----------|-----------|
| 1 | producing 주문의 estimatedCompletionAt이 미래 → applyLazyUpdates | 상태 변화 없음 |
| 2 | MockClock을 completionAt 이후로 설정 → applyLazyUpdates | status = confirmed, 재고 증가 |
| 3 | producing 주문 2개 (A→B 순), A 완료 시각 이후로 설정 → applyLazyUpdates | A=confirmed, B의 productionStartedAt 설정됨 |
| 4 | producing 2개 모두 완료 시간 경과 → applyLazyUpdates | A=confirmed, B=confirmed, 순서대로 처리 |
| 5 | 대기 중 주문의 시작 시각이 앞 주문 완료 시각 이후인지 확인 | B.productionStartedAt >= A.estimatedCompletionAt |
| 6 | getActiveOrder() — 생산 중 없을 때 | nullopt |
| 7 | getQueue() — 대기 2건 | queuedAt 오름차순 정렬 |
| 8 | getRemainingMinutes — 현재 시각 기준 잔여 계산 | 예상 완료 시각 - 현재 시각 (분, 음수이면 0) |

---

## 의존 관계

```
ProductionService  →  IClock              (시간 조회)
                   →  OrderService        (주문 상태 변경, 조회)
                   →  SampleService       (생산 완료 시 재고 증가)

MockClock          →  IClock
SystemClock        →  IClock
```

---

## 주의사항

- `applyLazyUpdates()`는 메뉴 진입마다 호출되므로 멱등(idempotent)해야 한다. 같은 시각에 두 번 호출해도 결과가 달라지지 않아야 한다.
- 타임스탬프 파싱/포매팅은 이 Phase에서 공용 유틸리티 함수로 분리한다.
