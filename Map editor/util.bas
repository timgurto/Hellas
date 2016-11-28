Attribute VB_Name = "Util"
Option Explicit

Public Function strToColor(str As String) As Long
    Dim r As String
    Dim g As String
    Dim b As String
    r = "&H" & Mid(str, 1, 2)
    g = "&H" & Mid(str, 3, 2) & "00"
    b = "&H" & Mid(str, 5, 2) & "0000"
    
    strToColor = Val(r) + Val(g) + Val(b)
End Function

Public Function bind(ByRef n As Integer, min As Integer, max As Integer)
    If n > max Then
        n = max
    ElseIf n < min Then
        n = min
    End If
End Function
