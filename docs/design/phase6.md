# Phase 6 설계 문서: 통합 검증

## Agent 참여

| 단계 | Agent | 역할 |
|------|-------|------|
| ② 설계 검토 | **doc-consistency** | 검증 시나리오가 plan.md의 전체 기능 목록을 빠짐없이 커버하는지 확인 |
| ③ 검증 수행 | **system-design** | 실제 콘솔 출력 가독성 최종 검토, 운영자 관점 UX 종합 평가 |
| ④ 커버리지 | **test** | 전체 누적 커버리지 측정, 미커버 주요 경로 최종 보완 |
| ⑤ 최종 검토 | **doc-consistency** | CLAUDE.md가 최종 구현 상태를 정확히 반영하는지 확인, 업데이트 필요 항목 목록화 |

---

## 목표

이 Phase가 끝나면 다음이 가능하다.
- 전체 워크플로우가 실제 콘솔 환경에서 오류 없이 동작한다.
- 프로그램 재시작 후에도 생산 상태가 올바르게 복원된다.
- 비정상 입력과 파일 오류 상황에서 안전하게 처리된다.

---

## 검증 시나리오

### 시나리오 1: 재고 충분 — 즉시 출고

```
1. 시료 등록 (sampleId=S0001, avgTime=120, yield=0.85, 초기재고=100)
2. 주문 접수 (sampleId=S0001, qty=30) → ORD-...-0001, status=reserved
3. 주문 승인 → 재고 100 >= 30, status=confirmed, 재고=70
4. 출고 처리 → status=released
5. 모니터링: released 목록에 ORD-...-0001 확인, 재고=70 확인
```

### 시나리오 2: 재고 부족 — 생산 후 출고

```
1. 시료 등록 (S0002, avgTime=60, yield=0.9, 초기재고=5)
2. 주문 접수 (S0002, qty=20) → status=reserved
3. 주문 승인:
   - 부족분 = 20-5=15, productionQty=ceil(15/(0.9×0.9))=ceil(18.52)=19
   - totalProductionMinutes = 60 × 19 = 1140분
   - status=producing, 재고=0 (5 선점 차감)
4. 생산 라인 조회:
   - 현재 생산 중: ORD-..., 잔여 시간 표시
5. (MockClock으로 1140분 경과 시뮬레이션)
6. applyLazyUpdates() → status=confirmed, 재고=19 증가
7. 출고 처리 → status=released
```

### 시나리오 3: 프로그램 재시작 후 생산 상태 복원

```
1. 주문 승인 → producing (예상완료시각=T+1140분)
2. 프로그램 종료 (JSON 파일에 타임스탬프 저장됨)
3. 실제 시간이 1140분 이상 경과
4. 프로그램 재시작
5. 시작 시 applyLazyUpdates() 자동 호출
6. 모니터링: status=confirmed 확인
```

### 시나리오 4: FIFO 대기열

```
1. 시료 S0003 등록 (avgTime=30, yield=0.8, 재고=0)
2. 주문 A 접수 (qty=10) → 승인 → producing (생산 시작, 잔여 A.totalMinutes)
3. 주문 B 접수 (qty=5) → 승인 → producing (대기, productionStartedAt 비어 있음)
4. 주문 C 접수 (qty=8) → 승인 → producing (대기)
5. 생산 라인 조회: A 생산 중, B→C 순서 대기 확인
6. A 완료 시각 경과 → applyLazyUpdates:
   - A=confirmed
   - B 생산 시작 (productionStartedAt = A.estimatedCompletionAt)
7. A+B 완료 시각 경과 → applyLazyUpdates:
   - B=confirmed
   - C 생산 시작
```

### 시나리오 5: 잘못된 입력 처리

```
- 존재하지 않는 시료ID로 주문 접수 → 오류 메시지, 진행 불가
- confirmed 주문 재승인 시도 → 오류 메시지
- 숫자 입력 칸에 문자 입력 → 오류 메시지 + 재입력 요청
```

### 시나리오 6: 손상된 JSON 파일

```
- data/samples.json 내용을 빈 파일로 교체 후 시작 → 빈 목록으로 정상 시작
- data/orders.json을 잘못된 JSON으로 교체 후 시작 → 오류 메시지 표시 후 종료 또는 빈 목록으로 복구
```

---

## 수동 검증 체크리스트

### 기능 완성도

- [ ] 시료 등록·조회·검색
- [ ] 주문 접수 및 주문 ID 자동 부여 확인
- [ ] 재고 충분 → confirmed 전이 + 재고 차감
- [ ] 재고 부족 → producing 전이 + 실생산 수량 표시
- [ ] 주문 거절 → rejected (모니터링 목록에 미표시 확인)
- [ ] 생산 완료 lazy 처리 → confirmed 전이
- [ ] FIFO 대기열 순서 유지
- [ ] 출고 처리 → released
- [ ] 모니터링: 상태별 목록, 재고 여유/부족/고갈 색상 표시
- [ ] 생산 라인: 잔여 시간, 대기 순서 표시

### 안정성

- [ ] 빈 목록에서 모든 메뉴 진입 시 크래시 없음
- [ ] 비정상 입력(빈 문자열, 음수, 초과 범위) 처리
- [ ] JSON 파일이 없는 상태로 첫 실행 시 정상 동작

---

## 성능 기준

별도의 성능 목표는 없으나, 시료 100개·주문 500개 데이터를 로드했을 때 메뉴 진입이 1초 이내에 응답해야 한다.
