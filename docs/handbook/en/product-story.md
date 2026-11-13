# A little care in the middle of a workday

Some days, a desk feels like a queue of things that need attention. RoboPlanter puts one small living thing in that queue, then makes caring for it a reason to care for yourself too.

The blocky, Creeper-inspired enclosure holds a plant, a touchscreen, a phone cradle and a thermal-printer outlet. The screen grows a pixel plant from a seed. The real sensors provide measurements; the character supplies personality. Its game health is never presented as a biological diagnosis.

## Three moments

**Arriving:** “Good morning, clocking in.” The scene system is designed to offer a check-in ticket when automatic printing is enabled.

**Checking in:** “How is my plant doing?” YOYO reads the controller's calibrated soil percentage and environmental data. Missing or stale readings remain missing, rather than becoming reassuring invented numbers.

**Taking a breather:** “I've had too many meetings.” A meeting-survivor or break ticket turns a passing remark into a small object. Three empty checkboxes leave room for handwriting. A card from the full 78-card tarot deck, upright or reversed, adds playfulness rather than a prediction about the future.

The scene scripts are implemented; the phone-to-paper acceptance matrix is still incomplete. The screen-to-paper path has been physically confirmed.

## Why three interfaces?

The phone is for conversation. The Indicator is for persistent, glanceable information and deliberate touch actions. Paper is for a moment you can keep after the screen goes dark. The XIAO coordinates them, so calibration, focus state and action results have one source of truth.

This division also makes failure easier to explain: an unavailable Skill entry, an unpaired phone, a disconnected screen and a busy printer are different problems. The interface now exposes connection stages and an ordinary reconnect action that preserves the existing bond.

Start with the [architecture](system-api.md), or follow the [build guide](hardware-quickstart.md).

