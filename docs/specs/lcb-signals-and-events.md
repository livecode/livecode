# Signals and Events for LiveCode Builder Widgets
Copyright © LiveCode Ltd.

## Introduction

The types of external occurrences that a LiveCode Builder (LCB) widget can respond to can be broadly divided into two categories:

1. **Signals**: these are emitted from a specific LCB widget, and are handled only by those widgets that are already expecting to receive them.

   For example, the _Segmented_ widget emits a "segmentSelected" message when a segment is selected.  If the _Segmented_ widget was a child of a composed widget, then _only_ the composed widget receives the message, and only if the widget code contains a public "OnSegmentSelected" handler (i.e. it is expecting to receive the message).

2. **Events**: these originate from outside the program, and represent some sort of interaction with the outside world (for example, a mouse movement or keyboard event).  They are targeted at a specific widget (based on diverse criteria, such as position, visibility, or various types of focus), and propagate based on widget composition.

This document describes a new, more rigorous scheme for ensuring that these behave in a unambiguous and consistent way, and that there are good facilities for LCB programmers to make use of them.

## Existing problems

There are several issues with the way signals and events currently work for controls implemented in LCB.  The problems affect both the way that widgets can be used to construct other widgets (i.e. composed widgets), and the easy of handling widgets alongside legacy controls from a LiveCode Script (LCS) point of view.

1. Signal emission and handling is not very well checked.  At a language level, there is no way for widgets to describe what type of signals they can emit.  Similarly, there is no syntax for distinguishing between widget handlers and "normal" handlers, meaning that (in principal) a handler that is not intended to be a signal handler could end up getting called in a signal context if a child widget happens to start emitting a signal with a matching name.  Finally, there is no facility for checking that signal prototypes match signal handler prototypes.

2. Similar issues to 1. above apply for event handling.

3. In LCS, event handling propagates upward: if a button doesn't have an `MouseUp` handler, then the event gets passed on to a parent card, and so on.  Furthermore, there is the idea of explicit propagation: it is possible to do something in an event handler and then, for example, `pass MouseUp` to invoke the normal event propagation logic.

   In LCB, event propagation is *not* currently automatic.  If a widget does not explicitly handle and re-`post` each possible event, then its script (or parent widget) never sees the event.

4. Sometimes, it's important to be able to be able to prevent child widgets from seeing an event, or performing some pre-processing / post-processing around the normal event handling process.  The current LCB event handling scheme does not permit this.

## Assumptions

For the purposes of this document we assume that:

* All connection of signal and event handlers is implicit, rather than explicit.

* An instance of a widget as an engine control can be considered as a composed widget, comprising a script and the "real" widget instance (this makes sense when you consider the fact that the widget control exists, has a script, and can be manipulated even if the "real" widget was unable to be loaded for some reason).  This will be referred to as a "host widget" in the rest of this document.

## Signals

### Signal emission

A widget's LCB source code must explicitly name, and provide prototypes for, the signals that it can emit [1]:

    public signal <name:id>(<params:formals>)

Signal declarations must be placed in module scope.  The signal **name** must be unique in the module.

Signals are explicitly emitted by a widget using an `emit` statement:

    emit <name:id>(<params:expressions>)

It is a run-time error to emit a signal with the wrong number or types of emission parameters [2].  The compiler may statically verify that signals are emitted with compatible parameter lists.

The `emit` statement may return before all the signal handlers for the signal have finished running [3].

The signal handlers for a signal get run exactly once for each emission.  All of the handlers for an emission must complete before any of the handlers for a subsequent emission begin [4].

The `emit` statement does not return a value [5].

**Notes**:

1. These declarations provide a convenient anchor point for documentation comments about the purpose and usage of a signal.

2. In principle a widget could set properties on itself before emitting a signal.  However, when adding features to LCS previously it's been found that users prefer relevant information to be passed as parameters to messages rather than out-of-band.  Passing parameters to signal emission also helps to ensure that all of the correct setup has been performed for signal handling - for example, if a parameter is added or removed from the signal, it's easy to verify that all relevant code has been updated.

3. Although, initially, signal emission _could_ be implemented by iterating over registered signal handlers and calling them within the execution of the `emit` statement, this could easily lead to arbitrarily-deep signal emission chains going backwards and forwards between LCS and LCB contexts.  By allowing signal emission to be delayed, this is avoided (the current stack can be allowed to unwind before signal dispatch is performed).  Delaying signal handling also helps to reason about the safety of deleting a widget in response to receiving a signal from it.

4. Emitting a signal twice from the same handler - even with the same parameters - will cause all of the handlers for that signal to be run twice.

5. This is a consequence of asynchronous signal handling.

### Signal handling

Signal handlers are explicitly declared in an LCB program using a `signal handler` declaration [1]:

    signal handler for <name:id>(<params:formals>) is <handler:constexpr>

At most one signal handler for each distinct signal prototype may be declared.  The **handler** must conform to the signal prototype.

