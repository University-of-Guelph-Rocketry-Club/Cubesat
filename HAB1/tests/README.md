# Tests

Testing is organized by what the test needs:

- `unit/` requires no physical hardware;
- `integration/` checks multiple software components together;
- `hardware/` contains bench procedures and checklists; and
- `data/` contains small known-good and intentionally bad fixtures.

The initial repository provides organization and test vectors. Automated test tooling will be chosen with the firmware and ground-language toolchains.
