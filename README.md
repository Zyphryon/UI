# Zyphryon UI

A retained game interface for Zyphryon. Artists and modders author it as data, the game feeds it values, and the
engine's own graphics draw it.

- **Themes and layouts in JSON**: palettes, typefaces, styles per state, transitions and keyframed animations.
- **Data binding**: widgets show values of a model the game keeps, and write the user's edits back.
- **A full widget library**: buttons through grids, tree views, splitters and docking panels.
- **Drag and drop, tooltips, context menus, menu bars and modal dialogs**.
- **Keyboard, gamepad and pointer navigation**, shortcuts, scrolling and text editing.
- **Resolution independence and localisation**: layouts in units that follow the display's DPI, and string tables per
  language.

## Getting started

Add the repository to your CMake project and link `ZyUI`:

```cmake
FetchContent_Declare(ZyUI GIT_REPOSITORY https://github.com/Zyphryon/UI GIT_TAG main)
FetchContent_MakeAvailable(ZyUI)

TARGET_LINK_LIBRARIES(MyGame PRIVATE ZyUI)
```

`ZyUI` gathers five libraries, which can also be linked on their own:

| Library | Gives | Leave out when |
|---------|-------|----------------|
| `ZyUI_Core` | The interface, styles, layout, drawing and input. | — |
| `ZyUI_Data` | Models, bindings and languages. | binding nothing, in one language |
| `ZyUI_Markup` | Themes and layouts read from files. | building the interface in code |
| `ZyUI_Gesture` | Drag and drop, tooltips and context menus. | needing none of them |
| `ZyUI_Control` | The widget library and its default theme. | bringing widgets of your own |

A frame reads a layout, spawns a window and drives the interface:

```cpp
Markup Reader(AddressOf(Content));
ControlModule::Declare(Reader);                                 // every widget, and the states a theme names
Reader.Dress(Theme, ThemeText, Error);                          // the default theme, with the game's over it
Reader.Read(LayoutText, Error);

Interface.Attach(Input, AddressOf(Platform.GetWindow()));       // input, key repeats, the clipboard and the scale
Interface.Bind("ShowNames", Handler::Create<&OnShowNames>());
const Element Window = Reader.Spawn(Interface, "Settings");

// Every frame:
Interface.Update(TargetSize, Delta);
Interface.Draw(Canvas);
```

`ZY_UI_EXAMPLE` builds an example showing every feature, with its resources under `Example/Resources`.

## Widgets and themes

A widget is behaviour plus a template made of other widgets: a `Button` is a panel holding a `Label`, and a `Combobox`
is a button-like header with a popup holding a `Listbox`. Nothing paints itself. Every part wears a named style
(`Button`, `Button.Label`, `Checkbox.Mark`), and the theme decides how that style looks. A style can start from a
`Base`, and gives variants worn in states (`Hover`, `Press`, `Focus`, `Checked`...). Parts take their control's states,
so a theme can light a button's label while the button is hovered.

The library ships `Button`, `Checkbox`, `Combobox`, `Dialog`, `Dock`, `Dropdown`, `Expander`, `Grid`, `Image`, `Label`,
`Listbox`, `Menu`, `Menubar`, `Outline`, `Panel`, `Progress`, `Radio`, `Repeater`, `Scalar`, `Scrollbar`, `Scroller`,
`Slider`, `Spinner`, `Splitter`, `Swatch`, `Tabs`, `Textbox` and `Window`. Each has `Create` to make it in code and is
named by its `Widget` in a layout.

Events bubble from the target through every control above it. Each control runs its behaviour, then the game's
observers, then the action it names. Most actions only care that their control was clicked or changed: `OnClick` and
`OnChange` hear just that, from the control naming the action, and let the event bubble on so a menu or dialog holding
the control still closes. `Bind` hears every event raw, and can stop one.

```cpp
Interface.OnClick("Shop.Buy", [this](Element Control) { Buy(Repeater::GetKey(mStock, Control)); });
Interface.OnChange("Settings.Volume", [](Element Control) { SetVolume(Slider::GetValue(Control)); });
```

## Themes

A `.theme` holds a palette, typefaces, images, styles and animations. `Dress(Theme, Error)` uses the default theme
alone; `Dress(Theme, Source, Error)` reads a game's theme over it, so the game's gives only what it changes:

- Its colours, typefaces and images win, and reach the built-in styles too.
- Its animations replace those of the same name.
- A style it names merges over the built-in one field by field, and a `When` variant over the one of the same states.
- A typeface given as `null` is a slot, filled by `Markup::SetFont` or a later theme.

A file with a mistake in it is reported, and whatever was in use is kept.

