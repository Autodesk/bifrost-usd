# The persistent `GraphExecutor`'s lifetime

This document zooms in on one piece of [ARCHITECTURE.md](ARCHITECTURE.md): the
lifetime of `BifrostDffDataImpl::m_executor` — when it is created, when it actually
executes the Bifrost graph, and when `executeGraphFrameToLayer` is used instead of
`advanceGraphFrame` (and vice versa).

## The pot's lifetime, concretely

Sticking with the slow-cooker analogy from `ARCHITECTURE.md`: `m_executor` is a
`std::unique_ptr<GraphExecutor>` — a single pot. Each `resetExecutor_l()` call throws
out whatever pot exists (the `unique_ptr` assignment destroys it) and hands you a
brand-new one. Between resets, that *same* pot object gets executed on repeatedly, its
hidden state accumulating frame after frame.

```
 One pot's life:

   resetExecutor_l()                     resetExecutor_l()
   creates Pot #1                        destroys Pot #1, creates Pot #2
        │                                       │
        ▼                                       ▼
   ┌─────────┐  execute(f1)  execute(f2) ...   ┌─────────┐  execute(f1) ...
   │  Pot #1 │──────────────────────────────▶  │  Pot #2 │───────────────▶
   └─────────┘                                 └─────────┘
   m_hasCurrent: false→true                     m_hasCurrent: false→true
   m_currentFrame:  f1 → f2 → ...                m_currentFrame: f1 → ...
```

### When is it *created*? — `resetExecutor_l()`

Exactly two call sites, both inside `BifrostDffDataImpl`:

```
ensureTopology_l()                       ensureFrameComputed_l(frame)
──────────────────                       ────────────────────────────
Always, unconditionally, the             Only when:
FIRST time any query touches               !m_executor          (doesn't exist yet)
this BifrostDffDataImpl. There's        || !m_hasCurrent         (was just reset, hasn't
no pot yet, so one has to be                                      run since)
made before anything can happen.        || frame <= m_currentFrame (target is at-or-behind
                                              the cursor)
```

The third condition is the "backward scrub" case — the cursor can only move forward,
so landing on a frame at or behind it forces a fresh pot. Note it's `<=`, not `<`: if
`frame == m_currentFrame`, the very first check at the top of `ensureFrameComputed_l`
(`m_computedFrames.find(frame)`) should already have returned `true` before we even
get here — under the invariant that the cursor always points at a cached frame, that
case can't actually reach the `<=`. Using `<=` anyway is a deliberate defensive
margin: it doesn't rely on that invariant holding forever, and costs nothing extra
when it does hold (the "redundant" replay just re-runs the cursor frame once via the
cheap path below).

### When is it *executing*?

Every single call to `executor->execute(...)` — regardless of which wrapper invokes
it — happens in exactly one of these two places:

```
ensureTopology_l()                                ensureFrameComputed_l(frame)'s loop
───────────────────                                ────────────────────────────────────
Exactly once, at the start frame,                  Once per nextFrame, for every
via executeGraphFrameToLayer().                    nextFrame from (cursor+1) up
This is the ONLY execution whose                   through the target frame.
purpose is discovering STRUCTURE
(what prims/properties exist).
```

So "executing" is never a background/async thing — it's synchronous, one frame at a
time, always inside a call that's holding `m_mutex`.

### `executeGraphFrameToLayer` vs `advanceGraphFrame`

This is decided fresh on **every iteration** of the stepping loop:

```
                    for nextFrame in [cursor+1 .. target frame]:
                              │
              nextFrame ∈ m_computedFrames ?
                    │
        ┌───────────┴────────────-┐
       yes                       no
        │                         │
        ▼                         ▼
  advanceGraphFrame()      executeGraphFrameToLayer()
  ─────────────────        ──────────────────────────
  setGraphInputs +         setGraphInputs + execute()
  execute() only.          + extractTerminalOutput/
  Output is thrown          extractOutputClosure
  away — we already        + objects_to_stage / build
  have this frame's         a UsdStage
  cached value.            + flatten to an SdfLayer
                           + stampFrameSamples_l(...)
                             into m_accumLayer
                           + m_computedFrames.insert(...)
```

The **only** reason `advanceGraphFrame` ever runs is that a reset just happened and
the replay is passing back through frames it's already computed before (most
commonly the start frame, since that's always cached the moment topology succeeds).
`executeGraphFrameToLayer` is the one that actually produces new, uncached data —
including the very first call ever, at the start frame in `ensureTopology_l`, since at
that point `m_computedFrames` is still empty and there's no cache to fall back on.

## The gotcha

It's tempting to think "resetting the executor means starting over from scratch" —
but `resetExecutor_l`'s own doc comment is explicit about what it does *not* touch:
*"The computed-frame cache (`m_computedFrames`) and the accumulating layer are
intentionally preserved; only the executor and the stepping cursor are reset."* The
cache **outlives** the pot. So after a reset-and-replay, if you then query a frame
that was cached *before* the reset (say frame 2, cached back when the cursor first
passed through it), the top-level check in `ensureFrameComputed_l` finds it in
`m_computedFrames` and returns immediately — **without touching the new pot at all**,
even though that pot has never personally been run through frame 2 in its own short
life. The cache doesn't care which pot produced a value, only that the value exists.
