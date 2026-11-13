# Check the complete interaction loop

Use one action at a time and record both device response and observed effect. Passing a build or receiving “accepted” is not enough to mark a physical action complete.

## Reproduction checklist

| Check | Pass condition |
| --- | --- |
| Board boot | Correct target; no repeated reset or unstable display |
| Screen/controller link | Correct device identity and authenticated HTTP, not merely a Wi-Fi icon |
| Soil | Probe responds, stale values expire, dry/wet references save, valid percentages agree between screen and phone |
| Environment | Light changes with shade, temperature/humidity are plausible and respond, individual validity is respected |
| Water distance | Fixed target produces repeatable readings; installed full/stop/empty references match geometry |
| Reconnect | Existing phone bond survives an ordinary reconnect; a fresh read reaches XIAO |
| Strip | 30-pixel green status, white fill light and return to green; thresholds save |
| Ticket | Seven preview pages; one confirmation; same card/content; normal proportions; full output and observed cut |
| Focus | Start, pause, resume and reset agree between interfaces; verify audible completion separately |
| Pump | Relay first unloaded; correct supply, priming and tubing; timed outflow stops; low/invalid water data prevents or stops it |
| Recovery | Reboot retains appropriate settings; lost responses do not create duplicate actions |

The original prototype's completed results are listed on [roadmap and evidence](roadmap-evidence.md). That list does not automatically validate a reproduction using different sensors, supplies or module revisions.

For a phone command, distinguish: Skill opens → tool connects → request reaches controller → business action result → physical observation. For each failed stage, show the next relevant step instead of repeatedly deleting bonds or printing new test jobs.

