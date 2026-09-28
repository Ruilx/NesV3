# CHR Cache Experiment Snapshot

- Baseline commit: `4e16947` (`Snapshot MMC1 and rendering cache baseline`)
- Baseline branch: `master`
- Experiment branch: `exp/2026-09-29_chr-cache`
- Created: 2026-09-29

## Scope

This branch experiments with reducing rendering work while preserving:

- real CPU and MMC1 serial writes;
- the 2x2 nametable debug layout;
- 3840 individual Tile QGraphicsItems and the white viewport frame;
- CHR-ROM and CHR-RAM correctness;
- the existing mapper and PPU behavior.

The first targets are physical CHR bank caching, deduplicated dirty-tile collection, and explicit model/item invalidation. QImage/QPixmap implicit sharing is not treated as an automatic refresh mechanism.

## Context Snapshot

The current renderer already caches decoded pattern tiles for the active PPU mapping and suppresses redundant mapper mapping notifications. The remaining experiment is to reduce cache invalidation, duplicate pixel/image work, and scanning of the 3840-tile dirty array without changing emulated bus behavior.

The session plan calls for these stages:

1. establish and retain performance counters;
2. organize CHR pixel caching by physical bank and generation;
3. use a dirty queue or bitmap so each tile is collected once;
4. add a TileModel/TileAtlas layer while retaining individual Tile items;
5. cache final sprite images separately from sprite positions.

Repository memory records the same constraints and the Qt fact that implicit image sharing only reduces copies; model changes still require explicit `update()` notifications.

## Validation

```powershell
D:\Qt\Tools\CMake_64\bin\cmake.exe --build build-qt6 --parallel 4
ctest --test-dir build-qt6 --output-on-failure
```

Do not add ROMs, save files, runtime logs, or build directories to this experiment commit.

## Rollback

The baseline remains available on `master` at `4e16947`. To stop the experiment without rewriting history:

```powershell
git switch master
git pull --ff-only
```

The experiment branch can remain on the remote for comparison. If the experiment succeeds, merge `exp/2026-09-29_chr-cache` into `master` after the full regression and visual checks.