A colour is `#RRGGBB`, `#RRGGBBAA`, a palette name, `[r, g, b]` or `[r, g, b, a]` from 0 to 1, or a hex or name with an
alpha of its own, as `["Gold", 0.18]`, which keeps following the palette when a theme changes it.

```json
{
    "Fonts":  { "Body": "Resources://Font/MedievalSharp-Regular.fnt" },
    "Colors": { "Gold": "#F0D98F" },
    "Styles": {
        "Label":  { "Font": "Body", "Size": 16 },
        "Button": { "Background": "#2E3645", "Radius": 6, "Padding": [12, 6],
                    "When": { "Hover": { "Glow": { "Color": ["Gold", 0.18], "Blur": 8 } },
                              "Focus": { "Border": "Gold", "BorderWidth": 1 } } }
    }
}
```

A game's theme names only the parts it restyles; the rest keep the built-in look.

### Effects

Any style can give CSS-like effects, and each can be set per state:

| Field | Value | Meaning |
|-------|-------|---------|
| `Gradient` | `{ "From": c, "To": c, "Angle": deg }` or `["#a", "#b"]` | A fill over `Background`. The angle reads as in CSS: `0` runs upward, `90` to the right, `180` (the default) downward. |
| `Shadow` | `{ "Color": c, "Offset": [x, y], "Blur": n, "Spread": n }` | A soft box cast behind the element. |
| `Glow` | the same | A shadow that is never moved, so it rings the element. |
| `InnerGlow` | `{ "Color": c, "Blur": n }` | A glow just inside the element's edge. |
| `Scale` | `n` or `[x, y]` | Drawn larger or smaller about its middle. |
| `Rotate` | degrees | Drawn turned clockwise about its middle. |
| `Translate` | `[x, y]` | Drawn moved, in pixels. |
| `Transition` | seconds, or `{ "Duration": s, "Delay": s, "Easing": name }` | How the element moves to the values of a new state. |
| `Animation` | name | A keyframed animation played while the style is worn. |
| `Enter` / `Exit` | name | Played once when the element appears / is hidden. |

`"Shadow": { }` removes an inherited shadow. Easings are named as in `ZyMath::Easing` (`Linear`, `OutQuad`,
`InOutSine`, `OutBack`...), and a bare number of seconds eases with `OutQuad`.

**Transitions.** When an element changes state, style or theme and its new style gives a `Transition`, colours,
opacities, radii, borders, gradients, halos and transforms move smoothly to the new values; sizes, padding, typefaces
and alignment change at once. As in CSS, the transition of the style being entered is used, so one given in the normal
style applies into and out of every state.

**Animations.** A theme's `Animations` are CSS's `@keyframes`. Each key stands at a share of the run (`0` to `1`, or
`"50%"`) and gives any animatable field. `Repeat` counts runs (`0` plays forever) and `Alternate` plays every second run
backward. The default theme defines `FadeIn`, `FadeOut`, `Drop`, `Pop`, `Pulse` and `Spin`.

```json
"Animations": {
    "Pulse": { "Duration": 1.2, "Repeat": 0, "Easing": "InOutSine", "Keys": [
        { "At": 0, "Scale": 1 }, { "At": "50%", "Scale": 1.06 }, { "At": 1, "Scale": 1 } ] }
},
"Styles": {
    "Button": {
        "Gradient": { "From": "#3A4458", "To": "#262D3A" },
        "Shadow": { "Color": "#00000059", "Offset": [0, 2], "Blur": 6 },
        "Transition": 0.12,
        "When": { "Hover": { "Glow": { "Color": "#F0D98F2E", "Blur": 8 } }, "Press": { "Scale": 0.97 } }
    },
    "Window": { "Enter": "Pop", "Exit": "FadeOut" },
    "Badge":  { "Animation": "Pulse" }
}
```

While an `Exit` plays, the element is still drawn but can no longer be hit, focused or used. `Scale`, `Rotate` and
`Translate` change only how an element is drawn: layout and clicks still use its untransformed box.

### Images and sounds

**Images.** A style can lay an image over its box without changing the box's size. `"Image"` is an address, a name from
the theme's `"Images"` table, or an object of `Source`, `Region` (`[u0, v0, u1, v1]`), `Slice` (nine-slice borders in
pixels of the image) and `Tint`. A variant can change one member and keep the rest, and `"Image": ""` removes it.

```json
"Images": { "Door": "Resources://Image/Door.tex" },
"Styles": {
    "Framed": { "Base": "Button", "Image": { "Source": "Door", "Region": [0, 0, 0.5, 1], "Slice": 18, "Tint": "#A89F8F" },
                "When": { "Hover": { "Image": { "Tint": "#FFE9A8" } }, "Press": { "Image": { "Region": [0.5, 0, 1, 1] } } } }
}
```

