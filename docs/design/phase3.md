# Phase 3 설계 문서: 비즈니스 서비스 레이어

## Agent 참여

| 단계 | Agent | 역할 |
|------|-------|------|
| ② 설계 검토 | **doc-consistency** | 상태 전이 허용 표가 plan.md의 상태 전이 다이어그램과 일치하는지, ProductionCalculator 수식이 plan.md와 동일한지 확인 |
| ③ 구현 | **test** | 유효성 검사·상태 전이·계산 공식을 TDD 순서로 구현. Mock Repository를 활용해 서비스 계층을 격리 테스트 |
| ④ 코드 검토 | **doc-consistency** | 승인 처리 세부 흐름이 코드와 일치하는지, 재고 차감 시점이 설계대로인지 확인 |

---

## 목표

이 Phase가 끝나면 다음이 가능하다.
- 시료 등록·조회·검색에 유효성 검사가 적용된다.
- 주문 접수 시 존재하지 않는 시료를 참조하면 거부된다.
- 주문 승인 시 재고를 확인해 `confirmed` 또는 `producing`으로 전이되고, 실생산 수량과 총 생산 시간이 계산된다.
- 잘못된 상태 전이(예: `released` 상태 주문을 다시 승인)는 예외로 차단된다.

---

## 새로 추가되는 파일

| 파일 | 위치 | 설명 |
|------|------|------|
| `SampleService.hpp/.cpp` | `services/` | 시료 CRUD + 유효성 검사 |
| `OrderService.hpp/.cpp` | `services/` | 주문 접수·승인·거절·조회 |
| `ProductionCalculator.hpp` | `services/` | 실생산 수량·총 생산 시간 계산 (순수 함수) |

---

## SampleService

### 제공하는 기능

| 메서드 | 설명 |
|--------|------|
| `findAll()` | 전체 시료 목록 반환 |
| `findById(sampleId)` | ID로 단일 시료 조회 |
| `search(keyword)` | 이름·ID에서 키워드 검색 |
| `create(sample)` | 시료 등록 |
| `updateStock(sampleId, delta)` | 재고 수량 증감 |

### 유효성 검사 규칙

| 항목 | 규칙 |
|------|------|
| `sampleId` | 비어 있으면 거부 |
| `sampleId` 중복 | 이미 존재하면 등록 거부 |
| `avgProductionTime` | 0.1 이상 2.0 이하 실수(double) |
| `yield` | 0.0 초과 1.0 이하 |
| `stockQty` | 0 이상 정수 |

---

## OrderService

### 제공하는 기능

| 메서드 | 설명 |
|--------|------|
| `placeOrder(sampleId, customerName, qty) → Order` | 주문 접수 → `reserved`, 생성된 Order 반환 |
| `approve(orderId)` | 승인 처리 (재고 확인 후 `confirmed` 또는 `producing`) |
| `reject(orderId)` | 거절 → `rejected` |
| `findByStatus(status)` | 상태별 주문 조회 |
| `findById(orderId)` | ID로 단일 주문 조회 |
| `release(orderId)` | 출고 처리 → `released` |

### 승인 처리 세부 흐름

```
approve(orderId):
  1. orderId에 해당하는 주문이 'reserved'인지 확인 (아니면 예외)
  2. 해당 시료의 현재 재고 조회
  3. 재고 >= 주문수량:
       - 재고 차감: stockQty -= 주문수량
       - 주문 상태 → confirmed
  4. 재고 < 주문수량:
       - productionQty, totalProductionMinutes 계산 (ProductionCalculator 사용)
       - queuedAt = 현재 시각
       - 주문 상태 → producing
       - 재고 차감: stockQty -= 현재 재고 (가용 재고 전량 선점)
       - productionStartedAt, estimatedCompletionAt은 Phase 4에서 설정
```

> `producing`으로 전이 시 재고를 선점하는 이유: 같은 시료에 대한 이후 주문이 이미 할당된 재고를 중복 사용하지 않도록 하기 위함.

### 상태 전이 허용 표

| 현재 상태 | 가능한 전이 | 불가능한 전이 |
|-----------|------------|--------------|
| `reserved` | → `confirmed`, `producing`, `rejected` | 그 외 전이 |
| `confirmed` | → `released` | 그 외 전이 |
| `producing` | → `confirmed` (Phase 4 lazy 처리) | 직접 `released`, `reserved` 등 |
| `released` | 없음 (최종) | 모든 전이 |
| `rejected` | 없음 (최종) | 모든 전이 |

---

## ProductionCalculator

순수 계산 함수 모음. 외부 상태(Repository, 시간)에 의존하지 않는다.

```
calcProductionQty(orderQty, stockQty, yield):
    부족분 = max(0, orderQty - stockQty)
    if 부족분 == 0: return 0
    return ceil(부족분 / (yield * 0.9))

calcTotalMinutes(productionQty, avgProductionTime):
    return productionQty * avgProductionTime
```

---

## TDD 테스트 케이스

### SampleService

| # | 시나리오 | 예상 결과 |
|---|----------|-----------|
| 1 | yield = 1.2 로 시료 등록 | 예외 발생 |
| 2 | avgProductionTime = 0 으로 등록 | 예외 발생 |
| 3 | 동일 sampleId 두 번 등록 | 두 번째에서 예외 발생 |
| 4 | 정상 시료 등록 후 findById | 등록한 시료 반환 |
| 5 | "AlGaN" 키워드 검색 | 이름에 "AlGaN" 포함된 시료 반환 |

### ProductionCalculator

| # | 시나리오 | 예상 결과 |
|---|----------|-----------|
| 1 | orderQty=30, stockQty=50, yield=0.85 | productionQty = 0 (재고 충분) |
| 2 | orderQty=30, stockQty=10, yield=0.85 | 부족분=20, qty=ceil(20/(0.85×0.9))=ceil(26.1)=27 |
| 3 | orderQty=10, stockQty=0, yield=1.0 | qty=ceil(10/0.9)=ceil(11.1)=12 |
| 4 | productionQty=12, avgTime=1.5 | totalMinutes=18.0 |

### OrderService

| # | 시나리오 | 예상 결과 |
|---|----------|-----------|
| 1 | 존재하지 않는 sampleId로 주문 | 예외 발생 |
| 2 | 재고 충분한 주문 승인 | status=confirmed, 재고 차감 |
| 3 | 재고 부족한 주문 승인 | status=producing, productionQty 계산됨 |
| 4 | confirmed 주문 다시 approve | 예외 발생 (잘못된 전이) |
| 5 | rejected 주문 approve | 예외 발생 |
| 6 | confirmed 주문 release | status=released |
| 7 | producing 주문 release (직접) | 예외 발생 |

---

## 의존 관계

```
OrderService    →  SampleService       (재고 조회·차감)
                →  OrderRepository     (주문 저장·조회)
                →  ProductionCalculator (수량 계산)

SampleService   →  SampleRepository   (시료 저장·조회)
ProductionCalculator → (없음, 순수 함수)
```
