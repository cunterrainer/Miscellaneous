import nigui, strutils, strformat

type InputPair = object
    container: LayoutContainer
    button: Button
    input: TextBox
    unit: Label

proc CreatePair(name: string, unit: string) : InputPair =
    var pair = InputPair(container: newLayoutContainer(Layout_Horizontal), button: newButton(name), input: newTextBox(), unit: newLabel(unit))
    pair.container.add(pair.button)
    pair.container.add(pair.input)
    pair.container.add(pair.unit)
    pair.button.width = 150
    pair.button.height = pair.button.height+5
    pair.unit.width = 50
    pair.input.height = pair.button.height-1
    pair.input.fontSize = pair.input.fontSize+5
    pair.input.onTextChange = proc(event: TextChangeEvent) =
        if pair.input.text.len > 0 and "1234567890.".find(pair.input.text[pair.input.text.len-1]) == -1:
            pair.input.text = pair.input.text[0 ..< pair.input.text.high]
            pair.input.cursorPos = pair.input.text.len
    return pair

proc main() =
    app.init()
    var window = newWindow("Ohm's law")
    window.width = 600.scaleToDpi
    window.height = 400.scaleToDpi
    var verticalContainer = newLayoutContainer(Layout_Vertical)
    window.add(verticalContainer)

    var voltage = CreatePair("Voltage", "Volts")
    var current = CreatePair("Current", "Ampers")
    var resistance = CreatePair("Resistance", "Ohms")
    var power = CreatePair("Power", "Watts")
    verticalContainer.add(voltage.container)
    verticalContainer.add(current.container)
    verticalContainer.add(resistance.container)
    verticalContainer.add(power.container)

    voltage.button.onClick = proc(event: ClickEvent) =
        if current.input.text.len != 0 and resistance.input.text.len != 0:
            voltage.input.text = fmt"{parseFloat(current.input.text)*parseFloat(resistance.input.text)}"
        elif power.input.text.len != 0 and current.input.text.len != 0:
            voltage.input.text = fmt"{parseFloat(power.input.text)/parseFloat(current.input.text)}"

    current.button.onClick = proc(event: ClickEvent) =
        if voltage.input.text.len != 0 and resistance.input.text.len != 0:
            current.input.text = fmt"{parseFloat(voltage.input.text)/parseFloat(resistance.input.text)}"
        elif power.input.text.len != 0 and voltage.input.text.len != 0:
            current.input.text = fmt"{parseFloat(power.input.text)/parseFloat(voltage.input.text)}"

    resistance.button.onClick = proc(event: ClickEvent) =
        if voltage.input.text.len != 0 and current.input.text.len != 0:
            voltage.input.text = fmt"{parseFloat(voltage.input.text)/parseFloat(current.input.text)}"

    power.button.onClick = proc(event: ClickEvent) =
        if voltage.input.text.len != 0 and current.input.text.len != 0:
            voltage.input.text = fmt"{parseFloat(voltage.input.text)*parseFloat(current.input.text)}"
        
    window.show()
    app.run()

main()