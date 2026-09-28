# Game design

Everything about **what** the game is and **why**, as opposed to how the code works (see `CLAUDE.md` and the code for that). Work is tracked on the `ft_minecraft` GitHub Project.

Working title: **ft_minecraft**. Name candidates are in [identity/names.md](identity/names.md).

## Start here

- [vision/overview.md](vision/overview.md): the whole game on one page.

## Sections

| Folder | Contents |
|---|---|
| [vision/](vision/) | The pitch, pillars and what the game is not. |
| [world/](world/) | The setting: the three suns, eras, zeniths, planets and life. |
| [gameplay/](gameplay/) | What players do: survival, building, exploration, threats, co-op. |
| [progression/](progression/) | The research tree, the end goal and what comes after it. |
| [technical/](technical/) | Design-level technical decisions that shape the game (not code documentation). |
| [identity/](identity/) | Name, and later art direction, audio and tone. |
| [decisions/](decisions/) | The decision log: what we chose, why, and what we rejected. |
| [open-questions.md](open-questions.md) | Everything still undecided. |

## Keeping it up to date

These documents are maintained with an AI agent, which follows the rules below on every change. **Prefer asking the assistant to make changes** instead of editing by hand: one idea usually touches several files (the topic, the decision log, the open questions and the indexes), and the assistant keeps them all consistent.

If you do edit by hand, apply the rules below yourself, especially updating every affected `README.md` and the decision log.

## Rules for these documents

- **Always update the indexes.** Adding, renaming, moving or removing a file means updating this README and the `README.md` of its folder in the same change. Every document must be reachable from here.

- **Describe the game, not the story.** There is no narrator, lore or quest book; if something can only be explained with a story, it doesn't belong in the game.
- **Record decisions in [decisions/](decisions/)** with the reasons and the rejected alternatives, so rejected ideas don't come back without a new argument.
- **Mark anything unsettled as _(open)_** and list it in [open-questions.md](open-questions.md). Resolve it there first, then update the topic document and add a decision.
- **Add new topics as new files** in the matching folder (e.g. `gameplay/farming.md`), and a new folder only when a group of topics doesn't fit anywhere (e.g. `audio/`). Link every new file from this README or its folder's README.
- **Nothing here is final.** Documents describe the current direction; changes go through the decision log.
