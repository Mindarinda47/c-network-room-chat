# Test Evidence

실제 실행 후 아래 항목을 채웁니다. 실행하지 않은 결과는 PASS로 기록하지 않습니다.

## Environment

- Date:
- OS / distribution:
- Compiler and version:
- Python version:
- Command: `make clean && make test`

## Results

| Check | Result | Evidence |
|---|---|---|
| warning-free build with `-Werror` | 미실행 | |
| `protocol_test` | 미실행 | |
| three-client integration test | 미실행 | |
| split command | 미실행 | |
| coalesced commands | 미실행 | |
| room isolation and broadcast | 미실행 | |
| disconnect and SIGINT cleanup | 미실행 | |

## Screenshot

저장 위치: `docs/images/network-tests.png`

스크린샷에는 build command, warning 0개, `protocol test: PASS`,
`integration test: PASS`가 식별 가능해야 합니다. 사용자명과 로컬 절대 경로는 포함하지
않거나 잘라냅니다.
