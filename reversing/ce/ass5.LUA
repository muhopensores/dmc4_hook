if getCEVersion() < 6.6 then
    showMessage(
        "WatchChangesToAddresses Lua extention could not be loaded: outdated CE veESIon. Please upgrade to CE 6.6 or later."
    )
    return
end

local memviewform = getMemoryViewForm()
local windowIsOpen = false
local miWatchChangesToAddress = createMenuItem(memviewform.Extra1)
miWatchChangesToAddress.Caption = "watch MTFramework objects on breakpoints"
memviewform.Extra1.insert(memviewform.Watchmemoryallocations1.MenuIndex + 1, miWatchChangesToAddress)
gLvResults = nil
miWatchChangesToAddress.OnClick = function()
    --[[ Place GUI elements ]]
    windowIsOpen = true
    local form = createForm(false)
    form.Height = 300
    form.Width = 400
    form.Caption = "Watch Changes to Addresses MTFramework edition"
    form.Constraints.MinHeight = 300
    form.Constraints.MinWidth = 400
    form.BorderStyle = bsSizeable
    form.Position = poScreenCenter

    local pnlOptions = createPanel(form)
    pnlOptions.Align = alLeft
    pnlOptions.Width = 150
    pnlOptions.BevelOuter = bvNone

    local lblInterval = createLabel(pnlOptions)
    lblInterval.AnchorSideLeft.Control = pnlOptions
    lblInterval.AnchorSideTop.Control = pnlOptions
    lblInterval.BorderSpacing.Left = 10
    lblInterval.BorderSpacing.Top = 10
    lblInterval.Caption = "wew"

    local lvResults = createListView(form)
    lvResults.AnchorSideLeft.Control = pnlOptions
    lvResults.AnchorSideLeft.Side = asrBottom
    lvResults.Align = alRight
    lvResults.Anchors = "[akLeft]"
    lvResults.AutoSort = false
    lvResults.HideSelection = false
    lvResults.MultiSelect = true
    lvResults.ReadOnly = true
    lvResults.RowSelect = true
    lvResults.ViewStyle = vsReport
    lvResults.Columns.add().Caption = "REG"
    lvResults.Columns.add().Caption = "ADDR"
    lvResults.Columns.add().Caption = "STRING"
    ---lvi=listview_getItems(lvResults);
    ---row1=listitems_add(lvi)
    ---listitem_setCaption(row1, 'Row 1'); --rw 1 column a
    ---row1_subitems=listitem_getSubItems(row1) --returns a Strings object
    ---strings_add(row1_subitems, 'r1_cb') --row 1 column b
    ---strings_add(row1_subitems, 'r1_cc') --row 1 column c
    gLvResults = lvResults

    --[[ End main implementation, begin GUI events ]]
    form.OnClose = function(sender)
        windowIsOpen = false
        return caFree
    end

    form.show()
end

function GetNameSafe(input)
    local ok, err = pcall(GetName, input)
    local pok, perr = pcall(GetName, readInteger(input))
    if not ok then
        if not pok then
          return "null"
        else
          if not perr == nil then
            return "Ptr -> "..perr
          else
            return "null"
          end
        end
    else
        return err
    end
end

function GetName(input)
    local vtable = readInteger(input)
    local getDTIfunction = readInteger(vtable + 0x10)
    local dti = readInteger(getDTIfunction + 0x1)
    local stringPtr = dti + 0x4
    local stringOffset = readInteger(stringPtr)
    --[[print(string.format("vtable=0x%X", vtable))
         print(string.format("getDTIfunction=0x%X", getDTIfunction))
         print(string.format("dti=0x%X", dti))
         print(string.format("stringOffset=0x%X", stringOffset))
         ]]
    return readString(stringOffset, 666)
end

function debugger_onBreakpoint()
    if not windowIsOpen then
        return 0
    else
        lvi = listview_getItems(gLvResults)
        lvi.clear()
        row1 = listitems_add(lvi)
        listitem_setCaption(row1, "EAX") --rw 1 column a
        row1_subitems = listitem_getSubItems(row1) --returns a Strings object
        strings_add(row1_subitems, string.format("0x%X", EAX)) --row 1 column b
        strings_add(row1_subitems, GetNameSafe(EAX)) --row 1 column c
        row2 = listitems_add(lvi)
        listitem_setCaption(row2, "EBX")
        row2_subitems = listitem_getSubItems(row2)
        strings_add(row2_subitems, string.format("0x%X", EBX))
        strings_add(row2_subitems, GetNameSafe(EBX))
        row3 = listitems_add(lvi)
        listitem_setCaption(row3, "ECX")
        row3_subitems = listitem_getSubItems(row3)
        strings_add(row3_subitems, string.format("0x%X", ECX))
        strings_add(row3_subitems, GetNameSafe(ECX))
        row4 = listitems_add(lvi)
        listitem_setCaption(row4, "EDX")
        row4_subitems = listitem_getSubItems(row4)
        strings_add(row4_subitems, string.format("0x%X", EDX))
        strings_add(row4_subitems, GetNameSafe(EDX))
        row5 = listitems_add(lvi)
        listitem_setCaption(row5, "EBP")
        row5_subitems = listitem_getSubItems(row5)
        strings_add(row5_subitems, string.format("0x%X", EBP))
        strings_add(row5_subitems, GetNameSafe(EBP))
        row6 = listitems_add(lvi)
        listitem_setCaption(row6, "ESI")
        row6_subitems = listitem_getSubItems(row6)
        strings_add(row6_subitems, string.format("0x%X", ESI))
        strings_add(row6_subitems, GetNameSafe(ESI))
        row7 = listitems_add(lvi)
        listitem_setCaption(row7, "EDI")
        row7_subitems = listitem_getSubItems(row7)
        strings_add(row7_subitems, string.format("0x%X", EDI))
        strings_add(row7_subitems, GetNameSafe(EDI))
        return 0
    end
end
