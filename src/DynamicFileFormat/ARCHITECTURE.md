# Dynamic File Format internals

This document explains *how* the BifrostUsd Dynamic File Format plugin is implemented
internally: how `BifrostDynamicFileFormat`, `BifrostDffData`/`BifrostDffDataImpl`, and
`dffExecuteGraphFrameToLayer` fit together to lazily evaluate a Bifrost graph as a USD
dynamic payload, including the frame-stepping/caching model and the
reset-and-replay-on-backward-scrub behavior.

For the user-facing documentation (how to author a Dynamic Payload in a `.usda` file,
the metadata fields, attributes, etc.), see [README.md](README.md). For a closer look
specifically at the persistent `GraphExecutor`'s lifetime (when it's created, when it
executes, and when `executeGraphFrameToLayer` is used instead of `advanceGraphFrame`),
see [EXECUTOR_LIFETIME.md](EXECUTOR_LIFETIME.md).

## The analogy: a kitchen with one slow-cooker

Picture a restaurant that doesn't pre-cook every dish for every day of the year and
stack them in a walk-in fridge. Instead, the kitchen only cooks a dish when an actual
order comes in for a specific day — that's the **lazy evaluation** half of this design
(older versions of this plugin baked the *entire* timeline eagerly in `Read()`; this
code doesn't).

But there's a catch: this kitchen has exactly **one slow-cooker**, and the dish is a
stew that gets richer each day by simmering on top of yesterday's pot — a **stateful
simulation** (a feedback loop, like "move a point up by adding to where it was last
frame"). You can't just teleport the pot to "day 5's stew" — it has to actually simmer
through days 1, 2, 3, 4 first. If someone orders day 5, and the pot is currently at day
3, the cook just keeps simmering forward. But if the pot has moved on to day 8 and
someone orders day 5 (which was never actually made in *this* pot, or the pot's gone
stale), the cook can't rewind — they dump it, grab a **fresh pot**, and simmer all over
again from day 1. Even though they already know exactly what days 1–4 tasted like
(it's written on a card — cached), they still have to put those ingredients in the new
pot and let it simmer (the pot's hidden internal state has to be rebuilt); they just
don't need to stop, plate it, and photograph it for the menu again each time (skip the
expensive conversion step) since that record already exists.

## The pieces, and how they fit together

```
 USD composition                                    USD runtime queries
 ┌──────────────────────-┐                          ┌────────────────────────-─--┐
 │ /Root (prim)          │                          │ UsdAttribute::Get(t)       │
 │  bifrostCompound:name │                          │ GetTimeSamples()           │
 │  bifrostSettings:{}   │                          │ Flatten() / TransferContent│
 │  payload = @...dff@   │                          └────────────-┬───────────--─┘
 └──────────┬────────────┘                                        │
            │ PCP asks: "what args for this payload?"             │
            ▼                                                     │
 ┌───────────────────────────────-─────┐                          │
 │ BifrostDynamicFileFormat            │                          │
 │  ::ComposeFieldsForFileFormat       │                          │
 │    Arguments()                      │                          │
 │  (decodes compound/settings/inputs/ │                          │
 │   outputs into a FileFormatArguments│                          │
 │   map — see dffArguments.*)         │                          │
 └──────────────┬──────────────────────┘                          │
                │ FileFormatArguments                             │
                ▼                                                 │
 ┌───────────────────────--─────────────┐                         │
 │  ::InitData(args)                    │                         │
 │   -> BifrostDffDataParams::FromArgs  │                         │
 │   -> BifrostDffData::New(params)     │                         │
 └──────────────┬───────────────────────┘                         │
                │ creates                                         │
                ▼                                                 │
 ┌───────────────────────────────────-─┐   Has() / Get() /        │
 │  BifrostDffData  (SdfAbstractData)  │◀── QueryTimeSample()  ───┘
 │   thin public wrapper, forwards     │
 │   everything to _impl               │
 └──────────────┬──────────────────────┘
                │
                ▼
 ┌───────────────-─────────────────────┐
 │  BifrostDffDataImpl                 │   <- the actual kitchen
 │  (mutex-guarded; all the logic)     │
 └──────────────┬──────────────────────┘
                │ lazily, on demand
                ▼
 ┌──────────────────-──────────────────┐
 │  dffExecuteGraphFrameToLayer.*      │   <- the slow-cooker's interface
 │   executeGraphFrameToLayer()  (expensive: run + extract + convert to USD)
 │   advanceGraphFrame()         (cheap: just run, throw away the output)
 └──────────────┬──────────────────────┘
                ▼
        Bifrost GraphExecutor (runs the compound; this IS the pot —
        it carries the hidden simulation state between frames)
```

### Walkthrough of what actually happens

1. A prim payloads onto this file format. USD asks `ComposeFieldsForFileFormatArguments`
   to decode all the compound/settings/globals/inputs/outputs fields and attribute
   overrides into a flat `FileFormatArguments` map (this is `dffArguments.*`'s job).
2. USD calls `InitData(args)`, which decodes those args into a `BifrostDffDataParams`
   struct (compound name, output port, timeline start/end/fps, verbosity, etc.) and
   wraps it in a `BifrostDffData` — this *is* the layer's content from USD's point of
   view, but nothing has been computed yet.
3. The first time anything actually asks this data a question (`HasSpec`, `Get`,
   `QueryTimeSample`...), `BifrostDffDataImpl::ensureTopology_l()` fires exactly once:
   it builds a fresh `GraphExecutor`, runs the compound at the **start frame**, and
   keeps that single result as `m_topologyLayer` — this tells USD what
   prims/properties/types exist, and serves as the default (untimed) value. If the
   payload is animated, it also seeds an anonymous `m_accumLayer` with that same
   structure and stamps the start frame's per-frame values into it as time sample 1.
4. From then on, every `QueryTimeSample(path, time)` for an animated attribute goes
   through `ensureFrameComputed_l(frame)`.

## The stepping cursor and cache, in one picture

```
 Animated frames declared:      1    2    3    4    5
                                 │    │    │    │    │
 m_computedFrames (the cache):  ●────●────●              <- 1,2,3 already
                                 ▲              ▲            simmered & cached
                          start frame      m_currentFrame
                        (topologyFrame)    = 3  (the cursor:
                                                  how far THIS pot
                                                  has actually simmered)
```

`m_computedFrames` is the recipe cards — what's already been cooked and written down
in `m_accumLayer`. The **cursor** (`m_currentFrame` / `m_hasCurrent`) is separate: it's
how far *this specific pot* has actually simmered, which is what determines whether
you can keep going or have to start a new pot.

```
                    ensureFrameComputed_l(frame)
                              │
        frame already cached? ───yes──▶ return true   (free — just read the card)
                              │no
                              ▼
        frame behind the cursor? ───yes──▶ resetExecutor_l()
                              │no              (new pot, cursor reset to "none")
                              ▼                        │
        nextFrame = cursor+1 (or start frame) ◀────────┘
                              │
        while nextFrame <= frame:
            already cached?  yes ──▶ advanceGraphFrame()          (cheap: simmer
                                                                    through it, but
                                                                    don't re-plate it)
                             no  ──▶ executeGraphFrameToLayer()    (expensive: simmer
                                       + extract outputs           + extract + convert
                                       + stamp into m_accumLayer   + cache the result)
            cursor = nextFrame; nextFrame += 1
```

The "reset and replay from the start" branch is why the pot analogy matters: a
*fresh* `GraphExecutor` has zero memory. To get it to frame 5's state, you must run it
through 1, 2, 3, 4 first — there's no way to skip ahead, even for frames whose *output*
you already know. That's exactly why `advanceGraphFrame` exists: replaying an
already-cached frame during a reset still calls it (simmer it, keep the pot's state
moving forward) but skips the expensive `executeGraphFrameToLayer` extraction (no need
to re-plate a dish you've already photographed).

## The gotcha

The natural assumption is "if I already have frame 2's value cached, querying it
should never touch the graph executor at all." That's true for a **forward** revisit —
but it's *not* true the moment you land on an uncached frame that's *behind* the
cursor. In that case, even perfectly-cached frames along the replay path (like the
start frame) get **genuinely re-executed** — you'll see `"Set frame 1"` fire again in
the diagnostics — even though nothing about frame 1's value changed. The only thing
spared is the expensive USD-conversion step. This is easy to misread as a caching bug
("why is it re-running a frame I already have?"), but it's required: the cache
remembers *outputs*, not the executor's own hidden internal state, and only the
executor's forward-only replay can rebuild that state correctly.