**Sounds.** A style names a sound per moment: `"Sounds": { "Hover": a, "Press": a, "Click": a, "Focus": a, "Change": a,
"Open": a, "Close": a }`. A style without one plays its base's, and an empty address silences it. The library plays
nothing itself: the game hands `Interface::SetSoundHook` a hook that hears the address, the moment and the element.

## Layouts

A `.ui` holds templates. Each node names its `Widget`: one of the library's, one the game registers
(`Markup::Register`), or another template, which the node can then override field by field.

```json
{
    "Templates": {
        "Heading":  { "Widget": "Label", "Style": "Title" },
        "Settings": { "Layer": "Window", "Style": "Card", "Width": 300, "Children": [
            { "Widget": "Heading",  "Text": "Settings" },
            { "Widget": "Checkbox", "Text": "Show names", "Checked": true, "Action": "ShowNames" },
            { "Widget": "Label",    "Text": "Volume", "Name": "Volume" },
            { "Widget": "Slider",   "Value": 0.6, "Action": "Volume" }
        ] }
    }
}
```

A template can declare `Parameters` with defaults, reaching only the fields that name them as `"$Name"`, so a reusable
component exposes exactly what may change:

```json
"Setting": { "Parameters": { "Caption": "Unnamed", "Start": 0.5 }, "Flow": "Row", "Gap": 8, "Children": [
    { "Widget": "Label",  "Text": "$Caption", "Grow": 1 },
    { "Widget": "Slider", "Value": "$Start", "Width": 160 }
] }

{ "Widget": "Setting", "Caption": "Music", "Start": 0.8, "Action": "Music" }
```

