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
<summary><strong>Generic Changes</strong></summary>

- <code>When friendly fire is disabled, non-player units other than followers and summons no longer treat one another as hostile and cannot damage one another. A Confused or Drunk attacker retains vanilla hostility and bypasses friendly-fire protection (same goes for players and their followers & summons).</code>
- <code>The top-right follower roster also shows teammates' followers and summons after your own. Teammate units are display-only and use an owner prefix such as T's; they cannot be selected or commanded.</code>

</details>

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

Whenever an EXP-granting non-player unit other than a follower or summon dies after taking at least 1 damage from a player, player-owned follower or summon, or an attacker that was Confused or Drunk at the time, all players and eligible followers receive normal party-kill EXP. The qualifying unit does not need to land the final blow.

Player shares have a 3-EXP minimum. Follower gains use integer rounding without a minimum.

</details>

<details>
<summary><strong>Minimap Behavior</strong></summary>

- <code>Quality synchronization uses Barony's common packet transport for direct connections, Steam P2P, and Crossplay. Clients repeat a slot-based handshake until the host's current floor is confirmed, then receive chest states, exit counts, creature sightings, and follower data. Counts are sent immediately, whenever they change, and as a periodic recovery heartbeat.</code>
- <code>[untested]</code> <code>A living hostile that is inside a player or ghost's real camera view, illuminated in that viewer's player-specific lightmap, and not blocked by a wall or closed sight-blocking entity appears as a solid purple dot for the whole Quality party. This follows darkness, sneaking vision, carried lights, full invisibility, dithered visibility, and telepathy. A sighting disappears when nobody can currently see the creature; vanilla minimap-only detection effects remain separate.</code>
- <code>Identified ordinary ground items and identified-only nonempty chests are blue and map-wide immediately. When a player views a ladder or portal exit dialog, green markers for unidentified ordinary items, loose gold, loot-bearing breakable containers, and chests containing any unidentified item become map-wide for that player. The same reveal enables shiny-yellow markers for all four artifact orbs and unused graves or usable fountains/sinks, and preserves the existing exit, workbench, and cauldron reveal. Each player must view the dialog independently. Rough Rocks, player loot bags, empty chests, and void chests are excluded. Mixed chests are green, and chest colors update whenever their contents change.</code>
- <code>That exit dialog also shows the host-authoritative x hostiles / y neutrals alive count for the viewing player; followers are excluded. A client shows “Enemy count syncing...” instead of a false 0/0 until the first host snapshot arrives. Once that player's hostile count reaches two or fewer, all living hostiles and neutrals remain visible as purple dots until the next floor, even if the count later rises. A ghost entering the exit's 3×3 tile neighborhood triggers the same full reveal and sees the live count persistently below the minimap until leaving the area.</code>
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
    <tr><td><img src="distributable/minimap/station.svg" width="18" alt="Blue dot"></td><td><img src="distributable/minimap/quality-workbench.svg" width="24" alt="Blue circle with a centered W"></td><td>Workbench</td><td>Appears on explored tiles.</td></tr>
    <tr><td><img src="distributable/minimap/station.svg" width="18" alt="Blue dot"></td><td><img src="distributable/minimap/quality-cauldron.svg" width="24" alt="Blue circle with a centered C"></td><td>Cauldron</td><td>Appears on explored tiles.</td></tr>
    <tr><td><img src="distributable/minimap/yellow-dot.svg" width="18" alt="Yellow dot"></td><td><img src="distributable/minimap/yellow-dot.svg" width="18" alt="Yellow dot"></td><td>Highlighted item, donation, or pinpointed target</td><td>Blinks for items and donations; pinpointed targets last for the effect duration.</td></tr>
    <tr><td></td><td><img src="distributable/minimap/quality-item-uninteracted.svg" width="18" alt="Dark olive-green circle"></td><td>Unidentified ordinary item; loose gold; loot-bearing breakable container; chest containing any unidentified item</td><td><code>Map-wide only after that player views an exit dialog. Mixed chests are green. Breakable containers appear only while they hold an item or gold. Rough Rocks and player loot bags are excluded.</code></td></tr>
    <tr><td></td><td><img src="distributable/minimap/quality-shiny-yellow.svg" width="18" alt="Gold circle with a white sparkle"></td><td>Artifact orb; unused grave; usable fountain or sink</td><td><code>Map-wide only after that player views an exit dialog. Orb classification overrides identification. Used interactables disappear.</code></td></tr>
    <tr><td></td><td><img src="distributable/minimap/quality-item-interacted.svg" width="18" alt="Royal blue circle"></td><td>Identified ordinary item; chest containing at least one identified item and no unidentified items</td><td><code>Map-wide immediately, without viewing an exit dialog. Chest markers change dynamically between green, blue, and hidden. Empty and void chests are excluded.</code></td></tr>
    <tr><td><img src="distributable/minimap/purple-dot.svg" width="18" alt="Purple dot"></td><td><img src="distributable/minimap/purple-dot.svg" width="18" alt="Purple dot"></td><td>Detected unit</td><td>Persists while detected by vanilla effects. <code>Quality also uses it for hostiles currently visible in any party member's camera and player-specific lightmap, and for the viewing player's latched final hostile/neutral reveal.</code></td></tr>
    <tr><td><img src="distributable/minimap/scry-dot.svg" width="18" alt="Pale blue dot"></td><td><img src="distributable/minimap/scry-dot.svg" width="18" alt="Pale blue dot"></td><td>Scrying target</td><td>Blinks while scrying is active.</td></tr>
    <tr><td><img src="distributable/minimap/pink-dot.svg" width="18" alt="Pink dot"></td><td><img src="distributable/minimap/pink-dot.svg" width="18" alt="Pink dot"></td><td>Other temporary or gyro-detected target</td><td>Blinks while the reveal is active.</td></tr>
  </tbody>
</table>

</details>
