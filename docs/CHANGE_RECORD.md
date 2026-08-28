# Change Record

2023년 보존 과제와 2026년 독립 재구현을 대조해 기록한 변경 사항입니다.

## TCP framing

- 원본 증상: 수신 파일 뒤에 79byte의 제어·menu 문자열이 추가됨
- 원인: TCP byte stream이 `send()`별 message boundary를 보존한다고 가정
- newline framing을 선택한 이유: 현재 범위가 text command와 chat message이기 때문
- binary protocol이라면 다르게 설계할 부분: type·payload length header와 exact-length receive
- split·coalesced test 결과: `protocol_test`와 integration test PASS

## Event loop

- 원본 처리 방식: nonblocking `recv()` 반복 호출 또는 disconnect 처리가 부족한 `select`
- `select`로 감시하는 descriptor: server listener·client socket, client stdin·server socket
- client별 input buffer가 필요한 이유: 연결마다 서로 다른 partial line을 보존하기 위해
- `recv()==0` 처리: room 이탈 후 socket close와 client state 초기화
- slow client가 미치는 영향과 현재 제한: blocking send가 event loop를 지연할 수 있으며
  향후 nonblocking output queue가 필요

## Room state

- lobby와 room 상태 표현: `room == -1`은 lobby, `0..2`는 참여 room
- room capacity 검사: 상태 변경 전에 room별 5명 제한 확인
- 입장·퇴장·disconnect 시 invariant: client는 동시에 하나의 room에만 속함
- broadcast 대상 선택: 같은 room의 연결된 client 중 sender를 제외

## Verification

- 실행 환경: GitHub Codespaces Linux
- compiler warning: `-Werror` build PASS
- unit test 결과: `protocol test: PASS`
- integration test 결과: `integration test: PASS`
- 검증 범위: split·coalesced command, byte preservation, room isolation, broadcast, disconnect
