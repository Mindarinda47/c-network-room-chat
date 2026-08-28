# Independent Tests

These tests were written for the 2026 reconstruction and are not copied from
course materials.

## Coverage

- split command reassembly across multiple `recv` calls
- separation of multiple commands coalesced into one `recv`
- input-buffer capacity enforcement
- exact byte preservation by `send_all`
- three simultaneous TCP clients
- lobby, room join, room isolation, chat broadcast, and leave
- client connection cleanup and graceful server termination

## Run

```sh
make clean
make test
```

The runtime result has not yet been recorded in this repository. After running
the suite, add a privacy-safe screenshot at `docs/images/network-tests.png` and
record the OS, compiler, and test output in `docs/TEST_EVIDENCE.md`.
