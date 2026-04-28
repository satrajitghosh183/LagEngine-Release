# Control Nodes

LAG's UI system is organized as a tree of `UIWidget` nodes — analogous to
Godot's Control nodes or Unity's RectTransform hierarchy.

## Widget tree

```cpp
auto canvas = CreateRef<UICanvas>();

auto panel = CreateRef<UIPanel>();
panel->setSize({ 400.0f, 300.0f });
canvas->addWidget(panel);

auto btn = CreateRef<UIButton>("Click me");
btn->setPosition({ 16.0f, 16.0f });
btn->OnClick = [] { GE_CORE_INFO("clicked"); };
panel->addChild(btn);

UISystem::SetCanvas(canvas);
```

## Anchors

Every widget has an `Anchor` controlling how it positions itself relative
to its parent:

```cpp
btn->setAnchor(Anchor::BottomRight); // stays in the bottom-right corner
```

Presets: `TopLeft`, `TopCenter`, `TopRight`, `MiddleLeft`, `Center`,
`MiddleRight`, `BottomLeft`, `BottomCenter`, `BottomRight`,
`StretchHorizontal`, `StretchVertical`, `StretchAll`.

## Built-in widgets

- `UILabel` — static text
- `UIButton` — clickable button with OnClick
- `UIImage` — textured quad
- `UISlider` — draggable value slider
- `UIProgressBar` — non-interactive progress bar
- `UIPanel` — container with background + border
- `UICheckBox` — toggle
- `UITextEdit` — single-line text input
- `UIDropdown` — select from a list
- `UITabContainer` — tabbed content
- `UITreeView` — hierarchical list

## Layout containers

- `UIHBox` — horizontal box
- `UIVBox` — vertical box
- `UIGridContainer` — N-column grid
- `UIScrollContainer` — clipped + scrollable
- `UIDockRoot` / `UIDockPanel` — editor-style docking
