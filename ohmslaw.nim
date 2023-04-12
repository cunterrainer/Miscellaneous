import nigui, strutils, strformat, math

type InputPair = object
    container: LayoutContainer
    button: Button
    input: TextBox
    units: ComboBox
    unitsIdx: int

proc CreatePair(name: string, units: array[3, string]) : InputPair =
    var pair = InputPair(container: newLayoutContainer(Layout_Horizontal), button: newButton(name), input: newTextBox(), units: newComboBox(@units))
    pair.container.add(pair.button)
    pair.container.add(pair.input)
    pair.container.add(pair.units)
    pair.button.width = 150
    pair.button.height = pair.button.height+5
    pair.units.width = 100
    pair.units.fontSize = pair.input.fontSize+3
    pair.unitsIdx = pair.units.index
    pair.input.height = pair.button.height-1
    pair.input.fontSize = pair.input.fontSize+5
    pair.input.onTextChange = proc(event: TextChangeEvent) =
        if pair.input.text.len > 0 and "1234567890.".find(pair.input.text[pair.input.text.len-1]) == -1:
            pair.input.text = pair.input.text[0 ..< pair.input.text.high]
            pair.input.cursorPos = pair.input.text.len
    return pair

proc FormatFloat(value: float) : string =
    if value.toInt.toFloat == value: # no decimal places
        return fmt"{value.toInt}"
    var valStr = value.formatBiggestFloat(ffDecimal, 16)
    while valStr[valStr.len-1] == '0':
        valStr = valStr[0 ..< valStr.high]
    return valStr

proc ChangeUnit(value: float, prevUnitIdx: int, currUnitIdx:int, isResistance: bool) : float =
    if not isResistance:
        return value*pow(1000, (currUnitIdx-prevUnitIdx).float)
    return value/(1*pow(1000, (currUnitIdx-prevUnitIdx).float))

proc Calc(first: string, second: string, divide: bool, currUnitIdx: int, isResistance: bool) : string =
    if first.len == 0 or second.len == 0:
        return
    var x = parseFloat(first)
    var y = parseFloat(second)
    var res = if divide: x/y else: x*y
    return FormatFloat(ChangeUnit(res, 0, currUnitIdx, isResistance))

proc ToDefaultUnit(pair: InputPair, isResistance: bool) : string =
    if pair.input.text.isEmptyOrWhitespace: return
    return fmt"{ChangeUnit(parseFloat(pair.input.text), pair.units.index, 0, isResistance)}"

proc main() =
    app.init()
    var window = newWindow("Ohm's law")
    window.width = 600.scaleToDpi
    window.height = 400.scaleToDpi
    var verticalContainer = newLayoutContainer(Layout_Vertical)
    window.add(verticalContainer)

    var voltage = CreatePair("Voltage", ["Volts", "Millivolts", "Microvolts"])
    var current = CreatePair("Current", ["Amps", "Milliamps", "Microamps"])
    var resistance = CreatePair("Resistance", ["Ohm", "Kilohm", "Megaohm"])
    var power = CreatePair("Power", ["Watts", "Milliwatts", "Microwatts"])
    verticalContainer.add(voltage.container)
    verticalContainer.add(current.container)
    verticalContainer.add(resistance.container)
    verticalContainer.add(power.container)

    voltage.units.onChange = proc(event: ComboBoxChangeEvent) =
        if voltage.input.text.isEmptyOrWhitespace: return
        voltage.input.text = fmt"{FormatFloat(ChangeUnit(parseFloat(voltage.input.text), voltage.unitsIdx, voltage.units.index, false))}"
        voltage.unitsIdx = voltage.units.index

    current.units.onChange = proc(event: ComboBoxChangeEvent) =
        if current.input.text.isEmptyOrWhitespace: return
        current.input.text = fmt"{FormatFloat(ChangeUnit(parseFloat(current.input.text), current.unitsIdx, current.units.index, false))}"
        current.unitsIdx = current.units.index

    resistance.units.onChange = proc(event: ComboBoxChangeEvent) =
        if resistance.input.text.isEmptyOrWhitespace: return
        resistance.input.text = fmt"{FormatFloat(ChangeUnit(parseFloat(resistance.input.text), resistance.unitsIdx, resistance.units.index, true))}"
        resistance.unitsIdx = resistance.units.index

    power.units.onChange = proc(event: ComboBoxChangeEvent) =
        if power.input.text.isEmptyOrWhitespace: return
        power.input.text = fmt"{FormatFloat(ChangeUnit(parseFloat(power.input.text), power.unitsIdx, power.units.index, false))}"
        power.unitsIdx = power.units.index

    voltage.button.onClick = proc(event: ClickEvent) =
        voltage.input.text = Calc(ToDefaultUnit(current, false), ToDefaultUnit(resistance, true), false, voltage.units.index, false)
        if(voltage.input.text.isEmptyOrWhitespace):
            voltage.input.text = Calc(ToDefaultUnit(power, false), ToDefaultUnit(current, false), true, voltage.units.index, false)

    current.button.onClick = proc(event: ClickEvent) =
        current.input.text = Calc(ToDefaultUnit(voltage, false), ToDefaultUnit(resistance, true), true, current.units.index, false)
        if(current.input.text.isEmptyOrWhitespace):
            current.input.text = Calc(ToDefaultUnit(power, false), ToDefaultUnit(voltage, false), true, current.units.index, false)

    resistance.button.onClick = proc(event: ClickEvent) =
        resistance.input.text = Calc(ToDefaultUnit(voltage, false), ToDefaultUnit(current, false), true, resistance.units.index, true)

    power.button.onClick = proc(event: ClickEvent) =
        power.input.text = Calc(ToDefaultUnit(voltage, false), ToDefaultUnit(current, false), false, power.units.index, false)
        
    window.show()
    app.run()

main()