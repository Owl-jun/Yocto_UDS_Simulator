# Embedded Linux UDS Diagnostic ECU Simulator

## 프로젝트 소개

Classic AUTOSAR의 DCM(Diagnostic Communication Manager) 구조를 참고하여, **Yocto Linux 기반에서 동작하는 UDS Diagnostic ECU**를 구현하는 프로젝트입니다.

실제 ECU 대신 QEMU에서 부팅되는 Yocto Linux 이미지를 사용하며, systemd를 통해 Diagnostic Daemon을 자동 실행합니다.

TCP 기반 Tester와 통신하여 UDS 서비스를 처리하고, DID 관리 및 Diagnostic Session을 구현하는 것을 목표로 합니다.

---

# 프로젝트 목표

* Yocto Build System 이해
* Recipe(.bb) 작성
* Custom Layer 생성
* systemd Service 등록
* Embedded Linux Application 개발
* Linux Daemon 구조 이해
* UDS Protocol 구현
* AUTOSAR DCM 구조를 Linux 환경에 적용

---

# 개발 환경

### Host

* Windows 11
* WSL2 Ubuntu
* VSCode

### Target

* Yocto Project
* QEMU
* Embedded Linux

### Language

* C++17

### Build

* CMake

---

# 전체 구조

```text
                +----------------------+
                |     Tester(Client)   |
                +----------+-----------+
                           |
                        TCP Socket
                           |
                +----------v-----------+
                |  Diagnostic Daemon  |
                +----------+-----------+
                           |
        +------------------+------------------+
        |                  |                  |
        |                  |                  |
  Transport         UDS Dispatcher      Logger
        |                  |
        |                  |
        |           +------+------+
        |           |             |
        |       DID Manager   Session Manager
        |                         |
        +-------------------------+
```

---

# Boot Flow

```text
Power On

↓

Yocto Linux Boot

↓

systemd

↓

diagnostic.service

↓

diagnosticd 실행

↓

TCP Listen

↓

Tester 연결 대기
```

---

# 프로젝트 디렉터리

```text
diagnostic-ecu/

├── app/
│   ├── main.cpp
│   ├── DiagnosticServer.cpp
│   ├── TcpServer.cpp
│   ├── UdsDispatcher.cpp
│   ├── DidManager.cpp
│   ├── SessionManager.cpp
│   └── Logger.cpp
│
├── include/
│
├── config/
│   └── diagnostic.conf
│
├── systemd/
│   └── diagnostic.service
│
├── yocto/
│   ├── meta-diagnostic/
│   ├── recipes-app/
│   └── diagnostic.bb
│
└── README.md
```

---

# 구현 기능

## 1. TCP Server

* 특정 Port Listen
* Tester 연결
* Command 수신
* Response 송신

---

## 2. UDS Dispatcher

지원 예정 SID

* 0x10 Diagnostic Session Control
* 0x11 ECU Reset
* 0x22 ReadDataByIdentifier
* 0x2E WriteDataByIdentifier
* 0x19 ReadDTCInformation

Dispatcher가 SID를 해석하여 각 Handler로 전달한다.

---

## 3. DID Manager

예시 DID

| DID  | 내용            |
| ---- | ------------- |
| F190 | VIN           |
| F187 | SW Version    |
| F188 | HW Version    |
| F18C | Serial Number |

Map 기반으로 관리한다.

---

## 4. Session Manager

지원 예정

* Default Session
* Extended Session

Session에 따라 접근 가능한 SID를 관리한다.

---

## 5. Logger

기록 내용

* Client Connect
* SID 요청
* SID 응답
* Error
* Session 변경

journalctl 또는 로그 파일을 사용한다.

---

## 6. Config

설정 파일

```text
/etc/diagnostic.conf
```

예시

```ini
Port=5000
LogLevel=INFO
VIN=KMHXXXXXXXXXXXXXX
```

---

# Yocto 작업

## Custom Layer 생성

```text
meta-diagnostic
```

---

## Recipe 작성

```text
diagnostic.bb
```

수행 내용

* Source Build
* Binary Install
* Config Install
* systemd 등록

---

## Image 생성

Custom Image

```text
diagnostic-image
```

생성 후

QEMU에서 Boot

---

# systemd

Service 등록

```text
diagnostic.service
```

Boot 완료 후

자동 실행

자동 재시작 설정 적용

---

# 통신 예시

Request

```text
22 F190
```

Response

```text
62 F190 KMH123456789
```

---

# 향후 확장

* CAN Transport
* ISO-TP
* SOME/IP
* Adaptive AUTOSAR 연계
* Security Access (0x27)
* Routine Control (0x31)
* OTA Stub
* DTC 저장
* Multi Client

---

# 프로젝트를 통해 얻는 기술

* Embedded Linux
* Yocto
* BitBake
* Layer
* Recipe
* systemd
* Linux Daemon
* Socket Programming
* UDS Protocol
* C++
* Software Architecture
* Diagnostic System Design

---

# 최종 목표

> "Classic AUTOSAR DCM에서 사용되는 Diagnostic 구조를 Embedded Linux 환경으로 재구성하여, Yocto 기반 ECU Simulator를 구현한다."

이를 통해 MCU 중심 개발 경험을 Linux 기반 HPC 환경으로 자연스럽게 확장하는 것을 목표로 한다.
