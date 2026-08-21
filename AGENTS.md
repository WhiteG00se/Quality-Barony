# Quality Barony agent instructions

## Repository boundaries

- `Barony-Repo/` is a separate Git repository nested inside this repository. It contains the source code of Barony and is reference material only.
- AI agents must never create, edit, format, generate, move, replace, or delete any file or directory inside `Barony-Repo/`.
- Only the human may pull new code or otherwise update `Barony-Repo/`.
- Read-only inspection and searching inside `Barony-Repo/` are allowed when needed to understand the game.
- Never create, edit, replace, or delete files in the installed copy of Barony.

## Compatibility

- The mod was created for Barony v5.0.2. The SHA-256 of `barony.exe` is `8566DA37BC39EA5A1ED08A8AD57608AF4F019FB415869258FB3C1D310B4419E4`.

## Repository layout

- `distributable/` contains every file required to run Quality Barony. Completed executables, DLLs, and runtime assets are written directly here and are not duplicated elsewhere.
- `source/quality/` contains Quality Barony source code, while `source/tests/` contains its tests.
- `scripts/` contains project automation.
- `.local/` is an ignored workspace for third-party tools, dependencies, and temporary working files. It never contains completed distributable files.
- `Barony-Repo/` is the human-maintained, read-only reference repository for the Barony source code.
- Keep every compiled or playable project-owned file under `distributable/`.
- Write completed executables, DLLs, and other runtime artifacts directly to `distributable/`. Never create a second build, release, staging, or backup copy elsewhere in the repository.
- Use `.local/` only for ignored third-party tools, dependencies, and temporary working files. Never place a completed runtime artifact there.

## Evidence-first development

- Use `Barony-Repo/` as the primary source of truth for game types, functions, data flow, and behavior.
- Keep user-facing documentation aligned with the verified behavior of the project.
- Keep each distributable asset in one canonical location and reference that file wherever it is needed.
- If a request conflicts with these boundaries, stop and explain the conflict instead of working around it.
