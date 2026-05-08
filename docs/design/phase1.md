# Phase 1 설계 문서: 프로젝트 기반 구성

## Agent 참여

| 단계 | Agent | 역할 |
|------|-------|------|
| ② 설계 검토 | **doc-consistency** | 이식 대상 파일 목록이 plan.md의 참조 저장소 항목과 일치하는지 확인 |
| ③ 구현 | _(없음)_ | 코드 이식 및 빌드 설정은 agent 없이 직접 수행 |
| ④ 코드 검토 | **doc-consistency** | 빌드 체크리스트 항목이 모두 완료됐는지 확인 |

---

## 목표

이 Phase가 끝나면 다음이 가능하다.
- Visual Studio에서 솔루션을 열고 빌드 오류 없이 빌드된다.
- 실행 시 "S-Semi 시스템 시작" 메시지만 출력하는 빈 콘솔 앱이 동작한다.
- DummyDataGenerator로 생성한 초기 JSON 파일이 `data/` 폴더에 존재한다.

---

## 이식할 외부 코드

### DataPersistence → `extern/JsonLib/`

경로: `SampleOrderSystem/extern/JsonLib/`

| 파일 | 설명 |
|------|------|
| `include/JsonLib/json.hpp` | JSON 파싱·직렬화 헤더 |
| `src/json.cpp` | 구현부 |

vcxproj에 include 경로(`extern/JsonLib/include`)와 소스 파일(`src/json.cpp`)을 추가한다.

### DataPersistence → `repositories/`

경로: `SampleOrderSystem/repositories/`

| 파일 | 설명 |
|------|------|
| `IRepository.hpp` | 제네릭 CRUD 인터페이스 (findAll, findById, save, update, remove) |
| `JsonRepository.hpp` | IRepository\<T\> 구현체. getId, setId, toJson, fromJson 람다를 생성자에서 주입받음 |

> Product 관련 코드는 이식하지 않는다. 제네릭 템플릿 부분만 가져온다.

### ConsoleMVC → `Controller/`, `View/`

| 파일 | 설명 |
|------|------|
| `Controller/IController.h` | `run()`, `handleCommand()` 순수 가상 인터페이스 |
| `View/IView.h` | `showMessage()`, `showError()`, `promptInput()` 순수 가상 인터페이스 |
| `View/ConsoleView.h/.cpp` | 표준 입출력 구현체 (기본 버전, Phase 5에서 확장) |

### DataMonitor → `View/ConsoleUI.h`

DataMonitor의 `ConsoleUI.h`를 `View/ConsoleUI.h`로 복사한다.  
Windows API(`SetConsoleTextAttribute`) 기반 색상 출력과 테이블 출력 유틸리티를 제공한다.  
Phase 5에서 ConsoleView와 통합한다.

---

## 디렉터리 구조 (Phase 1 완료 시점)

```
SampleOrderSystem/
├── main.cpp                  ← "S-Semi 시스템 시작" 출력 후 종료
├── Controller/
│   └── IController.h
├── View/
│   ├── IView.h
│   ├── ConsoleView.h / .cpp
│   └── ConsoleUI.h
├── repositories/
│   ├── IRepository.hpp
│   └── JsonRepository.hpp
└── extern/
    └── JsonLib/
        ├── include/JsonLib/json.hpp
        └── src/json.cpp

data/
├── samples.json              ← DummyDataGenerator 출력
└── orders.json               ← DummyDataGenerator 출력
```

---

## 초기 데이터 생성

DummyDataGenerator CLI를 사용해 `data/` 폴더에 초기 파일을 생성한다.

```
DummyDataGenerator.exe -s 10 -r 0 -o data/samples.json
```

> 주문(`orders.json`)은 처음에는 비어 있는 배열 `[]`로 시작한다.  
> DummyDataGenerator의 시료 구조체가 이 프로젝트의 Sample 구조체의 기준이 된다.

---

## 빌드 설정 체크리스트

- [ ] vcxproj에 `extern/JsonLib/include` include 경로 추가
- [ ] vcxproj에 `extern/JsonLib/src/json.cpp` 소스 파일 추가
- [ ] C++ 표준: `stdcpp20`
- [ ] 문자셋: Unicode
- [ ] 플랫폼: x64
- [ ] 빌드 성공 (경고 0, 오류 0) 확인

---

## 이 Phase에서 테스트하지 않는 것

Phase 1은 코드 이식과 빌드 설정에 집중한다. 비즈니스 로직 테스트는 Phase 2부터 시작한다.
