Attribute VB_Name = "XML"
Option Explicit

Public Function printError(ByRef xDoc As DOMDocument)
    Dim strErrText As String
    With xDoc.parseError
        strErrText = "Your XML Document failed to load" & _
                "due the following error." & vbCrLf & _
                "Error #: " & .errorCode & ": " & .reason & _
                "Line #: " & .Line & vbCrLf & _
                "Line Position: " & .linepos & vbCrLf & _
                "Position In File: " & .filepos & vbCrLf & _
                "Source Text: " & .srcText & vbCrLf & _
                "Document URL: " & .url
    End With
    MsgBox strErrText, vbExclamation
End Function

Public Function loadXML(ByVal filename As String) As DOMDocument
    Dim xDoc As DOMDocument
    Set xDoc = New DOMDocument
    Dim ret As Boolean
    ret = xDoc.Load(filename)
    If Not ret Then printError xDoc
    Set loadXML = xDoc
End Function

Public Function getAttr(ByRef node As IXMLDOMNode, name As String) As Double
    getAttr = node.Attributes.getNamedItem(name).nodeValue
End Function

Public Function getAttrString(ByRef node As IXMLDOMNode, name As String) As String
    getAttrString = CStr(node.Attributes.getNamedItem(name).nodeValue)
End Function
