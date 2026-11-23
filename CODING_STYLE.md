# Coding Style

These conventions apply to first-party C++ code in `src/` and `tests/`. Leave third-party, generated, and vendored code unchanged.

## Braces

Use Allman style for namespaces, types, functions, and control-flow blocks that need braces. For `if`, `else`, and loop bodies containing exactly one statement, prefer omitting braces. Put the statement on the next line, one indent level in, and apply this to nested statements when removing braces preserves the same control-flow association. Keep braces for multi-statement bodies, `switch` bodies, and branches where removing braces could change which `if` an `else` binds to. Put opening braces on their own lines where braces are used, and put `else` and similar continuations on their own lines after the preceding closing brace.

```cpp
for (Racer* Runner : GetEntrants())
	if (Runner != nullptr && !HasResult(Runner))
		ActiveRacers.push_back(Runner);

if (Ready)
{
	Start();
	LogStart();
}
else
	Stop();
```

## Namespaces

Do not use anonymous namespaces in first-party C++ code. Put implementation details in a descriptive, source-specific named namespace (for example, `CareerWorldSimulationDetail`) so helper names remain distinct across files. A source file may import its own detail namespace locally after the namespace block; never add such a directive to a header. Do not change third-party, generated, or vendored code.

## Header guards

Use filename-based `#ifndef` include guards in first-party headers, followed by a matching `#define` and a closing `#endif`. Derive the uppercase guard name from the header filename, not its directory or a generic project name (for example, `Race.h` uses `RACE_H`).

## Error handling

Do not use C++ exceptions. Do not `throw`, `try`, or `catch`. Report failures through explicit return/status values or error objects, and make callers handle them visibly. Do not replace errors with silent defaults or success-shaped results.

## Numeric values

Do not use magic numbers. Give domain-specific thresholds, timings, rates, bounds, dimensions, and other meaningful numeric values descriptive named constants, enums, or configuration fields. Keep only self-evident values such as zero/one checks and simple loop indices inline; if a literal needs explanation, name it.

## Control flow

Avoid deeply nested `if` statements; do not nest more than two levels. Prefer guard clauses with early returns or continues, and extract a focused helper when it keeps the main path clear.

## Existing conventions

For indentation, naming, and formatting not specified here, follow the surrounding first-party code and preserve each file's existing encoding and line endings.