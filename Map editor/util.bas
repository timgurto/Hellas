Attribute VB_Name = "Util"
Option Explicit

Public Declare Function GetMem4 Lib "msvbvm60" (ByVal pSrc As Long, ByVal pDst As Long) As Long
Public Declare Function ArrPtr Lib "msvbvm60" Alias "VarPtr" (arr() As Any) As Long

Public Function strToColor(str As String) As Long
    Dim r As String
    Dim g As String
    Dim b As String
    r = "&H" & Mid(str, 1, 2)
    g = "&H" & Mid(str, 3, 2) & "00"
    b = "&H" & Mid(str, 5, 2) & "0000"
    
    strToColor = Val(r) + Val(g) + Val(b)
End Function

Public Function bind(ByRef n As Long, min As Long, max As Long)
    If n > max Then
        n = max
    ElseIf n < min Then
        n = min
    End If
End Function

Public Function StrArrPtr(arr() As String, Optional ByVal IgnoreMe As Long = 0) As Long
  GetMem4 VarPtr(IgnoreMe) - 4, VarPtr(StrArrPtr)
End Function

Public Function UDTArrPtr(ByRef arr As Variant) As Long
  If VarType(arr) Or vbArray Then
    GetMem4 VarPtr(arr) + 8, VarPtr(UDTArrPtr)
  Else
    Err.Raise 5, , "Variant must contain array of user defined type"
  End If
End Function


Public Function ArrayExists(ByVal ppArray As Long) As Long
  GetMem4 ppArray, VarPtr(ArrayExists)
End Function