Signal handlers have no return value.

Within the body of a signal handler, `the signal source` evaluates to the widget that emitted the signal that is being handled.

It is an error to evaluate `the signal source` outside a signal handling context.

**Notes:**

1. This allows a single LCB handler to be used to handle multiple signals.  It allows multiple signals with different prototypes to be handled by a single widget's script without overloading.

### Signal connection

When a child widget is added to a composed widget, each declared signal of the child widget is examined.  If the parent widget declares a signal handler with a matching prototype, then it is connected to the signal [1].

When a child widget is removed from a composed widget, all signal handlers in the parent widget that are connected to a signal of the child widget are disconnected.

**Notes:**

1. When these rules are applied to "host widgets", each public script `on` handler in the host widget's script is automatically considered to have been declared as a polymorphic signal handler.

## Events

### Event processing

Events are processed in three phases [1]:

1. **Enter**: before handling an event, perform event-handling setup tasks and determine at point in the event hierarchy the event should be targeted

2. **Dispatch**: do work in response to the event

3. **Leave**: perform any tasks that should be carried out after event handling (for example, tearing down any state set up during event enter)

Before beginning to process an event, the event manager first determines which control should be the **target** of the event (taking into account visibilities, and bounding rectangles) [2].

A list of "interested controls" is built.  This recursively contains every parent of the target.  This is used to determine which objects should have their event handlers run.

**Notes:**

1. The enter phase and leave phase facilitate the implementation of behaviour that wraps normal event dispatch.  For example, consider a composed widget with a border that highlights when the mouse is inside it.  To ensure that highlight changes sequence correctly with with its child widgets, the highlight must be enabled during the `MouseEnter` event's enter phase, and hidden during the `MouseLeave` event's leave phase.

2. How the target of an event is determined may vary a lot on a per-event-type basis (for example, the target for a mouse click will depend on the mouse position, but the target for a keyboard event may depend on the keyboard focus).  For the purposes of this document, assume that it is an arbitrary process.

### Event data

When an event is submitted to the event manager, it has a **event name** and a set of **event data**.  The event data is an array containing all of the captured data relating to the event itself [1].

In an event handler, `the event` evaluates to the event data array [2].

Evaluating `the event` outwith an event handler is an error.

`the event` is immutable.

**Notes:**

1. For example, when creating a mouse event, it is important to capture the position of the mouse at the time the event began to be dispatched into the event data.  If computationally-intensive event handlers are run, the user could move the mouse between event creation and event handling.

2. This permits simple implementation of syntactic sugar for frequently-needed event data.  For example:

       the mouse position of the event

   can easily be implemented as:

       the event["mouse position"]

### Event handlers

All event handlers (for enter, dispatch and leave phases) must conform to:

    public type EventHandler() returns any

The return value of all event handlers is discarded.

### Enter phase

During the enter phase, **pre-event** handlers are run.  They are run from "outside-in"; a parent widget's pre-event handler runs before the pre-event handler of its child widgets.

A pre-event handler is declared using a `pre event handler` declaration:

    pre event handler for <name:id> is <handler:constexpr>

By default, all pre-event handlers declared by the list of interested controls are run.  There is no need to "pass" the event onwards [1].

A control can use the pre-event handler to **capture** an event:

    capture event

The `capture event` statement makes the capturing control becomes the "target" for the event; all its children are removed from the list of interested controls [2].

**Notes:**

1. As per the `mouseEnter`/`mouseLeave`, implicit passing reflects the expectation that pre- and post-event handlers are usually be used to implement behaviour that complements rather than conflicts with that of other interested controls.

2. This allows composed widgets to "hide" events from their children.

### Dispatch phase

During the dispatch phase, **on-event** handlers are run.  They are run from "inside-out", starting from the target control.  A parent widget's on-event handler runs after its child widgets' on-event handlers.

An on-event handler is declared using an `event handler` declaration [1]:

    [on] event handler for <name:id> is <handler:constexpr>

By default, if an on-event handler is found for an event, then it **blocks** further on-event handler execution for the event [2].

An on-event handler can use the `pass event` statement to prevent the default-blocking behaviour.

The `pass event` statement does not return.

**Notes:**

1. Since most widgets will only need to handle events during the dispatch phase, on-event handlers may be declared without a qualifier.

2. This is for conceptual compatibility with LCS.

### Leave phase

During the leave phase, **post-event** handlers are run.  Like on-event handlers, they are run from "inside-out", starting from the target control.

All interested controls have the opportunity to run a post-event handler, even if an on-event handler blocked the event during the dispatch phase [1].

A post-event handler is declared using a `post event handler` declaration:

    post event handler for <name:id> is <handler:constexpr>

It is not possible to capture or block post event handler execution.

**Notes:**

1. All controls that run a pre-event handler are guaranteed to get their post-event handler run.

## Event handler connection

The handling of LCS events is unchanged.
