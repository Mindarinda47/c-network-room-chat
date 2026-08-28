# C Network Programming Case Study

## 한눈에 보기

| 항목 | 내용 |
|---|---|
| 개발 시기 | 2023년 학부 과제 경험, 2026년 재검토·재구현 |
| 형태 | 개인 전공 기술 사례 |
| 언어 | C, Python(test) |
| 핵심 기술 | TCP, AF_UNIX 경험, `select`, stream framing, room state |
| 현재 상태 | source·test 작성 완료, Linux runtime 검증 필요 |

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

## 포트폴리오 역할

대형 핵심 프로젝트가 아니라 운영체제·C·네트워크 기초를 보여주는 전공 기술 사례로
배치합니다. 게임 서버에서 필요한 연결 상태, room membership, broadcast, packet
boundary 설명을 면접에서 코드 없이 설명할 수 있어야 합니다.
