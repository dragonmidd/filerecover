# Project TODO — filerecover

This file is the central project TODO visible in the workspace. It mirrors the managed todo list used by the developer assistant.

Priority items (short-term)

- **Create Project TODO File**: `TODO.md` (this file) — done.
- **Annotate implementations**: Add concise comments to `src/engine/src/*.cpp` (done for key files).
- **Build & test**: Ensure `scripts/build_engine.ps1` configures, builds and runs tests (done locally).
- **Commit comment changes**: Commit the recent annotation changes with a clear message.

Core engine work (medium-term)

- **Implement full MFT parser**: Parse MFT record header, STANDARD_INFORMATION, FILE_NAME, DATA attributes and data runs. Add unit tests in `src/tests/`.
- **Harden DiskIO**: Improve `DiskIO` for device access, overlapped IO, alignment and concurrency.
- **Increase test coverage**: Add unit/integration tests and aim for coverage thresholds.

Infrastructure & release (long-term)

- **Add CI (Windows)**: GitHub Actions workflow to build and run tests on Windows (MSYS2/MinGW or MSVC) and Linux, ensuring runtime availability.
- **Packaging & runtimes**: Decide static vs dynamic runtimes and implement packaging scripts for releases.
- **Enforce docs policy**: Add CI or pre-commit checks to ensure headers/impls are documented per `docs/DEVELOPMENT_GUIDELINES.md`.

Process items

- **Daily end-of-day automation**: `scripts/end_of_day.ps1` exists and the `.vscode` task is configured to run it.

How to use this file

- When you want the assistant to start/complete an item, ask it to: "Start <TODO title>" or "Complete <TODO title>".
- The assistant also mirrors this list in its task tracker and will update statuses as work proceeds.

Contact

- Repo: `d:\project\filerecover`
- Current branch: `main`

