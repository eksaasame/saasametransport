Function LPad (str, pad, length)
    LPad = String(length - Len(str), pad) & str
End Function
curDate = Year(Now) & "-"& LPad( Month(Now), "0", 2 ) & "-" & LPad( Day(Now), "0", 2 ) & " " & LPad( Hour(Now), "0", 2 ) & ":" & LPad( Minute(Now), "0", 2 ) & ":" & LPad( Second(Now), "0", 2 )
WScript.Echo curDate