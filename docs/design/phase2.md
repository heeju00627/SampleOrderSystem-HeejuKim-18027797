# Phase 2 설계 문서: 시료·주문 데이터 모델 및 영속성

## Agent 참여

| 단계 | Agent | 역할 |
|------|-------|------|
| ② 설계 검토 | **doc-consistency** | 모델 필드 정의가 plan.md 도메인과 일치하는지, orderId 포맷이 일관성 있는지 확인 |
| ③ 구현 | **test** | TDD 테스트 케이스 목록을 기준으로 직렬화·Repository 테스트 작성 후 구현 |
| ④ 커버리지 | **test** | OpenCppCoverage 실행 후 미커버 경로 보완 |
| ⑤ 코드 검토 | **doc-consistency** | JSON 키 이름·필드 타입이 설계 문서와 일치하는지, 테스트 케이스가 모두 구현됐는지 확인 |

---

## 목표

이 Phase가 끝나면 다음이 가능하다.
- 시료와 주문을 JSON 파일에서 읽고 저장할 수 있다.
- 시료 ID로 특정 시료를 조회할 수 있다.
- 새 주문을 저장하면 주문 ID가 자동으로 부여된다.

---

## 새로 추가되는 파일

| 파일 | 위치 | 설명 |
|------|------|------|
| `Sample.hpp` | `Model/` | 시료 구조체 + JSON 변환 함수 |
| `Order.hpp` | `Model/` | 주문 구조체 + JSON 변환 함수 |
| `SampleRepository.hpp` | `repositories/` | `JsonRepository<Sample>` 인스턴스화 헬퍼 |
| `OrderRepository.hpp` | `repositories/` | `JsonRepository<Order>` 인스턴스화 헬퍼 |

---

## 데이터 구조

### Sample

```
sampleId          문자열   "S0001" 형식, 직접 입력
name              문자열   시료 이름
avgProductionTime 실수(double)  개당 생산시간 (0.1 ~ 2.0 min/ea)
yield             실수     0.60 ~ 1.00
stockQty          정수     현재 재고 수량 (0 이상), 기본값 0
```

> `sampleId`는 사용자가 직접 입력한다. 중복 시 등록 거부는 Phase 3(서비스 레이어)에서 처리한다.

### Order

```
orderId                문자열   "ORD-YYYYMMDD-NNNN" 형식, 자동 부여
sampleId               문자열   참조하는 시료 ID
customerName           문자열   고객명
orderQty               정수     주문 수량 (1 이상)
status                 문자열   reserved / confirmed / producing / released / rejected
productionQty          정수     실생산 수량 (승인 후 계산, 기본 0)
totalProductionMinutes 실수(double)  총 생산 시간 분 (승인 후 계산, 기본 0.0)
queuedAt               문자열   생산 대기열 진입 시각 ISO 8601, 없으면 빈 문자열
productionStartedAt    문자열   실제 생산 시작 시각 ISO 8601, 없으면 빈 문자열
estimatedCompletionAt  문자열   예상 완료 시각 ISO 8601, 없으면 빈 문자열
createdAt              문자열   주문 생성 시각 ISO 8601
```

#### orderId 자동 부여 규칙

`JsonRepository`의 "기존 최대 ID + 1" 패턴을 활용한다. 단, `orderId`가 문자열이므로 시퀀스 번호만 별도 파일(`data/order_seq.json`)에 정수로 저장하고, 저장 시마다 증가시켜 `ORD-{오늘날짜}-{4자리 시퀀스}` 형식으로 조합한다.

> 시퀀스는 날짜가 바뀌어도 초기화하지 않는다 (일관성 유지).

---

## JSON 파일 형식

### `data/samples.json`

```json
[
  {
    "sampleId": "S0001",
    "name": "AlGaN 에피",
    "avgProductionTime": 120,
    "yield": 0.85,
    "stockQty": 50
  }
]
```

### `data/orders.json`

```json
[
  {
    "orderId": "ORD-20260508-0001",
    "sampleId": "S0001",
    "customerName": "한국반도체",
    "orderQty": 30,
    "status": "reserved",
    "productionQty": 0,
    "totalProductionMinutes": 0,
    "queuedAt": "",
    "productionStartedAt": "",
    "estimatedCompletionAt": "",
    "createdAt": "2026-05-08T10:00:00"
  }
]
```

---

## Repository 구성

`JsonRepository<T>`에 주입할 람다 함수를 각 Repository 헬퍼에서 정의한다.

### SampleRepository

- `getId`: `sample.sampleId` 반환
- `setId`: 주입 없음 (sampleId는 사용자 입력값)
- `toJson`: Sample → JSON 문자열
- `fromJson`: JSON → Sample

### OrderRepository

- `getId`: `order.orderId` 반환
- `setId`: 새 `orderId` 문자열 할당 (시퀀스 파일 참조)
- `toJson`: Order → JSON 문자열
- `fromJson`: JSON → Order

---

## TDD 테스트 케이스

### Sample 직렬화

| # | 테스트 시나리오 | 예상 결과 |
|---|----------------|-----------|
| 1 | `toJson` 후 `fromJson` 왕복 변환 | 모든 필드 일치 |
| 2 | `stockQty`가 JSON에 없을 때 `fromJson` | `stockQty = 0` 기본값 |
| 3 | `samples.json` 파일이 없을 때 `findAll()` | 빈 벡터 반환, 예외 없음 |
| 4 | `findById("S0001")` | 해당 Sample 반환 |
| 5 | 존재하지 않는 ID `findById("S9999")` | `nullopt` 또는 빈 결과 |

### Order 직렬화

| # | 테스트 시나리오 | 예상 결과 |
|---|----------------|-----------|
| 1 | `toJson` 후 `fromJson` 왕복 변환 | 모든 필드 일치 |
| 2 | 빈 타임스탬프 필드 왕복 변환 | 빈 문자열 유지 |
| 3 | `save()` 호출 시 `orderId` 자동 부여 | `"ORD-"` 로 시작하는 ID 할당 |
| 4 | 연속 `save()` 두 번 | 두 번째 ID의 시퀀스 번호가 첫 번째보다 크다 |
| 5 | `update()`로 `status` 변경 후 `findById()` | 변경된 `status` 반환 |

---

## 의존 관계

```
SampleRepository  →  JsonRepository<Sample>  →  IRepository<Sample>
                                            ↘  JsonLib (json.hpp)
OrderRepository   →  JsonRepository<Order>   ↗
```

이 Phase에서 서비스 레이어(Phase 3)와 UI(Phase 5)는 포함하지 않는다.
