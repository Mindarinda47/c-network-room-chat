# C Network Programming Case Study

## 한눈에 보기

| 항목 | 내용 |
|---|---|
| 개발 시기 | 2023년 학부 과제 경험, 2026년 재검토·재구현 |
| 형태 | 개인 전공 기술 사례 |
| 언어 | C, Python(test) |
| 핵심 기술 | TCP, AF_UNIX 경험, `select`, stream framing, room state |
| 현재 상태 | source·독립 test 작성 및 GitHub Codespaces Linux 검증 완료 |

## 기술적 문제

기존 파일 전송은 종료 sentinel과 다음 menu가 같은 `recv()`로 합쳐지면서 수신 파일에
프로토콜 문자열이 포함되었습니다. 비동기 채팅은 nonblocking socket을 사용했지만
busy polling과 불완전한 disconnect 처리가 남아 있었습니다.

## 해결 접근

1. 제출 source, 실행 화면, 실제 전송 파일을 서로 대조했습니다.
2. 파일 내용 자체가 아니라 원본 이후에 동일한 79byte가 반복 추가된 패턴을 확인했습니다.
3. TCP stream에서 application message boundary를 직접 정의해야 한다고 판단했습니다.
4. client별 누적 buffer와 newline framing을 구현했습니다.
5. busy polling을 `select` 기반 server·client event loop로 교체했습니다.
6. 분할·병합 send를 의도적으로 발생시키는 독립 test를 설계했습니다.

## 대표 문제해결 시나리오

### 1. 정상처럼 보였던 파일 전송 결과의 경계 오류 추적

- **문제:** 2023년 실행 화면에서는 파일 전송이 완료된 것처럼 보였지만 수신 파일이
  원본과 정확히 일치하는지는 검증되지 않았습니다.
- **판단:** byte 비교 결과 두 수신 파일 모두 원본을 동일한 prefix로 포함하고 뒤에
  `EOF`와 다음 menu로 구성된 79byte가 추가됐습니다. TCP가 `send()`별 메시지 경계를
  보존한다고 가정한 것이 원인이었습니다.
- **조치:** 2026년 text protocol은 client별 누적 buffer와 newline framing을 사용하고,
  송신은 `chat_send_all()`로 전체 byte가 처리될 때까지 반복하도록 설계했습니다.
- **검증:** split·coalesced command와 NUL byte 포함 payload를 독립 test로 확인했습니다.

### 2. Busy polling을 `select` event loop로 전환

- **문제:** 2023년 비동기 채팅은 nonblocking `recv()`를 반복 호출해 읽을 데이터가
  없어도 계속 socket을 확인하는 구조였습니다.
- **판단:** client 수가 늘어나면 불필요한 polling과 상태 관리 복잡도가 커질 수 있지만,
  당시 CPU 수치를 측정한 자료는 없으므로 정량 성능 향상은 주장하지 않습니다.
- **조치:** server는 listener와 모든 client socket을 하나의 `select()` loop로 감시하고,
  client는 stdin과 server socket을 동시에 감시하도록 재구현했습니다.
- **검증:** 세 client의 동시 접속, 방 입장, broadcast와 격리를 통합 test로 확인했습니다.

### 3. 연결 종료와 Room 상태를 하나의 정리 경로로 통합

- **문제:** 과거 다중 client 코드에는 종료 조건 오류와 disconnect descriptor 정리 부족이
  있어 socket 종료와 application room 상태가 어긋날 수 있었습니다.
- **판단:** socket만 닫고 membership을 남기면 방 인원과 broadcast 대상이 잘못됩니다.
- **조치:** `/quit`, `recv()==0`, input buffer 초과가 모두 `disconnect_client()`를 거치며,
  기존 방 이탈·socket close·client state 초기화를 같은 순서로 수행하도록 했습니다.
- **검증:** `/quit` 후 연결 종료, 방 이탈 알림과 SIGINT server cleanup을 통합 test가
  확인합니다.

## 포트폴리오 역할

대형 핵심 프로젝트가 아니라 운영체제·C·네트워크 기초를 보여주는 전공 기술 사례로
배치합니다. 게임 서버에서 필요한 연결 상태, room membership, broadcast, packet
boundary 설명을 면접에서 코드 없이 설명할 수 있어야 합니다.
