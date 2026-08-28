# Chat Protocol

## Transport

- IPv4 TCP
- 기본 주소 `127.0.0.1`
- 기본 port `12345`
- UTF-8 또는 ASCII 범위의 newline-terminated text
- 최대 한 줄 길이 1023byte

## Client commands

| Command | State | Result |
|---|---|---|
| `/list` | lobby or room | room별 현재 인원 반환 |
| `/join N` | lobby or room | room `N`으로 이동 |
| `/leave` | room | lobby로 이동 |
| `/quit` | any | TCP 연결 종료 |
| other line | room | 같은 room의 다른 client에게 broadcast |

지원하지 않는 `/` command는 오류와 command menu를 반환합니다. Lobby에서 일반 메시지를
보내면 먼저 room에 입장하라는 응답을 반환합니다.

## Limits

- 전체 client: 15
- room: 3
- room별 client: 5
- client input buffer: 4096byte

## Deliberate scope

이 사례는 TCP stream framing, event multiplexing과 room state 관리에 집중합니다. 인증,
암호화, 영속 저장, 악성 client rate limiting, binary file transfer는 현재 범위가 아닙니다.
