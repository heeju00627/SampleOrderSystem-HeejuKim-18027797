# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

**S-Semi** 반도체 시료 생산주문관리 시스템. C++20 콘솔 애플리케이션으로, 시료(Sample)와 주문(Order) 도메인을 중심으로 동작하며 실제 시간 기반 분단위 생산 라인 시뮬레이션을 포함한다. 구현 계획 상세는 `docs/plan.md` 참조.

## 빌드

Visual Studio 2022 (MSBuild, 툴셋 v145, C++20) 기반. NuGet 패키지 복원은 Visual Studio에서 자동 처리되거나 nuget.exe를 통해 수동 복원.

```powershell
# Debug x64 빌드
msbuild SampleOrderSystem-HeejuKim-18027797.slnx /p:Configuration=Debug /p:Platform=x64

# Release x64 빌드
msbuild SampleOrderSystem-HeejuKim-18027797.slnx /p:Configuration=Release /p:Platform=x64
```

빌드 결과물은 `x64/Debug/` 또는 `x64/Release/` 디렉토리에 생성된다.

## 테스트

Google Mock 1.11.0 (NuGet 패키지 `gmock.1.11.0`)을 사용한다. 테스트 바이너리 실행:

```powershell
# 빌드 후 테스트 실행
.\x64\Debug\SampleOrderSystem.exe

# 특정 테스트 필터링 (GTest 필터 구문)
.\x64\Debug\SampleOrderSystem.exe --gtest_filter=OrderTest.*
```

## 코드 커버리지

OpenCppCoverage로 측정한다. Phase 2부터 각 Phase 구현 완료 시 커버리지를 확인한다.

```powershell
# 커버리지 측정 및 HTML 리포트 생성
OpenCppCoverage.exe --sources C:*.cpp --export_type=html:coverage -- .\x64\Debug\SampleOrderSystem.exe
```

리포트는 `.\coverage\` 폴더에 HTML로 생성된다. 브라우저로 `.\coverage\index.html`을 열어 확인한다.

## 개발 방식

이 프로젝트는 **TDD(테스트 주도 개발)** 방식으로 진행한다. `/tdd` 스킬(`.claude/skills/test-driven-development/SKILL.md`)이 설정되어 있으므로, 새 기능·버그픽스·리팩터링 시 반드시 먼저 실패하는 테스트를 작성한 뒤 구현 코드를 작성한다.

## 아키텍처

소스 파일은 `SampleOrderSystem/` 폴더 아래 배치. 헤더(`.h`/`.hpp`)와 소스(`.cpp`)를 분리 관리한다.

핵심 도메인:
- **Sample(시료)**: 반도체 시료 정보 및 상태. 시료ID,이름, 평균생산시간, 수율을 속성값으로 가짐.
- **Order(주문)**: 시료ID, 고객명, 주문수량을 입력으로 받게 되어 있음. 주문번호(특정 포맷으로 자동 할당), 주문상태를 속성값으로 가지며, reserved, confirmed, producing, released, rejected가 가능. 주문 시점에서의 상태는 reserved. 
- **ProductionLine(생산라인)**: 대기열은 FIFO로 관리. 주문량에 대한 부족분을 생산하되, 수율 및 오차를 고려하여 주문 당 실생산량은 ceil(부족분/(수율*0.9)), 총생산시간은 (시료 평균생산시간*실생산량)으로 관리. 

동작 원칙: 주문 시간을 기록해놓고, 총생산시간에 따른 예상 완료 시간 역시 기록해놓았다가, 프로그램이 껐따 켜지거나 특정 메뉴 동작하는 시점에 현재 시간에 맞게 분단위로 lazy하게 시료 상태 업데이트.

재고 관리 원칙: approve() 시점에 stockQty를 즉시 차감(재고 충분 시 -= orderQty, 부족 시 = 0). 따라서 재고 현황 모니터링의 예약수요는 reserved(미승인) 주문만 집계하며, confirmed/producing은 이미 차감됐으므로 제외한다.

핵심 파일 구조:
```
Controller/
  OrderController.h   — 6개 메뉴 핸들러, 메인 루프 (헤더 전용)
  UIHelpers.hpp       — StockStatus, formatRemainingTime 순수 함수
Model/
  Sample.hpp / Order.hpp
repositories/
  IRepository.hpp / JsonRepository.hpp
  SampleRepository.hpp / OrderRepository.hpp
services/
  SampleService.hpp / OrderService.hpp
  ProductionService.hpp / ProductionCalculator.hpp
clock/
  IClock.h / SystemClock.h / MockClock.h
View/
  ConsoleUI.h         — 색상·테이블·입력 헬퍼 (displayWidth 보정 포함)
  ConsoleView.h/.cpp  — IView 구현체
extern/JsonLib/       — JSON 파싱·직렬화 라이브러리
data/                 — 초기 시드 데이터 (버전 관리 포함)
  samples.json / orders.json / order_seq.json
```

## 주요 기능
1. 시료 관리: 각 시료는 고유한 이름과 속성을 가지며, 시스템에 등록된 시료만 주문 가능.
    1) 등록: 새로운 시료를 시스템에 추가. 시료ID, 이름, 평균생산시간, 수율을 속성으로 입력해야함.
    2) 조회: 등록된 모든 시료 목록을 확인. 현재 재고 수량도 함께 표시
    3) 검색: 이름 등 속성으로 특정 시료를 검색
2. 시료 주문
    1) 원하는 시료와 수량을 주문하며 시료ID, 고객명, 주문수량 입력.
    2) 주문 상태는 reserved 상태.
    3) 주문번호를 자동할당.
3. 주문 승인/거절
    1) 접수된 주문(reserved) 목록 display.
    2) 승인/거절할 주문을 선택하고 승인/거절 여부 선택.
    2) 승인: 재고가 충분한 경우 즉시 주문을 confirmed 상태로 전환. 재고가 부족한 경우 생산 라인에 자동으로 등록하고 주문 상태를 producing으로 전환.
    3) 거절: 즉시 rejected 상태로 전환.
4. 모니터링
    1) 주문량 확인: 주문 상태별(reserved/confirmed/producing/released) 목록 display. rejected는 유효한 주문이 아니므로 무시.
    2) 재고량 확인: 각 시료별 현재 재고 수량 display. 주문 대비 재고 충분하면 여유, 주문 대비 재고 수량 부족하면 부족, 수량이 0이면 고갈로 표시.
5. 생산 라인 조회
    1) 생산 현황 표기: 현재 생산중인 시료에 대한 정보 표기
    2) 대기 주문 확인: 한 번에 하나의 주문에 대해서만 생산할 수 있으므로 생산라인의 대기열은 FIFO 활용. 대기하고 있는 목록 출력.
6. 출고 처리
    1) confirmed 주문에 대하여 출고 처리하면 주문 상태를 released로 전환.

## 시스템 동작
주문 등록(reserved) -> 승인 여부 결정 -> 승인이면 재고확인, 거절이면 rejected.
재고 확인 -> 재고 충분하면, 출고 준비(confirmed) -> 출고 처리(released)
재고 확인 -> 재고 부족하면 생산 요청(producing) -> 생산 완료(완료 시간에 도달하면) 출고 준비(confirmed) -> 출고 처리(released)

## Agent 팀

`.claude/agents/`에 세 agent가 정의되어 있으며, 각 Phase의 설계 검토·구현·코드 검토 단계에서 역할을 분담한다.

| Agent | 호출 시점 | 역할 요약 |
|-------|-----------|-----------|
| **doc-consistency** | 모든 Phase ② 설계 검토, ⑤ 코드 검토 | plan.md·설계 문서·코드 간 불일치 탐지. 코드를 직접 수정하지 않고 보고만 한다. |
| **test** | Phase 2~5 ③ 구현 | Google Mock/GTest 기반 TDD. 반드시 테스트 실패를 확인한 뒤 구현한다. |
| **system-design** | Phase 5 ②③, 출력 형식 추가 시 | 콘솔 출력 가독성·메뉴 흐름·색상 코딩 설계 및 검토. 코드를 직접 수정하지 않는다. |

각 Phase 설계 문서(`docs/design/phaseN.md`) 상단의 **Agent 참여** 표에서 단계별 세부 역할을 확인한다.

## 주의사항

- 모든 모듈에서 C++20 표준(`stdcpp20`)을 사용한다.
- `packages\gmock.1.11.0\` 디렉토리는 NuGet 캐시이므로 수정하지 않는다.
- `.vs/` 폴더는 IDE 상태 파일로 편집 대상이 아니다.
