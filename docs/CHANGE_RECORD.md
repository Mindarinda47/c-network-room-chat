# Change Record

2026년 변경을 설명할 때 아래 항목을 본인 문장으로 완성합니다.

## TCP framing

- 원본 증상: 수신 파일 뒤에 79byte의 제어·menu 문자열이 추가됨
- 원인:
- newline framing을 선택한 이유:
- binary protocol이라면 다르게 설계할 부분:
- split·coalesced test 결과:
- 코드를 보지 않고 설명할 수 있는가: [ ]

## Event loop

- 원본 처리 방식:
- `select`로 감시하는 descriptor:
- client별 input buffer가 필요한 이유:
- `recv()==0` 처리:
- slow client가 미치는 영향과 현재 제한:
- 코드를 보지 않고 설명할 수 있는가: [ ]

## Room state

- lobby와 room 상태 표현:
- room capacity 검사:
- 입장·퇴장·disconnect 시 invariant:
- broadcast 대상 선택:
- 코드를 보지 않고 설명할 수 있는가: [ ]

## Verification

- 실행 환경:
- compiler warning:
- unit test 결과:
- integration test 결과:
- 발견 후 수정한 결함:
- 최종 screenshot 위치:
