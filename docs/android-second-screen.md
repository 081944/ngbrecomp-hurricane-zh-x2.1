# Android second-screen inventory

The Android activity detects presentation displays and creates one `Presentation`
on the secondary display. The game remains in its existing SDL window. The
secondary display uses one fullscreen Canvas View with two pages. The default
page puts melee weapons in the left 56% of the area, with projectiles above
consumables on the right. A bottom-right button expands the secondary page:
Ninpo across the top, armlets/bands below left, costumes below right. An eligible
selection queues one command and collapses the page back to the bottom-right
tile. The tile can reopen it or close it without changing equipment.

Category regions have fixed bounds independent of inventory. Empty categories
keep their background and caption, with no item buttons. Only the item buttons
resize within their category as owned type counts change. Balanced rows fill the region,
including incomplete last rows. There is no overall title/status bar or whole-page
scrolling. Small category captions remain inside each region. The existing game
art, Chinese names, levels/quantities and equipped borders are retained. Armlets
use a resolution-independent Canvas cuff symbol. The control has its own space
below the right-hand grid and never changes size with inventory or page changes.
Drawer reveal/collapse takes 320/260 ms with a rounded reveal from the button,
slight scaling and fading. A page snapshot keeps text measurement and sprite
scaling out of animation frames. Presses are ignored during the transition.

## Map on the second display

Map and More Equipment have equal widths and 52 dp height; together they fill
the right-hand category's footer on both pages, keeping the same bounds across
pages and inventory changes.
It toggles a fullscreen map and does not send controller input or enter the
retail menu. The two footer controls remain accessible; entering equipment closes
the map. A region with no map closes the page and grays out its button; valid
regions automatically enable it again. The underlying equipment grid cannot
receive touches while the map is visible.

The guest snapshots the current map ID at DB932C, which the area's volume
selection updates independently of item ownership (002EE1C0). IDs outside 0–51
have no map. The language-specific 52-row tables at 43A5B8/43A838 provide the
native file ID at row+8; row+10 is a text label and must never index the archive.
Lookup uses the row key because aliases are unordered. The file registry rooted
at 44D430 leads to the file header, whose +8/+12 give the archive list and count.
Each 16-byte archive descriptor holds a ushort file-ID list, count and name.
Find spr and resolve the native file ID to its index within that list. Verified
regression: current region 3 has Japanese file 071D, archive index 212
(satoallj.spr); treating text label 217 as an archive index loaded zpkikanj.spr.
The panel reads the filename from the player's spr.afs, decodes the P8 texture,
and sends it through the existing immutable bitmap path. It never calls the
inventory ownership check 001542C0, grants a map item, or changes saves. The
image fills the available area after trimming transparent padding. Position
markers use the same map transform at 439EC0 as 00209B10/00209CE0, with the
actor's world-space matrix applied before projection. Invalid marker coordinates
are omitted without hiding the map itself.

The packet header retains its original first three columns and appends map ID,
art key, position-valid flag and normalized X/Y. No map uses ID -1. Tests cover
unordered table keys, both languages, empty inventory, map/no-map transitions,
coordinate projection, parser bounds, page toggles and touch geometry. Optional
asset integration checks decode 100 actual map/localized sheets from an ignored
local copy of the player's spr.afs. On-device acceptance remains required.

## Inventory sizing

Counts below describe the supported panel catalog, not every quest/key item.
Upgrades and mission aliases are not additional types.

| Category | Types | Basis |
| --- | ---: | --- |
| Melee | 14 | Includes True Dragon Sword, Unlabored Flawlessness, both Plasma Sabers and the native menu's Spear Gun grouping |
| Projectiles/ranged | 9 | Four thrown tools plus five bow/ammunition configurations (27, 29, 8D, E2, F8) |
| Immediate recovery items | 6 | BE, BF, C0, C1, 17E, 181; automatic revival and quest items are outside this panel |
| Ninpo | 4 | Fire Wheels, Inferno, Ice Storm, Inazuma |
| Armlets/bands | 14 | Seven armlets and seven bands in cheats.c's original All Armlets catalog |
| Costumes | 8 | Plus the separate automatic/story-default action; existing unlock filtering applies |

Armlets are panel category 6, while the controller carousel keeps its four
categories. Match retail 00180DC0/001811DF: native item class 4 equips slot 7
(58A57E plus the player stride). Use the same inventory transition as weapons,
never a direct write into the Ninpo slot. Verify ownership again when dispatching.

The existing Quick Menu setting controls availability. Off clears the panel and
rejects requests. The existing Quick Menu Pause setting pauses only during an
eligible touch selection, not for the lifetime of the visible panel. Touch down
acquires selection; releasing the same button equips/uses once and releases the
pause. Dragging, a second finger, leaving gameplay or disconnecting
the display cancels the selection. A five-second native timeout releases a lost
gesture. A press made before the action gate is ready never executes later.

`android_quick.c` is called from the guest's existing frame callback, including
when the controller menu is closed. Guest register context is saved around the
panel service. The frame callback only queues touch commands; it must never call
the equipment resource transition from inside Swap. The existing native
pause-check task consumes a dedicated second-screen command mailbox, rechecks the
action gate, and performs the transition after the current render task yields. It
publishes inventory snapshots at up to 10 Hz, sending only changed snapshots and
caching each icon. Java methods called through the existing C-to-Java pattern
publish state and expose a bounded FIFO of touch events. Java declares no native
methods. Both sides retain the same 24-bit session identifier, so input from a
previous gameplay/display session cannot execute after reconnect or load.

`quick_panel.c` validates each press and release against current ownership and
usability. Accepted releases queue a command for `quick_step`, which calls
`dispatch_command`, preserving the game's
native inventory resource transition, healing effects, inventory debit and sound.
No equipment or healing routine runs on the Android UI thread. Equipped flags
come from native slots, independently of the carousel cursor (including the spear
gun's special category). Icon decoding is mutex-protected; decoded pixels are
immutable and shared with the existing menu.

Do not dispatch an inventory command from `ngb_quick_tick`/`panel_tick`: those
callbacks run inside `D3DDevice_Swap` on the render task. A Thor thread dump of
the initial implementation showed `ngb_frame_tick -> ngb_quick_tick ->
sub_00166B90 -> sub_002FE1D0` waiting for the resource transition inside that
task. Submit through `ngb_panel_submit` and let `quick_step` perform the existing
transition in the native pause-check task instead. A successful request must
produce a dispatch log followed by an equip/use completion, with subsequent
frame/snapshot progress.

The second-screen mailbox is independent of the carousel's close/reset logic.
A temporarily closed action gate must not consume its request. The optional
touch pause remains held after release until the native task acknowledges the
command. A 250 ms deadline prevents an old tap from executing much later; leaving
gameplay or disabling Quick Menu cancels it and releases the pause.

## Costume requests

The original costume menu is in `costume_menu.c`. `sub_001779F0` records the
highlighted costume, and `sub_00177B60` commits the override at guest address
`0x589761`. `FF` means the area's automatic costume; IDs 0-7 name the eight
character variants. The current loaded costume is at `0x589760`.

The world task `sub_002DF590` services `sub_002DF750`, which calls
`sub_002FC6F0` with the one-shot request (`0xD6A100`) when it is not `FF`, then
clears that byte to `FF`. Idle `FF` is not a reason to disable costume buttons.
The loader wrapper records script-provided defaults separately from panel requests.
That routine
checks the override, skips unchanged costumes, releases/reloads the character
resources, waits for resource jobs and restores the player through `sub_002EA5E0`.
The panel command writes the checked override and arms the one-shot request;
it does not invoke that
loader from the frame callback or the UI thread. A subsequent `[COSTUME] active`
log shows the loaded ID. Actual model/animation/state preservation remains an
on-device acceptance check, not something established by compilation.

Costumes follow the original unlock flags through `sub_001DB520`. With All
Costumes off, the panel offers the automatic costume and earned costumes 4-7.
With it on, all eight explicit variants are offered as well. Extra variants
share the game's default costume art and have distinct numbered labels. The
controller carousel retains its four original categories; only the panel uses
the fifth view. Returning to automatic uses the `剧情默认` button. Requests are
disabled for unsupported player types or while a request is pending. Only the
automatic button requires an observed default for the current area. Panel-triggered
loads do not overwrite that default, so automatic can restore the script's choice.
Initial character loading can bypass the costume-change hook. While the native
override is FF and the character is idle, the panel seeds a missing area default
from the currently loaded model before accepting the first manual selection.
An explicit override or an in-flight load cannot establish that baseline. Tests
cover startup without a loader callback, manual selection and return to default.

For panel-triggered reloads, gameplay suspension now precedes resource teardown
(the retail path suspends later). Before resuming, restore the saved world basis
and position, including separate motion-history snapshots at 8C0/8D0/8E0,
18E0/1910/20A0/20D0. Each location restores its own saved value, rather than
copying the current world position over every sample. The local bone vectors
at 1120/1150 remain the loader's values. Unchanged costumes
skip this transaction. Resource lookup failure also balances the early suspension.
Script-triggered changes retain the retail order and transforms. Logs record the
root before and after restoration. The user confirmed the final costume switching
and panel behavior on AYN Thor before the release was prepared.

Slot zero has a separate animation cache at 809980 (stride 2650). The retail
animation task's model-ID check can miss costume reloads that retain that ID.
Before resuming, invalidate its rotating pose buffers' frame tags, rebind native
model metadata/constraints through 001919F0, and resolve the current animation
through 00188C40/00188E70. Clear the inter-pose blend counter at cache+2644 and
sample through 0018EDF0, the direct path in 001914D0. Build the matching skeleton
matrices and finish the animation step before collision/gameplay can run again.
The current animation restarts at its first frame. Preserve the original guest
CPU and animation-task globals around this synchronous operation. This avoids
copying old bone transforms or stale resource pointers into the new model.

## Checks

Run `tools/android/test-quick-panel.ps1` from PowerShell 7. It checks native event
validation, exactly-once release, cancellation, stale sessions, blocked actions,
usability changes, both pause settings and lost-release recovery. Costume checks
cover idle and pending triggers, all eight variants, automatic restoration and
area changes.
The loader harness executes the actual lifted costume routine with controlled
resource jobs, checking suspension order, balanced suspension and guest stack,
root restoration, fresh-pointer preservation, repeated selections and failure paths.
Skeletal stubs verify cache invalidation, metadata/script/direct-sample order,
zero blend before sampling, preservation of local bone vectors and task context,
and completion while gameplay remains suspended. These are orchestration checks,
not an on-device visual test.
Java checks
the actual packet parser, request FIFO and layout geometry for 1240x1080,
1080x1240, 1920x1080 and 720x480. It creates only intermediate test outputs under
`build/secondary-check`; no APK is built or installed.

Compile the normal Android native target and the activity with SDL's Java sources
against the Android platform jar. The user accepted the final build on AYN Thor.
Future on-device regression checks should observe all six equipment groups,
compare highlights/quantities to the native inventory, test one use per tap,
drag cancellation, both pause settings, Quick Menu Off, attack/cutscene/load
guards, sleep/resume, display reconnect and map/no-map transitions.
Use scratch game/settings/save folders and muted audio for automated gameplay
checks as required by AGENTS.md.
