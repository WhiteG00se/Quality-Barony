## important info

- Compatible with Steam save game cloud sync, but don't use with vanilla save games
- No need to copy/move any files; just start via `?`

## install options

1) Download this repository as a ZIP and extract it anywhere (not in the game's folder)
2) Or just clone this repository. [GitHub Desktop](https://desktop.github.com) works without an account

## Features

- ~~Vanilla~~ | unchanged | `Quality`
- Integer rounding and a 3-EXP minimum are considered whenever EXP is gained.

| EXP Receiver | Range | Solo | 2 players | 3 players | 4 players |
|---|---:|---:|---:|---:|---:|
| Ordinary follower<br>(other player last hits) | ~~\~6,250 tiles~~<br>`Global` | — | ~~0%~~<br>`130%` | ~~0%~~<br>`110%` | ~~0%~~<br>`90%` |
| Ordinary follower<br>(its player last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~100%~~<br>`150%` | ~~90%~~<br>`130%` | ~~80%~~<br>`110%` | ~~70%~~<br>`90%` |
| Ordinary follower<br>(it last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~200%~~<br>`150%` | ~~190%~~<br>`130%` | ~~180%~~<br>`110%` | ~~170%~~<br>`90%` |
| Conjurer summon<br>(other player last hits) | ~~\~6,250 tiles~~<br>`Global` | — | ~~0%~~<br>`130%` | ~~0%~~<br>`110%` | ~~0%~~<br>`90%` |
| Conjurer summon<br>(its player last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~100%~~<br>`150%` | ~~100%~~<br>`130%` | ~~100%~~<br>`110%` | ~~100%~~<br>`90%` |
| Conjurer summon<br>(it last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~200%~~<br>`150%` | ~~200%~~<br>`130%` | ~~200%~~<br>`110%` | ~~200%~~<br>`90%` |
| Player (last hits) | Global | 100% | 90% | 80% | 70% |
| Player (any follower last hits) | Global | 100% | 90% | 80% | 70% |
| Player (other player last hits) | Global | — | 90% | 80% | 70% |



### Minimap Behavior

- Quality marker states are pushed to Quality clients immediately. Unacknowledged updates retry every 0.3 seconds for up to 10 seconds.
- When any player views an “Exit dungeon floor” dialog, every <img src="mod/docs/minimap-legend/quality-item-uninteracted.svg" width="18" alt="Uninteracted item">, <img src="mod/docs/minimap-legend/quality-interactable-uninteracted.svg" width="18" alt="Unused object">, <img src="mod/docs/minimap-legend/quality-exit.svg" width="18" alt="Exit">, <img src="mod/docs/minimap-legend/quality-workbench.svg" width="18" alt="Workbench">, and <img src="mod/docs/minimap-legend/quality-cauldron.svg" width="18" alt="Cauldron"> is revealed for the entire team until the next floor.
- Quality markers clear when a floor loads or reloads.

<details>
<summary><strong>Minimap Legend</strong></summary>

<table>
  <thead>
    <tr><th>Vanilla</th><th>Quality</th><th>Meaning</th><th>Behavior</th></tr>
  </thead>
  <tbody>
    <tr><td><img src="mod/docs/minimap-legend/unknown.svg" width="18" alt="Dark teal square"></td><td><img src="mod/docs/minimap-legend/unknown.svg" width="18" alt="Dark teal square"></td><td>Unknown area or no floor</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/floor.svg" width="18" alt="Teal square"></td><td><img src="mod/docs/minimap-legend/floor.svg" width="18" alt="Teal square"></td><td>Explored walkable floor</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/wall.svg" width="18" alt="Cyan square"></td><td><img src="mod/docs/minimap-legend/wall.svg" width="18" alt="Cyan square"></td><td>Explored wall</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/mapped-floor.svg" width="18" alt="Dark gray square"></td><td><img src="mod/docs/minimap-legend/mapped-floor.svg" width="18" alt="Dark gray square"></td><td>Mapped but unseen floor</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/mapped-wall.svg" width="18" alt="Gray square"></td><td><img src="mod/docs/minimap-legend/mapped-wall.svg" width="18" alt="Gray square"></td><td>Mapped but unseen wall</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/highlight.svg" width="18" alt="Pink highlighted square"></td><td><img src="mod/docs/minimap-legend/highlight.svg" width="18" alt="Pink highlighted square"></td><td>Map highlight</td><td>Blinks briefly.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/player-1.svg" width="18" alt="Yellow arrow"></td><td></td><td>Player 1</td><td>Quality changes the marker from yellow to white. Points in facing direction.</td></tr>
    <tr><td></td><td><img src="mod/docs/minimap-legend/current-player.svg" width="18" alt="White arrow"></td><td>Current player</td><td>Quality always shows your marker in white, regardless of player number.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/players-2-4.svg" width="54" alt="Green, red, and pink arrows"></td><td><img src="mod/docs/minimap-legend/players-2-4.svg" width="54" alt="Green, red, and pink arrows"></td><td>Players 2–4</td><td>Point in facing direction.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/followers.svg" width="72" alt="Dark yellow, green, red, and pink arrows"></td><td><img src="mod/docs/minimap-legend/quality-followers.svg" width="50.4" alt="Smaller outlined white, green, red, and pink arrows"></td><td>Followers</td><td>Shadow tags gray the outline.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/ghost-player-1.svg" width="18" alt="Outlined yellow arrow"></td><td><img src="mod/docs/minimap-legend/ghost-current.svg" width="12.6" alt="Smaller outlined white arrow"></td><td>Player 1 ghost</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/ghost-players-2-4.svg" width="54" alt="Outlined green, red, and pink arrows"></td><td><img src="mod/docs/minimap-legend/quality-ghost-players-2-4.svg" width="37.8" alt="Smaller outlined green, red, and pink arrows"></td><td>Player 2–4 ghosts</td><td>—</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/minotaur.svg" width="24" alt="Large red arrow"></td><td><img src="mod/docs/minimap-legend/quality-minotaur.svg" width="24" alt="Red skull within a circle"></td><td>Minotaur</td><td><s>Blinks.</s></td></tr>
    <tr><td><img src="mod/docs/minimap-legend/player-dots.svg" width="72" alt="Yellow, green, red, and pink dots"></td><td><img src="mod/docs/minimap-legend/quality-player-dots.svg" width="72" alt="White, green, red, and pink dots"></td><td>Player ping or callout</td><td>Blinks, then fades; callouts blink the targeted player's arrow.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/radius-ping.svg" width="24" alt="Expanding yellow ring"></td><td><img src="mod/docs/minimap-legend/radius-ping.svg" width="24" alt="Expanding yellow ring"></td><td>Radius ping</td><td>Expands and fades.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/player-skulls.svg" width="72" alt="Yellow, green, red, and pink skulls"></td><td><img src="mod/docs/minimap-legend/quality-player-skulls.svg" width="72" alt="White, green, red, and pink skulls"></td><td>Death marker or player loot bag</td><td>Death markers fade after about 9 seconds.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/bounty-skull.svg" width="24" alt="Yellow bounty skull"></td><td><img src="mod/docs/minimap-legend/bounty-skull.svg" width="24" alt="Yellow bounty skull"></td><td>Bounty target</td><td>Pulses.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/exit.svg" width="18" alt="Red dot"></td><td><img src="mod/docs/minimap-legend/quality-exit.svg" width="18" alt="Green running-person exit pictogram"></td><td>Exit, ladder, or portal</td><td><s>Blinks.</s> Appears after exploration or exit reveal.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/shadow-tag.svg" width="18" alt="Gray dot"></td><td><img src="mod/docs/minimap-legend/quality-shadow-tag.svg" width="24" alt="Gray skull within a circle"></td><td>Shadow-tagged creature</td><td>Also grays player and follower arrows.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/boulder.svg" width="18" alt="Brown dot"></td><td><img src="mod/docs/minimap-legend/quality-boulder.svg" width="18" alt="Cyan circle divided by four lines"></td><td>Boulder</td><td>Appears on explored tiles.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/station.svg" width="18" alt="Blue dot"></td><td><img src="mod/docs/minimap-legend/quality-workbench.svg" width="18" alt="Blue circle with a centered W"></td><td>Workbench</td><td>Appears on explored tiles.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/station.svg" width="18" alt="Blue dot"></td><td><img src="mod/docs/minimap-legend/quality-cauldron.svg" width="18" alt="Blue circle with a centered C"></td><td>Cauldron</td><td>Appears on explored tiles.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/yellow-dot.svg" width="18" alt="Yellow dot"></td><td><img src="mod/docs/minimap-legend/yellow-dot.svg" width="18" alt="Yellow dot"></td><td>Highlighted item, donation, or pinpointed target</td><td>Blinks for items and donations; pinpointed targets last for the effect duration.</td></tr>
    <tr><td></td><td><img src="mod/docs/minimap-legend/quality-item-uninteracted.svg" width="18" alt="Dark olive-green circle"></td><td>Unused chest, grave, fountain, or sink; uninteracted pickable item</td><td>Appears through exploration or exit reveal. Successful use removes interactable markers; item markers override other item markers.</td></tr>
    <tr><td></td><td><img src="mod/docs/minimap-legend/quality-item-interacted.svg" width="18" alt="Royal blue circle"></td><td>Party-dropped pickable item</td><td>Follows party-handled items until pickup or removal; keeps one identity across pickups and drops; overrides other item markers.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/purple-dot.svg" width="18" alt="Purple dot"></td><td><img src="mod/docs/minimap-legend/quality-detected-hostile.svg" width="18" alt="Red dot"></td><td>Detected hostile</td><td>Persists while hostile and detected by vision, Ring of Warning, or artifact boots.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/scry-dot.svg" width="18" alt="Pale blue dot"></td><td><img src="mod/docs/minimap-legend/scry-dot.svg" width="18" alt="Pale blue dot"></td><td>Scrying target</td><td>Blinks while scrying is active.</td></tr>
    <tr><td><img src="mod/docs/minimap-legend/pink-dot.svg" width="18" alt="Pink dot"></td><td><img src="mod/docs/minimap-legend/pink-dot.svg" width="18" alt="Pink dot"></td><td>Other temporary or gyro-detected target</td><td>Blinks while the reveal is active.</td></tr>
  </tbody>
</table>

</details>


## Notes just for developers

- Mod was created for Barony v5.0.2. (`barony.exe` SHA-256 is `8566DA37BC39EA5A1ED08A8AD57608AF4F019FB415869258FB3C1D310B4419E4`)

The launcher automatically searches Steam's registry entries and library folders. If it cannot find Barony, provide the installation directory manually:

```powershell
_Quality_Barony_Launcher.exe --game-dir "D:\path\to\steamapps\common\Barony"
```

Diagnostic commands:

```powershell
_Quality_Barony_Launcher.exe --verify-only
_Quality_Barony_Launcher.exe --test-injection
```
