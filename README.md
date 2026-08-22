- Compatible with Steam save game cloud sync, but don't use on runs that you started in vanilla Barony
- No need to copy/move any files; just start `distributable\Launch Quality Barony.bat`.

<details>
<summary><strong>Install Options</strong></summary>

1) Download this repository as a ZIP and extract it anywhere (not in the game's folder)
2) Or just clone this repository. [GitHub Desktop](https://desktop.github.com) works without an account

</details>

## Features

~~Vanilla~~ | unchanged | `Quality`

<details>
<summary><strong>EXP Multipliers</strong></summary>

| EXP Receiver | Range | Solo | 2 players | 3 players | 4 players |
|---|---:|---:|---:|---:|---:|
| Ordinary follower<br>(other player last hits) | ~~\~6,250 tiles~~<br>`Global` | — | ~~0%~~<br>`108%` | ~~0%~~<br>`96%` | ~~0%~~<br>`84%` |
| Ordinary follower<br>(its player last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~100%~~<br>`120%` | ~~90%~~<br>`108%` | ~~80%~~<br>`96%` | ~~70%~~<br>`84%` |
| Ordinary follower<br>(it last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~200%~~<br>`120%` | ~~190%~~<br>`108%` | ~~180%~~<br>`96%` | ~~170%~~<br>`84%` |
| Conjurer summon<br>(other player last hits) | ~~\~6,250 tiles~~<br>`Global` | — | ~~0%~~<br>`108%` | ~~0%~~<br>`96%` | ~~0%~~<br>`84%` |
| Conjurer summon<br>(its player last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~100%~~<br>`120%` | ~~100%~~<br>`108%` | ~~100%~~<br>`96%` | ~~100%~~<br>`84%` |
| Conjurer summon<br>(it last hits) | ~~\~6,250 tiles~~<br>`Global` | ~~200%~~<br>`120%` | ~~200%~~<br>`108%` | ~~200%~~<br>`96%` | ~~200%~~<br>`84%` |
| Player (last hits) | Global | 100% | 90% | 80% | 70% |
| Player (any follower last hits) | Global | 100% | 90% | 80% | 70% |
| Player (other player last hits) | Global | — | 90% | 80% | 70% |

Player shares have a 3-EXP minimum. Follower gains use integer rounding without a minimum.

</details>

<details>
<summary><strong>Minimap Behavior</strong></summary>

- <code>[untested]</code> <code>Quality marker states are pushed to Quality clients immediately. Unacknowledged updates retry every 0.3 seconds for up to 10 seconds.</code>
- <code>[untested]</code> <code>When a player views a ladder or portal exit dialog, every <img src="distributable/minimap/quality-item-uninteracted.svg" width="18" alt="Unused object, unbroken loot-bearing container, uninteracted pickable item, or loose ground gold">, <img src="distributable/minimap/quality-exit.svg" width="18" alt="Exit">, <img src="distributable/minimap/quality-workbench.svg" width="18" alt="Workbench">, and <img src="distributable/minimap/quality-cauldron.svg" width="18" alt="Cauldron"> is revealed for that player until the next floor. Each player must view the dialog to receive the reveal. Newly eligible objects, including enemy-dropped items and loose gold, appear as they are created. The exit dialog also shows x hostiles / y neutrals alive for the viewing player; followers are excluded.</code>
- <code>Quality markers clear when a floor loads or reloads.</code>

</details>

<details>
<summary><strong>Minimap Legend</strong></summary>

<table>
  <thead>
    <tr><th>Vanilla</th><th>Quality</th><th>Meaning</th><th>Remarks</th></tr>
  </thead>
  <tbody>
    <tr><td><img src="distributable/minimap/unknown.svg" width="18" alt="Dark teal square"></td><td><img src="distributable/minimap/unknown.svg" width="18" alt="Dark teal square"></td><td>Unknown area or no floor</td><td>—</td></tr>
    <tr><td><img src="distributable/minimap/floor.svg" width="18" alt="Teal square"></td><td><img src="distributable/minimap/floor.svg" width="18" alt="Teal square"></td><td>Explored walkable floor</td><td>—</td></tr>
    <tr><td><img src="distributable/minimap/wall.svg" width="18" alt="Cyan square"></td><td><img src="distributable/minimap/wall.svg" width="18" alt="Cyan square"></td><td>Explored wall</td><td>—</td></tr>
    <tr><td><img src="distributable/minimap/mapped-floor.svg" width="18" alt="Dark gray square"></td><td><img src="distributable/minimap/mapped-floor.svg" width="18" alt="Dark gray square"></td><td>Mapped but unseen floor</td><td>—</td></tr>
    <tr><td><img src="distributable/minimap/mapped-wall.svg" width="18" alt="Gray square"></td><td><img src="distributable/minimap/mapped-wall.svg" width="18" alt="Gray square"></td><td>Mapped but unseen wall</td><td>—</td></tr>
    <tr><td><img src="distributable/minimap/highlight.svg" width="18" alt="Pink highlighted square"></td><td><img src="distributable/minimap/highlight.svg" width="18" alt="Pink highlighted square"></td><td>Map highlight</td><td>Blinks briefly.</td></tr>
    <tr><td><img src="distributable/minimap/player-1.svg" width="18" alt="Yellow arrow"></td><td><img src="distributable/minimap/current-player.svg" width="18" alt="White arrow"></td><td>Player 1 / you</td><td><code>In online multiplayer, Quality always shows you in white.</code></td></tr>
    <tr><td><img src="distributable/minimap/players-2-4.svg" width="54" alt="Green, red, and pink arrows"></td><td><img src="distributable/minimap/players-2-4.svg" width="54" alt="Green, red, and pink arrows"></td><td>Players 2–4</td><td><code>In online multiplayer, the other players always use these colors and are never white.</code></td></tr>
    <tr><td><img src="distributable/minimap/followers-1-2.svg" width="36" alt="Dark yellow and green arrows"><br><img src="distributable/minimap/followers-3-4.svg" width="36" alt="Dark red and pink arrows"></td><td><img src="distributable/minimap/quality-followers-1-2.svg" width="25.2" alt="Smaller outlined white and green arrows"><br><img src="distributable/minimap/quality-followers-3-4.svg" width="25.2" alt="Smaller outlined red and pink arrows"></td><td>Followers</td><td><code>In online multiplayer, Quality always shows your followers in white; other players' followers are never white. Shadow tags gray the outline.</code></td></tr>
    <tr><td><img src="distributable/minimap/ghost-players-1-2.svg" width="36" alt="Outlined yellow and green arrows"><br><img src="distributable/minimap/ghost-players-3-4.svg" width="36" alt="Outlined red and pink arrows"></td><td><img src="distributable/minimap/quality-ghost-players-1-2.svg" width="25.2" alt="Smaller outlined white and green arrows"><br><img src="distributable/minimap/quality-ghost-players-3-4.svg" width="25.2" alt="Smaller outlined red and pink arrows"></td><td>Ghosts</td><td><code>In online multiplayer, Quality always shows your ghost in white; other players' ghosts are never white.</code></td></tr>
    <tr><td><img src="distributable/minimap/player-dots-1-2.svg" width="36" alt="Yellow and green dots"><br><img src="distributable/minimap/player-dots-3-4.svg" width="36" alt="Red and pink dots"></td><td><img src="distributable/minimap/quality-player-dots-1-2.svg" width="36" alt="White and green dots"><br><img src="distributable/minimap/quality-player-dots-3-4.svg" width="36" alt="Red and pink dots"></td><td>Player ping or callout</td><td><code>In online multiplayer, Quality always shows yours in white; other players' are never white.</code> Blinks, then fades; callouts blink the targeted player's arrow.</td></tr>
    <tr><td><img src="distributable/minimap/player-skulls-1-2.svg" width="36" alt="Yellow and green skulls"><br><img src="distributable/minimap/player-skulls-3-4.svg" width="36" alt="Red and pink skulls"></td><td><img src="distributable/minimap/quality-player-skulls-1-2.svg" width="36" alt="White and green skulls"><br><img src="distributable/minimap/quality-player-skulls-3-4.svg" width="36" alt="Red and pink skulls"></td><td>Death marker or player loot bag</td><td><code>In online multiplayer, Quality always shows yours in white; other players' are never white.</code> Death markers fade after about 9 seconds.</td></tr>
    <tr><td><img src="distributable/minimap/minotaur.svg" width="24" alt="Large red arrow"></td><td><img src="distributable/minimap/quality-minotaur.svg" width="24" alt="Red skull within a circle"></td><td>Minotaur</td><td><s>Blinks.</s> <code>Does not blink.</code></td></tr>
    <tr><td><img src="distributable/minimap/radius-ping.svg" width="24" alt="Expanding yellow ring"></td><td><img src="distributable/minimap/radius-ping.svg" width="24" alt="Expanding yellow ring"></td><td>Radius ping</td><td>Expands and fades.</td></tr>
    <tr><td><img src="distributable/minimap/bounty-skull.svg" width="24" alt="Yellow bounty skull"></td><td><img src="distributable/minimap/bounty-skull.svg" width="24" alt="Yellow bounty skull"></td><td>Bounty target</td><td>Pulses.</td></tr>
    <tr><td><img src="distributable/minimap/shadow-tag.svg" width="18" alt="Gray dot"></td><td><img src="distributable/minimap/quality-shadow-tag.svg" width="24" alt="Gray skull within a circle"></td><td>Shadow-tagged creature</td><td><code>Also grays player and follower arrows.</code></td></tr>
    <tr><td><img src="distributable/minimap/exit.svg" width="18" alt="Red dot"></td><td><img src="distributable/minimap/quality-exit.svg" width="18" alt="Green running-person exit pictogram"></td><td>Exit, ladder, or portal</td><td><s>Blinks.</s> <code>Does not blink.</code> Appears after exploration or exit reveal.</td></tr>
    <tr><td><img src="distributable/minimap/boulder.svg" width="18" alt="Brown dot"></td><td><img src="distributable/minimap/quality-boulder.svg" width="18" alt="Cyan circle divided by four lines"></td><td>Boulder</td><td>Appears on explored tiles.</td></tr>
    <tr><td><img src="distributable/minimap/station.svg" width="18" alt="Blue dot"></td><td><img src="distributable/minimap/quality-workbench.svg" width="24" alt="Blue circle with a centered W"></td><td>Workbench</td><td><code>[untested]</code> Appears on explored tiles.</td></tr>
    <tr><td><img src="distributable/minimap/station.svg" width="18" alt="Blue dot"></td><td><img src="distributable/minimap/quality-cauldron.svg" width="24" alt="Blue circle with a centered C"></td><td>Cauldron</td><td><code>[untested]</code> Appears on explored tiles.</td></tr>
    <tr><td><img src="distributable/minimap/yellow-dot.svg" width="18" alt="Yellow dot"></td><td><img src="distributable/minimap/yellow-dot.svg" width="18" alt="Yellow dot"></td><td>Highlighted item, donation, or pinpointed target</td><td>Blinks for items and donations; pinpointed targets last for the effect duration.</td></tr>
    <tr><td></td><td><img src="distributable/minimap/quality-item-uninteracted.svg" width="18" alt="Dark olive-green circle"></td><td>Unused chest, grave, fountain, or sink; unbroken loot-bearing container; uninteracted pickable item; loose ground gold</td><td><code>Appears only after that player views an exit dialog, never through exploration. Breakable-container markers are limited to containers that hold an item or gold, such as a jar, urn, crate, or barrel; ordinary furniture such as tables, chairs, and beds is excluded.</code> <code>Loose gold is included regardless of pile size, player origin, or temporary invisibility, but gold still inside a container is excluded.</code> <code>A successful item transfer removes an ordinary chest marker; merely opening the chest does not. Other successful uses remove their interactable markers. If multiple item markers occupy the same minimap tile, this marker is displayed on top.</code></td></tr>
    <tr><td></td><td><img src="distributable/minimap/quality-item-interacted.svg" width="18" alt="Royal blue circle"></td><td>Party-dropped pickable item or nonempty interacted ordinary chest</td><td><code>Chest markers begin after the first successful item transfer, appear while the chest contains at least one item, and disappear while it is empty. Void chests are excluded.</code> <code>Party-item markers follow party-handled items until pickup or removal, keep one identity across pickups and drops, and override other item markers. An item revealed in green stays green when a pickup fails and the game immediately drops it again. Online behavior is untested.</code></td></tr>
    <tr><td><img src="distributable/minimap/purple-dot.svg" width="18" alt="Purple dot"></td><td><img src="distributable/minimap/purple-dot.svg" width="18" alt="Purple dot"></td><td>Detected unit</td><td>Persists while detected by vision, Ring of Warning, or artifact boots; may be hostile or neutral.</td></tr>
    <tr><td><img src="distributable/minimap/scry-dot.svg" width="18" alt="Pale blue dot"></td><td><img src="distributable/minimap/scry-dot.svg" width="18" alt="Pale blue dot"></td><td>Scrying target</td><td>Blinks while scrying is active.</td></tr>
    <tr><td><img src="distributable/minimap/pink-dot.svg" width="18" alt="Pink dot"></td><td><img src="distributable/minimap/pink-dot.svg" width="18" alt="Pink dot"></td><td>Other temporary or gyro-detected target</td><td>Blinks while the reveal is active.</td></tr>
  </tbody>
</table>

</details>