Every node may give `Name`, `Style`, `Hidden`, `Disabled`, `Action`, `Children`, the fields of
[drag and drop, tooltips and menus](#drag-and-drop-tooltips-menus-and-dialogs), and the layout fields: `Placement`,
`Flow`, `Wrap`, `Align`, `Justify`, `Width`, `Height`, `Basis`, `Left`, `Top`, `Right`, `Bottom`, `Margin`, `Padding`,
`Gap`, `Grow` and `Shrink`. A length is pixels, `"50%"` or `"Auto"`. Room on each side is one number,
`[across, down]` or `[left, top, right, bottom]`.

Each window, HUD layer, popup or tooltip stands in a band (`Layer`): `Window`, `Popup`, `Modal`, `Tooltip`, `Drag`, or
one the game declares, as `Hud`.

## Data binding

The game keeps what the interface shows in a `Model` and points the interface at it. A value is a flag, a number, a
text or a colour, named by a key such as `Player.Gold`. A value can also be a list of records, each a model of its own:
`Party[2].Name`.

```cpp
mModel.SetText("Player.Name", "Aeldur");
mModel.SetNumber("Player.Gold", 120.0);
mModel.Append("Party")->SetText("Name", "Brom");   // also Insert, Erase, Resize, GetCount, GetRecord
Binder::SetModel(mInterface, AddressOf(mModel));
```

`Mirror` keeps a list matched to the game's own collection, given a key per item and code filling a record from it.
A kept key keeps its record, with anything the user left in it (a branch open, a field half typed), and only changed
values are shown again. `Repeater::GetKey` gives the key of the row an element stands in.

```cpp
mModel.Mirror("Party", mHeroes,
    [](ConstRef<Hero> Item) { return Item.Id; },
    [](Ref<Model> Row, ConstRef<Hero> Item) { Row.SetText("Name", Item.Name); Row.SetNumber("Level", Item.Level); });
```

In a layout, a field takes values in braces. One value alone, as `"Checked": "{Settings.Loot}"`, binds the whole value;
other text is written out with each value in place, as `"Text": "Gold: {Player.Gold:0}"`, where the number after the
colon fixes the decimals. `{!Key}` reads the opposite flag, and `{{` and `}}` write a brace. A template parameter may
itself be a binding.

| Widget | Shown only | Both ways |
|--------|------------|-----------|
| every node | `Hidden`, `Disabled` | |
| `Label`, `Button`, `Window`, `Expander`, tab | `Text` / `Title` | |
| `Checkbox`, `Radio` | `Text` | `Checked` |
| `Expander` | | `Expanded` |
| `Slider`, `Spinner` | `Minimum`, `Maximum` | `Value` |
| `Scalar` | `Minimum`, `Maximum`, `Tag` | `Value` |
| `Swatch` | `Value` (a colour) | |
| `Progress` | `Value` | |
| `Image` | `Source`, `Layer`, `Tint`, `Region` (four numbers as text), `Opaque` | |
| `Textbox` | `Hint` | `Text` |
| `Listbox`, `Combobox` | `Items` | `Selected` (a position, `-1` for none) |
| `Tabs` | | `Selected` |
| menu item | `Text`, `Icon`, `Hidden`, `Disabled`, `Tooltip`, `Heading` | `Checked` |
| `Menubar` menu | `Text`, `Hidden`, `Disabled` | |
| `Dropdown` | `Text` | |
| `Repeater` | `Items` | `Selected` |

What the game changes shows at the next `Update`, and a frame where nothing changed costs next to nothing. A change the
user makes is written to the model before any observer runs. Game changes are shown silently: no observer, action or
sound hears them.

`Markup::PushScope("Party[2]")` and `PopScope()` make the bindings built between them read one record, as a row of a
list does. A path opening with `/` still reads from the root: `"{Name} has {/Player.Gold:0}"`.

In code, `Binder::Attach(Element, Source, Writer, Reader)` binds any element, with writers and readers such as
`Binder::Put<Real32, &Slider::SetValue>()`, `Binder::PutText<&Label::SetText>()`, `Binder::PutState<State::Hidden>()`
and `Binder::Take<Real32, &Slider::GetValue>()`. A game's own widget binds its fields with `Markup::Bind`.

## Item lists

A `Repeater` shows a list of the model, one row per record, each made from a template whose bindings read the row's
record (and the root through `/`). Only the rows in view are made, so a list of thousands costs what a screenful does.

```json
"InventoryRow": { "Flow": "Row", "Gap": 8, "Children": [
    { "Widget": "Label", "Text": "{Name}", "Grow": 1 },
    { "Widget": "Label", "Text": "x{Count:0} for {/Player.Name}" }
] },
"Bag": { "Widget": "Window", "Title": "Bag ({Inventory} items)", "Children": [
    { "Widget": "Repeater", "Height": 230, "Items": "{Inventory}", "ItemTemplate": "InventoryRow", "Selected": "{Ui.Picked}" },
    { "Widget": "Repeater", "Height": 156, "Items": "{Inventory}", "Columns": 5, "ItemHeight": 48,
      "ItemTemplate": { "Widget": "Label", "Text": "{Count:0}" } }
] }
```

| Field | Meaning |
|-------|---------|
| `Items` | One list in braces, read from the scope the list stands in. |
| `ItemTemplate` | A template's name, or a node written in place. |
| `ItemHeight` | The height of every row. Left out, the first row is measured. |
| `Columns` | Rows side by side, making the list a grid. |
| `ItemWidth` | The width of a cell. Without `Columns`, as many columns as fit. |
| `Selected` | The position picked, `-1` for none; binds both ways. |
| `Multiple` | `true` to let control and shift pick several records, and control and `A` all of them. |
| `RightPick` | `false` to leave a right press's pick alone; `true` unless given. |
| `RowStates` | States each row wears by a value of its record, as `{ "Marked": "Sown", "Disabled": "!Usable" }`. |

A click picks a row and reports `Change`; the arrows, page keys, home and end move the pick. Each row wears the list's
style followed by `.Item` and is `Selected` while picked. A double press or enter sends `Signal::kActivate`, and `F2`
sends `Repeater::kRename`, both naming the record's path in `Content` (`Inventory[3]`). A right press picks its row so
the context menu opens over the pick.

The list must be given a height (200 unless given), and everything a row shows must come from bindings, since rows are
reused for other records as the list scrolls. A list given a `Name` scrolls back where it was left when it is made
again. In code:

```cpp
const Element Grid = Repeater::Create(Card, "Slots");
Repeater::SetColumns(Grid, 5);
Repeater::SetItemHeight(Grid, 48.0f);
Repeater::SetTemplate(Grid, Maker::Create<&FillSlot>());   // void FillSlot(Element Row, Text Scope)
Repeater::SetItems(Grid, "Inventory");
```

**List boxes and combo boxes** take `Items` as a list of texts or as one list of the model in braces. Each record makes
one item: `ItemText` is what it shows (`Name` unless given), `ItemHidden` hides it and `ItemDisabled` disables it, each a
key or a text naming values. `Selected` counts hidden items, so a combo box over an enum keeps picking by index while it
filters some out. These make every item up front, so a list of thousands belongs in a `Repeater`.

```json
{ "Widget": "Combobox", "Items": "{Assets.Formats}", "ItemText": "{Label} ({Bits:0} bit)",
  "ItemHidden": "{!Supported}", "Selected": "{Baker.Format}" }
```

## Drag and drop, tooltips, menus and dialogs

Any node may give these fields:

| Field | Value | Meaning |
|-------|-------|---------|
| `Drag` | text, number, flag or a binding | The element can be picked up, carrying this value. |
| `DragKind` | text | The kind of thing it is, which targets accept it by. |
| `DragTemplate` | template name | What the ghost under the pointer is made of, rather than a copy of the element. |
| `Drop` | a kind, a list of kinds, or `true` | Drags of those kinds, or of any, can be dropped on it. |
| `Tooltip` | text (may bind) or a template name | Shown once the pointer rests on the element. |
| `ContextMenu` | a list of items, or a template whose root is a `Menu` | Opened at the pointer by a right press. |

**Drag and drop.** A press that moves four pixels lifts the source: it is `Dragging`, a ghost wearing `Ghost` follows the
pointer, and a target accepting the kind is `DropHover`. The source hears `Drag::kStart` and `kEnd`, a target `kEnter`,
`kLeave` and `kDrop`, with `Event::Other` naming the other side. Escape or a release over no target cancels. Rows of a
list can be sources and targets, their payload bound to their record.

```json
"InventoryRow": { "Style": "BagRow", "Drag": "{Name}", "DragKind": "Item", "Drop": "Item", "Tooltip": "{Name}, {Count:0} in the bag" }
```

**Tooltips** wear `Tooltip` and `Tooltip.Label`, show after `Tooltip::SetDelay` seconds (half a second by default), stay
on screen, and hide when the pointer leaves or presses.

**Context menus.** A `Menu` holds items with text, an optional shortcut, an action, separators (`"-"`), disabled items and
submenus. Picking an item runs its action and closes the menus. The arrows, enter and escape drive it from the keyboard,
and `Menu::GetSubject` tells an action what the menu was opened over.

```json
"ContextMenu": [ { "Text": "Cast", "Action": "CastSpell", "Shortcut": "Enter" },
                 { "Text": "Assign to bar", "Items": [ { "Text": "Slot 1", "Action": "Assign1" } ] },
                 "-", { "Text": "Forget", "Disabled": true } ]
```

An item may also give `Checked` (a check item, binding both ways, that keeps its menu open), `Hidden`, `Icon` (a glyph
before the text), `Tooltip`, and `Heading` (a heading over the items that follow). A menu written in a list row reads the
row's record, so its items can bind to it. Parts wear `Menu.Item`, `Menu.Item.Label`, `Menu.Shortcut`, `Menu.Arrow`,
`Menu.Separator`, `Menu.Check`, `Menu.Icon` and `Menu.Heading`.

**Menu bars.** A `Menubar` is a row of titles, each opening its menu under it, with the left and right keys moving
between them. `Children` given to the bar stand at its far end.

```json
{ "Widget": "Menubar", "Menus": [
    { "Text": "File", "Items": [ { "Text": "Save", "Shortcut": "Ctrl+S", "Action": "Save" }, "-", { "Text": "Exit", "Action": "Exit" } ] },
    { "Text": "View", "Name": "View", "Items": [ { "Text": "Hierarchy", "Checked": "{View.Hierarchy}", "Action": "View.Panel" } ] }
  ], "Children": [
    { "Widget": "Label", "Name": "Fps" },
    { "Widget": "Button", "Text": "{Render.Quality}", "Action": "Quality.Step", "Tooltip": "Draw quality; click to step" }
] }
```

**Dropdowns.** A `Dropdown` is a button with a fixed `Text` that opens the menu of its `Items` under it. Its own
`Action` hears the click, so the game can ready the items first.

```json
{ "Widget": "Dropdown", "Text": "+ Add Component", "Items": [
    { "Heading": "Physics" }, { "Text": "Body", "Action": "Add.Body", "Disabled": "{Actor.HasBody}" } ] }
```

**Modal dialogs.** A pane in the `Modal` band holds the pointer and keyboard while shown, over a backdrop wearing
`Modal.Backdrop`. A `Dialog` asks its `Message` through `Buttons`: enter clicks the default button, escape the cancel
button, and any button runs its action and closes the dialog unless the action stops the click.

```json
"QuitDialog": { "Widget": "Dialog", "Title": "Quit?", "Message": "Leave the realm, {Player.Name}?",
                "Buttons": [ { "Text": "Quit", "Action": "Quit", "Default": true }, { "Text": "Stay", "Action": "Stay", "Cancel": true } ] }
```

In code: `Dialog::Open(Tree, Title, Message)`, `AddButton`, `Show` and `Close`.

**Windows.** A `Window` is dragged by its title bar and kept on screen. With `"Resizable": true` its edges and corner grip
size it, never under its `Minimum` (`[160, 100]` unless given). Its close button reports `Window::kClose`; a handler that
stops it keeps the window open, as an editor asking "Discard changes?" does.

```json
"MaterialEditor": { "Widget": "Window", "Title": "Material", "Width": 520, "Height": 420, "Resizable": true,
                    "Minimum": [ 360, 240 ], "Action": "MaterialEditor" }
```

## Keyboard, gamepads and scrolling

**Navigation.** A key goes first to the control holding the keyboard. A key no control keeps moves the keyboard:

| Key | Does |
|-----|------|
| `Tab`, shift and `Tab` | The next or previous control, in layout order, wrapping around. |
| arrows | The nearest control that way on screen. |
| `Enter`, `Space` | Clicks the control holding the keyboard. |
| `Escape` | Closes the popup or modal pane in front. |
| `PageUp`, `PageDown`, `Home`, `End` | Scroll whatever scrolls around the control holding the keyboard. |

Navigation skips hidden and disabled controls, stays within a modal pane, and scrolls the control it reaches into view.
Widgets keep the keys they use: a list moves its pick with up and down, a slider its value with left and right, a text
field the editing keys. Every focusable default style shows a gold ring while it holds the keyboard.

**Pointer buttons.** The left button presses, drags, releases and clicks a control. A control with `"AnyButton": true`
hears the same from every button, named by `Event::Button`. `"Hover": true` makes a control hear `kEnter`, `kMove` and
`kLeave`. `Event::Count` counts presses in a row, and `Event::Modifiers` gives the keys held.

**Shortcuts.** A `Shortcut` is a key chord (`Shortcut("Ctrl+Shift+Z", "Edit.Redo")`) running an action or callback before
the control holding the keyboard hears the key. `Interface::AddShortcut(Entry)` adds one that runs anywhere;
`AddShortcut(Entry, Element)` one that runs while the pointer rests on the element (`Hover`) or it holds the keyboard
(`Focus`). Shortcuts are skipped while a text field is being typed in, unless they set `Typing`.

```json
{ "Name": "Preview", "Hover": true, "Shortcuts": { "R": "Preview.Reset",
  "Ctrl+Z": { "Action": "Edit.Undo", "Scope": "Anywhere", "Typing": true, "Blur": true } } }
```

**Gamepads.** Bind the pad's buttons to actions in the game's `ZyInput::Mapping` and call the interface from `OnAction`:

```cpp
mInterface.Navigate(Heading::Down);   // the d-pad, as the arrow keys
mInterface.Activate();                // confirm: enter
mInterface.Cancel();                  // back: escape
mInterface.NextFocus();               // the shoulders: tab and shift tab
mInterface.PreviousFocus();
```

`Navigate` with nothing holding the keyboard gives it to the first control, so a pad needs no pointer to start.

**Scrolling.** A `Scroller` is content with a bar down its side and along its bottom, each shown only while there is
something to scroll. `Horizontal` lets it scroll sideways, and `Follow` keeps it at its end, as a log. A `Scrollbar`
scrolls any element named by its `Target`. A viewer must be given a height (200 unless given).

```json
{ "Widget": "Scroller", "Height": 250, "Horizontal": true, "Follow": false, "Children": [ ... ] }
{ "Widget": "Scrollbar", "Orientation": "Horizontal", "Target": "Strip", "Automatic": true }
```

**Kept across rebuilds.** A panel spawned again keeps how its named parts were left: a `Scroller`, `Repeater`, `Grid` or
`Outline` given a `Name` scrolls back where it was, and an `Expander` given a `Name` opens as it was left.

```json
{ "Widget": "Scroller", "Name": "Inspector.Body", "Height": 400, "Children": [
    { "Widget": "Expander", "Name": "Transform", "Title": "Transform", "Children": [ ] },
    { "Widget": "Expander", "Title": "Sprite", "Expanded": "{Inspector.SpriteOpen}", "Children": [ ] }
] }
```

## Text

**Editing.** A `Textbox` edits as a desktop field does:

| Input | Does |
|-------|------|
| left, right | A character; with control, a word. Shift selects. |
| home, end | The start or end of the line; with control, of the whole text. |
| up, down, page keys | Move by lines in a field of several lines. |
| backspace, delete | The selection, the character, or with control the word. |
| control and A, C, X, V | Select all, copy, cut, paste. |
| a press, a drag | Places the caret or selects. Two presses select a word, three the whole text. |

`"Multiline": true` breaks lines on enter and wraps to the width; `"ReadOnly": true` keeps the text from changing;
`"Numeric": true` accepts only digits, signs and points; `Limit` caps its length; `Hint` shows while it is empty. The
selection wears `Textbox.Selection`.

**Commits.** A field reports `Signal::kChange` on every edit, `kBegin` before the first, and `kCommit` and `kEnd` when
enter commits or the field loses the keyboard. Escape puts back the text the field started with.

**Ellipses.** `"Overflow": "Ellipsis"` on a label cuts a line too long for its box short with `…`.

## Scale and languages

**Scale.** Layouts are in units, not pixels: a target's pixels over its pixel ratio, which is the display's scale
(`SetDensity`) times the player's choice (`SetScale`). A window 300 wide covers 450 pixels at 150%, and text, corners,
borders and shadows stay sharp at any scale. An attached interface follows the window to monitors of another DPI.

```cpp
mInterface.SetDensity(Monitor.GetScale());   // the monitor the window opens on
mInterface.SetScale(1.25f);                  // the player's choice
```

**Languages.** A `Lexicon` holds a string table per language, read from a `.lang` file. An object groups keys under its
name, and a text may name values of the model in braces.

```json
{ "Menu": { "Play": "Play", "Gold": "Gold: {Player.Gold:0}" } }
```

```cpp
mLexicon.Read("English", EnglishText, Error);
mLexicon.Read("Spanish", SpanishText, Error);
mLexicon.SetLanguage("English");
Binder::SetLexicon(mInterface, AddressOf(mLexicon));
```

Any text a layout can bind may be written `"@Key"`: `"Text": "@Menu.Play"`, tooltips, titles, dialog messages and
buttons, hints, menu items and list items. `@@` writes an at sign. `SetLanguage` changes every such text at the next
update without rebuilding anything. A missing key shows itself and is logged once.

## Drawing

**Images.** An `Image` shows a picture of the content or any GPU texture, such as a render target. A layout gives it
`Source`, `Region` (`[u0, v0, u1, v1]`), `Layer` (the slice of an array), `Tint`, `Filter` (`Linear`, or `Point` for pixel
art), `Fit` (`Stretch`, or `Contain` to keep its shape), `Opaque` and `Flip`. Most of these bind, so each row of a list
can show its own thumbnail or frame of a sheet:

```json
{ "Widget": "Repeater", "Items": "{Assets}", "ItemHeight": 40, "ItemTemplate": { "Flow": "Row", "Children": [
    { "Widget": "Image", "Source": "{Thumb}", "Layer": "{Slice}", "Region": "{Frame}", "Fit": "Contain", "Width": 32, "Height": 32 },
    { "Widget": "Label", "Text": "{Name}" }
] } }
```

`Image::CreateView(Parent, Texture, Layout)` shows a GPU texture the game keeps alive, as a 3D viewport.

**The canvas.** `Canvas` draws boxes (rounded, outlined, gradient, shadowed, glowing), images and nine-slices, lines,
polylines, triangles, quads, circles, rings, arcs, ellipses and text, under a stack of transforms and clips.

**Sketches.** A `Sketch` on an element runs the game's draw code every frame inside the element's frame, for overlays,
gizmos and crop marks:

```cpp
Element View = Image::CreateView(Panel, Renderer.GetTarget());
View.Add<Sketch>().Code = Painter::Draw::Create<&Overlay::Draw>(this);   // void Draw(Ref<Canvas>, ConstRef<Painter::Brush>)
```

## Editor widgets

Four widgets for tools and editors:

| Widget | What it is |
|--------|------------|
| `Grid` | A table: a row per record, a column per value, under sortable, resizable headings. |
| `Outline` | A tree view of records and the lists of children they hold. |
| `Splitter` | Panes side by side or stacked, with draggable bars between them. |
| `Dock` | Tabbed regions along the edges and in the middle, and panels floating in windows. |

```json
{ "Widget": "Dock", "Height": 430, "Sizes": [ 220, 0, 0, 100 ], "Panels": [
    { "Name": "Scene", "Title": "@Editor.Scene", "Side": "Left", "Children": [
        { "Widget": "Outline", "Items": "{Scene}", "Selected": "{Editor.Picked}" }
    ] },
    { "Name": "Properties", "Title": "@Editor.Properties", "Children": [
        { "Widget": "Grid", "Items": "{Entities}", "Multiple": true, "Sort": "Name", "Columns": [
            { "Title": "@Editor.Name",  "Key": "Name" },
            { "Title": "@Editor.Level", "Key": "Level", "Width": 70, "Align": "End" }
        ] }
    ] },
    { "Name": "Console", "Title": "@Editor.Console", "Side": "Bottom", "Children": [
        { "Widget": "Splitter", "Panes": [ { "Size": 380, "Children": [ ] }, { "Children": [ ] } ] }
    ] }
] }
```

**Grid.** Its rows behave as a `Repeater`'s. A column keeps a `Width` or shares the room left, never below its
`Minimum`, and dragging a heading's grip resizes it. A click on a `Sortable` heading sorts by it, by a `Comparer` the game
gives or by value; the model keeps its own order. `Group` keeps records together ahead of the sort, as folders before
files. A column's `Cell` names a template to make each cell from.

**Outline.** Records holding children, or flagged `Expandable`, show an arrow; opening one reports `Outline::kExpand`,
so the game can fill children lazily. `Nested` names the list of children (`Children` unless given). `DragRows` lets
rows be dragged onto, or with `DropBetween` between, other rows, reported as `Outline::kDrop`; `Outline::GetLanding`
tells the source, target and spot, and the game moves the record in its model.

```json
{ "Widget": "Outline", "Items": "{Archetypes.Tree}", "DragRows": "Archetype", "Selected": "{Archetypes.Picked}",
  "RowStates": { "Disabled": "!Editable" } }
```

**Renaming in place.** A row's template holds a `Label` and a `Textbox`, swapped by a flag of the record. `F2` or a
double press asks to rename; the game sets the flag and focuses the field.

```json
"Row": { "Flow": "Row", "Grow": 1, "Children": [
    { "Widget": "Label",   "Text": "{Name}", "Hidden": "{Renaming}", "Grow": 1 },
    { "Widget": "Textbox", "Name": "Rename", "Text": "{Name}", "Hidden": "{!Renaming}", "Grow": 1 }
] }
```

**Splitter.** A pane given a `Size` in pixels keeps it, one given a share (`"25%"`) keeps that share, and the rest share
what is left. A double click on a bar folds a pane. A pane's `Hidden` may bind, so sidebars toggle from the model.
`Splitter::Save` and `Load` keep every splitter's sizes from one run to the next.

```json
{ "Widget": "Splitter", "Name": "Columns", "Panes": [
    { "Size": "20%", "Minimum": 120, "Hidden": "{!Ui.ShowLeft}", "Children": [
        { "Widget": "Splitter", "Name": "Left", "Orientation": "Vertical", "Panes": [ { "Size": "50%" }, { } ] } ] },
    { "Children": [
        { "Widget": "Splitter", "Name": "Center", "Orientation": "Vertical", "Panes": [ { }, { "Size": "25%" } ] } ] },
    { "Size": "25%", "Hidden": "{!Ui.ShowRight}" }
] }
```

**Tabs.** A tab given `"Closable": true` carries a close button reporting `Tabs::kClose`; a handler that stops it keeps
the tab.

**Dock.** Each panel has a `Name`, a `Title` and a `Side`: `Left`, `Right`, `Top`, `Bottom`, `Center` or `Float`. Dragging
a tab out floats its panel; dragging a window over the dock's targets docks it. `Dock::Save` and `Load` keep the layout as
JSON.

### Value fields

Every value widget reports `Signal::kBegin` and `kEnd` around the `kChange`s of one edit, so an editor's undo can group
them: a text field or scalar from its first change to its commit, a slider from press to release, a spinner per press,
notch or key, and a checkbox per click.

**Scalar.** A number typed into a field behind an optional tag, rounded to `Decimals` and held within `Minimum` and
`Maximum`. A vector is a row of them, tagged in the axis colours the theme gives (`Axis.X`, `Axis.Y`, `Axis.Z`, `Axis.W`):

```json
{ "Flow": "Row", "Gap": 2, "Children": [
    { "Widget": "Scalar", "Tag": "X", "TagStyle": "Axis.X", "Value": "{Pos.X}", "Decimals": 3, "Grow": 1 },
    { "Widget": "Scalar", "Tag": "Y", "TagStyle": "Axis.Y", "Value": "{Pos.Y}", "Decimals": 3, "Grow": 1 },
    { "Widget": "Scalar", "Tag": "Z", "TagStyle": "Axis.Z", "Value": "{Pos.Z}", "Decimals": 3, "Grow": 1 }
] }
```

**Swatch.** A box filled with a colour over a checkerboard, so transparency shows. Its `Value` is `#RRGGBBAA` or a bound
colour.

## Extending

A game's own module builds on the library the way the shipped widgets do, without editing it:

- **Widgets**: `Markup::Register` teaches the markup a widget, and `Markup::AddField` a field any node may give.
- **States, bands and signals** are declared by name: `States::Declare("Running")`, `Layers::Declare("Hud", Layer)`,
  and any `Identify("Name")` as an event's kind.
- **Input**: `Pointer::AddHook` sees input before the controls do, and `Dispatcher::AddListener` hears every event.
- **Drawing**: `Painter::AddDrawer`, `Layout::AddMeasurer` and `Animator::AddApplier` draw, size and animate custom data.
- **Passes**: `Interface::AddStage` runs code before styling or after layout each update.

## License

MIT. See [LICENSE](LICENSE).
