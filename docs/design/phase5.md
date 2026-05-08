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
- 각 메뉴 진입 시 `applyLazyUpdates()`가 자동으로 호출된다.
- 색상 테이블로 현황이 가독성 있게 표시된다.

---

## 새로 추가·수정되는 파일

| 파일 | 위치 | 설명 |
|------|------|------|
| `OrderController.h/.cpp` | `Controller/` | ConsoleMVC AppController 기반, 메인 루프 + 메뉴 디스패치 |
| `ConsoleView.h/.cpp` (수정) | `View/` | DataMonitor ConsoleUI의 색상·테이블 기능 통합 |
| `main.cpp` (수정) | `SampleOrderSystem/` | Model/View/Controller 조립 + ProductionService 주입 |

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

메뉴 선택 → 서브메뉴 진입 → 서브메뉴 진입 시마다 `applyLazyUpdates()` 호출.

---

## 메뉴별 세부 흐름

### 1. 시료 관리

```
1-1. 시료 목록 조회
     → 전체 시료를 테이블로 표시 (sampleId, 이름, 평균생산시간, 수율, 재고)

1-2. 시료 검색
     → 키워드 입력 → 이름 또는 ID에서 검색 → 결과 테이블 표시

1-3. 시료 등록
     → sampleId, 이름, 평균생산시간(분), 수율 입력
     → 유효성 검사 실패 시 오류 메시지 표시 후 재입력
     → 성공 시 "등록 완료" 메시지
```

### 2. 시료 주문

```
→ 시료 목록 표시
→ 시료ID, 고객명, 주문수량 입력
→ 주문 ID 자동 부여, status=reserved
→ "주문 접수 완료: ORD-..." 메시지
```

### 3. 주문 승인/거절

```
→ reserved 상태 주문 목록 표시 (orderId, 시료명, 고객명, 주문수량)
→ 처리할 주문 ID 입력
→ [승인(Y) / 거절(N)] 선택

  승인:
    재고 충분 → "승인 완료, 출고 대기(confirmed)" + 재고 차감 표시
    재고 부족 → "승인 완료, 생산 등록(producing)" + 실생산수량·예상완료시각 표시

  거절:
    → "거절 완료(rejected)"
```

### 4. 모니터링

```
4-1. 주문 상태별 목록
     → 탭 형태로 reserved / confirmed / producing / released 목록 표시
     (rejected 제외)

4-2. 시료별 재고 현황
     → 시료별 재고를 테이블로 표시
     → 재고 상태 컬럼: 여유(초록) / 부족(노랑) / 고갈(빨강)

재고 상태 판단:
  confirmed/producing 상태의 총 주문수량 합계 대비 현재 재고
    재고 >= 주문 합계    → 여유
    0 < 재고 < 주문 합계 → 부족
    재고 = 0            → 고갈
```

### 5. 생산 라인 조회

```
5-1. 현재 생산 중인 주문
     → orderId, 시료명, 고객명, 주문수량, 시작시각, 예상완료시각, 잔여시간(분) 표시

5-2. 생산 대기 중 주문 (FIFO 순서)
     → 대기 순번, orderId, 시료명, 고객명, 주문수량, 대기열 진입 시각 표시
```

### 6. 출고 처리

```
→ confirmed 상태 주문 목록 표시
→ 출고 처리할 주문 ID 입력
→ 확인 후 status = released
→ "출고 완료" 메시지
```

---

## ConsoleView 확장

DataMonitor `ConsoleUI.h`에서 가져올 기능:

| 기능 | 설명 |
|------|------|
| 색상 출력 | Windows: `SetConsoleTextAttribute`, 비-Windows: ANSI escape code |
| 테이블 출력 | 컬럼 너비 자동 정렬, 헤더 구분선 |
| 입력 프롬프트 | 정수 입력, 문자열 입력, Y/N 확인 헬퍼 |

재고 상태별 색상:
- 여유: 초록 (Green)
- 부족: 노랑 (Yellow)
- 고갈: 빨강 (Red)
- 생산 중: 시안 (Cyan)

---

## OrderController

ConsoleMVC `AppController`의 `run()` / `handleCommand()` 구조를 계승한다.

```
OrderController::run():
  while (실행 중):
    메인 메뉴 출력
    사용자 입력 수신
    applyLazyUpdates()    ← 메뉴 선택마다 호출
    handleCommand(입력)
```

`handleCommand()`는 메뉴 번호에 따라 각 서브메뉴 핸들러 메서드를 호출한다.  
각 핸들러 메서드는 `SampleService`, `OrderService`, `ProductionService`와 상호작용한다.

---

## 이 Phase에서 추가하는 TDD 테스트

UI 로직은 I/O 의존성 때문에 단위 테스트가 어렵다. 아래 항목에 한해 테스트한다.

| # | 시나리오 | 검증 방법 |
|---|----------|-----------|
| 1 | 재고 상태 판단 함수 (여유/부족/고갈) | 순수 함수 단위 테스트 |
| 2 | 잔여 시간 표시 음수 방어 (0 표시) | ProductionService 단위 테스트 |

나머지 UI 동작은 Phase 6 통합 검증에서 수동으로 확인한다.
