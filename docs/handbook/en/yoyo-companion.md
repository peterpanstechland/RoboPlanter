# Talk to the desk companion

YOYO supplies conversation and the official BLE tool. XIAO supplies real measurements and executes validated commands. The Indicator shows connection stages and handles touch interactions. The phone does **not** pair with the Indicator.

## First connection

Use the private Skill entry currently named **RoboPlanter工位小芽** on the development phone. On the Indicator, open Settings → Planter connection → YOYO and scene tickets → Show six-digit pairing code. Pair the phone with the XIAO's `RoboPlanter-…` name using the current code. Codes expire; do not reuse an old screenshot or paste a pairing code into a conversation.

Binding is saved. During ordinary use, “bound; phone connects on demand” is a normal idle state, not an error. If a command cannot connect, try Reconnect phone (**keep binding**) before considering a full re-pair. Replace the binding only when needed, such as changing phones.

## Conversation and action examples

| Intent | Example request | Behavior |
| --- | --- | --- |
| Just talk | “I've had too many meetings.” | A short, context-aware reply; no invented readings |
| Read soil | “What is the soil moisture percentage?” | Report XIAO's calibrated percentage, not raw ADC |
| Check the environment | “How is my desk garden doing?” | Read valid soil and environmental values |
| Light | “Turn on the light.” | Request white fill light; confirm from the business result |
| Focus | “Start a 25-minute focus session.” | Start the shared XIAO timer; screen displays the same session |
| Timer control | “Pause it.” / “Resume.” / “How long is left?” | Act on the visible session context |
| Ticket | “Print a little break ticket for me.” | Create a request ID and keep the ticket data stable |
| Automatic scenes | “Turn automatic scene printing off.” | Persist the preference on XIAO |

English phrasing here explains the interaction; it is not a claim that every phrase or phone-language setting has passed field testing. The shipped Skill instructions are currently Chinese. [Skill setup](honor-platform.md) explains configuration and testing.

## Scene tickets

The implemented scene vocabulary covers check-in, lunch, meetings, breaks and low energy. When automatic printing is enabled, scene matches can request a ticket; when disabled, ordinary conversation continues. Cooldowns and request deduplication reduce accidental repeats. A mention of work stress does not justify a health diagnosis.

The ticket combines plant state, handwritten to-dos and a card from all 78 tarot cards, with upright/reversed orientation. The selection is frozen for the request so the preview and printout agree. Tarot is entertainment and a reflection prompt.

## What has actually been tested?

The phone has returned soil/temperature/humidity, and subsequently a calibrated soil percentage close to the screen. Reconnection while preserving the bond has passed. The Indicator-to-XIAO-to-BLE-printer loop has passed, including proportions and cutting. The complete phone scene-printing/focus-timer matrix still needs separate checks. Voice-started watering is not exposed by this Skill.

If “hello” itself cannot open the Skill, check installation/publication state before deleting Bluetooth bonds. If chat works but a hardware request fails, inspect the connection stage and action result. See [troubleshooting](troubleshooting.md).

