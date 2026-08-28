# Portfolio Completion Checklist

원본 과제 폴더가 아니라 이 저장소에서만 수정합니다.

## P0 - 실행 검증

- [ ] Linux 또는 WSL에서 `make clean && make test`
- [ ] compile warning 0개 확인 (`-Werror`)
- [ ] `protocol test: PASS`
- [ ] `integration test: PASS`
- [ ] 실행 환경과 결과를 `docs/TEST_EVIDENCE.md`에 기록
- [ ] 개인정보 없는 test screenshot 추가

## P1 - 코드 재검토

### 1. Slow client와 server blocking 가능성

- 파일: `src/server/server.c`
- 함수: `broadcast_room`
- 위치: 각 client에 `chat_send_text`를 호출하는 반복문

현재 socket은 blocking mode이므로 수신하지 않는 client가 send buffer를 가득 채우면 server
event loop가 멈출 수 있습니다. 기본 과제 규모에서는 허용할 수 있지만, 공개 README의
제한사항으로 남기거나 nonblocking output queue를 구현해야 합니다.

### 2. Signal API

- 파일: `src/server/server.c`
- 함수: `main`
- 위치: `signal(SIGINT, ...)`, `signal(SIGTERM, ...)`

Linux에서 더 명시적인 동작을 원하면 `sigaction`으로 변경합니다. handler 안에서는 flag만
변경한다는 원칙을 유지합니다.

### 3. Protocol 확장 여부

- 파일: `src/common/protocol.c`, `docs/PROTOCOL.md`

현재 newline framing은 text chat에 적합합니다. binary packet까지 보여주려면 type과
payload length를 network byte order로 기록하는 header 방식과 `recv_exact` test를 별도
단계로 추가합니다. 필수는 아닙니다.

## P2 - 공개 품질

- [ ] README의 미실행·준비 표현을 실제 결과로 교체
- [ ] `docs/CHANGE_RECORD.md`를 본인 문장으로 작성
- [ ] `local-review/`가 Git에서 ignored인지 확인
- [ ] 교수 제공 자료·보고서·원본 screenshot·binary가 없는지 확인
- [ ] 개인정보·자격증명 검사
- [ ] LICENSE 선택
- [ ] Git 이력에 2023년 제출과 2026년 재구현을 혼동시키는 backdate가 없는지 확인
- [ ] 면접에서 TCP stream, partial send, `select`, disconnect, room invariant 복습

## 공개 판정

P0가 모두 통과하고 P2의 공개 범위 검사가 끝나기 전에는 GitHub 업로드 후보로 판단하지
않습니다. P1의 slow-client 개선을 하지 않는다면 README에 제한사항으로 명시합니다.
