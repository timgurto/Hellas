VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   9930
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   11460
   DrawWidth       =   4684
   LinkTopic       =   "Form1"
   ScaleHeight     =   9930
   ScaleWidth      =   11460
   StartUpPosition =   3  'Windows Default
   Begin VB.PictureBox picMap 
      AutoRedraw      =   -1  'True
      BackColor       =   &H00FF8080&
      Height          =   7215
      Left            =   1440
      ScaleHeight     =   477
      ScaleMode       =   3  'Pixel
      ScaleWidth      =   573
      TabIndex        =   0
      Top             =   1560
      Width           =   8655
   End
   Begin VB.Label lblOffset 
      Caption         =   "Offset: "
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   360
      Width           =   1215
   End
   Begin VB.Label lblZoom 
      Caption         =   "2x zoom"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   1215
   End
   Begin VB.Menu mnuLoad 
      Caption         =   "&Load"
      Begin VB.Menu mnuLoadMap 
         Caption         =   "&Map"
      End
      Begin VB.Menu mnuLoadTerrain 
         Caption         =   "&Terrain"
      End
      Begin VB.Menu mnuGap1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuLoadAll 
         Caption         =   "&All"
         Shortcut        =   ^L
      End
   End
   Begin VB.Menu mnuMisc 
      Caption         =   "&Misc"
      Begin VB.Menu mnuRefresh 
         Caption         =   "&Refresh"
         Shortcut        =   ^R
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim DATA_PATH As String
Dim terrainColors() As Long
Dim map() As Integer
Dim mapW As Integer 'in tiles
Dim mapH As Integer
Dim offsetX As Long
Dim offsetY As Long
Dim zoom As Integer 'pixels per tile, default=2

Function zoomIn()
    zoom = zoom * 2
    draw
End Function

Function zoomOut()
    If zoom >= 2 Then
        zoom = zoom / 2
    End If
    draw
End Function

Function panLeft()
    offsetX = offsetX - 100
    If offsetX < 0 Then offsetX = 0
    draw
End Function

Function panRight()
    offsetX = offsetX + 100
    If offsetX + picMap.ScaleWidth > mapW * zoom Then offsetX = mapW * zoom - picMap.ScaleWidth
    draw
End Function

Function panUp()
    offsetY = offsetY - 100
    If offsetY < 0 Then offsetY = 0
    draw
End Function

Function panDown()
    offsetY = offsetY + 100
    If offsetY + picMap.ScaleHeight > mapH * zoom Then offsetY = mapH * zoom - picMap.ScaleHeight
    draw
End Function

Function draw()
    lblZoom.Caption = zoom & "x zoom"
    lblOffset.Caption = "Offset: " & offsetX & ", " & offsetY

    picMap.Cls
    picMap.AutoRedraw = True
    
    Dim minX As Integer
    Dim minY As Integer
    Dim maxX As Integer
    Dim maxY As Integer
    minX = offsetX / zoom
    minY = offsetY / zoom
    maxX = minX + picMap.ScaleWidth / zoom
    maxY = minY + picMap.ScaleHeight / zoom
    bind minX, 0, mapW
    bind minY, 0, mapW
    bind maxX, 0, mapW
    bind maxY, 0, mapW
    
    Dim X As Integer
    Dim Y As Integer
    Dim rectSize As Integer
    For X = minX To maxX
        For Y = minY To maxY
            Dim X1 As Long
            Dim Y1 As Long
            X1 = X * zoom + IIf(Y Mod 2 = 0, zoom / 2, 0) - 1 - offsetX
            Y1 = Y * zoom - 1 - offsetY
            Dim color As Long
            color = terrainColors(map(X, Y))
            picMap.Line (X1, Y1)-(X1 + (zoom - 1), Y1 + (zoom - 1)), color, BF
        Next Y
    Next X
    
    picMap.Refresh
End Function

Private Sub Form_Load()
    DATA_PATH = App.Path
    DATA_PATH = Left(DATA_PATH, InStrRev(DATA_PATH, "\"))
    DATA_PATH = DATA_PATH & "Data\"
    
    zoom = 2
    
    picMap.Cls
    mnuLoadAll_Click
End Sub

Private Sub mnuLoadAll_Click()
    mnuLoadTerrain_Click
    mnuLoadMap_Click
End Sub

Private Sub mnuLoadMap_Click()
    Dim xDoc As DOMDocument
    Set xDoc = loadXML(DATA_PATH & "map.xml")
    
    Dim root As IXMLDOMNode
    Set root = xDoc.documentElement
    Dim size As IXMLDOMNode
    Set size = root.selectSingleNode("size")
    mapW = CInt(size.Attributes.getNamedItem("x").nodeValue)
    mapH = CInt(size.Attributes.getNamedItem("y").nodeValue)
    ReDim map(mapW, mapH)
    
    Dim row As IXMLDOMNode
    For Each row In root.selectNodes("row")
        Dim Y As Integer
        Y = row.Attributes.getNamedItem("y").nodeValue
        Dim tiles As String
        tiles = row.Attributes.getNamedItem("terrain").nodeValue
        Dim X As Integer
        For X = 1 To mapW
            map(X, Y) = Mid(tiles, X, 1)
        Next X
    Next
        
    draw
    
End Sub

Private Sub mnuLoadTerrain_Click()
    Dim xDoc As DOMDocument
    Set xDoc = loadXML(DATA_PATH & "terrain.xml")
    
    Dim root As IXMLDOMNode
    Set root = xDoc.documentElement
    Dim entries As IXMLDOMNodeList
    Set entries = root.selectNodes("terrain")
    ReDim terrainColors(entries.length)
    Dim terrain As IXMLDOMNode
    For Each terrain In entries
        Dim colorString As String
        Dim index As Integer
        colorString = CStr(terrain.Attributes.getNamedItem("color").nodeValue)
        index = terrain.Attributes.getNamedItem("index").nodeValue
        terrainColors(index) = strToColor(colorString)
    Next
End Sub

Private Sub mnuRefresh_Click()
    draw
End Sub

Private Sub picMap_KeyDown(KeyCode As Integer, Shift As Integer)
    Select Case KeyCode
    Case vbKeyPageDown, vbKeyAdd
        zoomIn
    Case vbKeyPageUp, vbKeySubtract
        zoomOut
    Case vbKeyUp
        panUp
    Case vbKeyDown
        panDown
    Case vbKeyLeft
        panLeft
    Case vbKeyRight
        panRight
    
    End Select
End Sub

