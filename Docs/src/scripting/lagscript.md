# LAGScript

LAGScript is a Python/GDScript-inspired embedded scripting language. It runs
on its own lexer, parser, AST, and tree-walking interpreter — no external deps.

## Syntax at a glance

```
extends Node3D

signal died(cause: String)

var health: int = 100
var speed: float = 5.0

func _ready():
    print("Player ready")

func _process(delta: float):
    if health <= 0:
        emit died("health_zero")

func take_damage(amount: int):
    health -= amount
    if health < 0:
        health = 0
```

## Features

- Indent-based blocks (Python-style)
- Variables (`var`) and constants (`const`) with optional type annotations
- Functions (`func`), including methods on classes
- Classes with single inheritance (`extends`)
- Signals with `emit` and `connect`
- Control flow: `if` / `elif` / `else`, `for x in iter`, `while`, `break`, `continue`
- First-class arrays `[1, 2, 3]` and dictionaries `{"key": value}`
- Built-ins: `print`, `len`, `range`, `str`, `int`, `float`, `abs`, `sin`, `cos`, `sqrt`, `random`, `append`

## Attaching to entities

```cpp
auto inst = LAGScriptEngine::LoadScript("Content/Scripts/player.lag");
inst->CallMethod("_ready", {});
inst->CallMethod("take_damage", { Value::Int(10) });
```

Or via `LAGScriptComponent` on a scene entity — the component forwards
`_ready`, `_process(delta)`, `_physics_process(delta)`, and `_input(event)`
lifecycle methods automatically.

## Registering native functions

```cpp
Interpreter& interp = inst->GetInterpreter();
interp.RegisterNative("get_entity", [](Interpreter&, const ValueList& args) {
    uint64_t id = (uint64_t)args[0].AsNumber();
    // ... look up in scene ...
    return Value::Int(id);
});
```
