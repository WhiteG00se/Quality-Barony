# Quality Barony agent instructions

## Repository boundaries

- `Barony-Repo/` is a separate Git repository nested inside this repository. It contains the source code of Barony and is reference material only.
- AI agents must never create, edit, format, generate, move, replace, or delete any file or directory inside `Barony-Repo/`.
- Only the human may pull new code or otherwise update `Barony-Repo/`.
- Read-only inspection and searching inside `Barony-Repo/` are allowed when needed to understand the game.
- Never create, edit, replace, or delete files in the installed copy of Barony.

## Evidence-first development

- Use `Barony-Repo/` as the primary source of truth for game types, functions, data flow, and behavior.
- Keep user-facing documentation aligned with the verified behavior of the project.
- If a request conflicts with these boundaries, stop and explain the conflict instead of working around it.
