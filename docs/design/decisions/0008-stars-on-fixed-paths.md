# 0008: Stars move on fixed, time-based paths

- **Date:** 2026-09-28
- **Status:** Accepted

## Decision

The stars' positions are computed from time on fixed paths, not simulated. The dance looks chaotic but is fully predictable. Details: [technical/star-system.md](../technical/star-system.md).

## Why

- Every multiplayer client sees the same sky.
- Saves resume exactly.
- "Predicting the eras" in the research tree requires the eras to be predictable.

## Rejected alternatives

- **Real three-body simulation:** unpredictable, hard to sync and to save, and it makes locations like the system's centre hard to find.
