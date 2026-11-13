# A pixel plant beside a real plant

RoboPlanter's screen makes care visible: start with a seed, choose a species and watch a little companion develop. Mint, basil, rose and succulent choices provide different identities. The screen also carries game health and a desk mood.

## Two different kinds of state

**Physical measurements** come from probes and XIAO's validity/calibration rules. A soil percentage is relative to saved dry/wet references; environmental humidity describes air, not soil.

**Companion state** is a game model. Watering, feeding, weeding and focus activities can express care in the game; they do not prove fertilizer was dosed, weeds were removed or a pump ran. A game-watering action must not silently become a physical relay command.

## Growth without invented biology

The animation and species selector are implemented UI features. A compressed game growth cycle is not a horticultural forecast. Real germination, flowering and fruiting depend on species, maturity, season, light, substrate and other conditions. The long-term design is to connect species guidance with explicit conditions and sources, rather than pretending all plants follow one countdown.

The healthy interaction is an invitation to take a small step, not a threat that the plant will die if someone stops working. Low game health is not a medical or botanical diagnosis.

For source-backed crop guidance and its current prototype boundaries, see [crop knowledge](crop-knowledge.md). For theme, motion and language behavior, see [design system](design-system.md).

