VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   9135
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   11460
   DrawWidth       =   4684
   LinkTopic       =   "Form1"
   ScaleHeight     =   9135
   ScaleWidth      =   11460
   StartUpPosition =   3  'Windows Default
   Begin MSComctlLib.StatusBar statusBar 
      Align           =   2  'Align Bottom
      Height          =   255
      Left            =   0
      TabIndex        =   1
      Top             =   8880
      Width           =   11460
      _ExtentX        =   20214
      _ExtentY        =   450
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   2
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            Object.Width           =   1764
            MinWidth        =   1764
            Picture         =   "Editor.frx":0000
         EndProperty
         BeginProperty Panel2 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            Picture         =   "Editor.frx":0352
         EndProperty
      EndProperty
   End
   Begin VB.PictureBox picMap 
      AutoRedraw      =   -1  'True
      BackColor       =   &H00FF8080&
      Height          =   7560
      Left            =   0
      ScaleHeight     =   500
      ScaleMode       =   3  'Pixel
      ScaleWidth      =   581
      TabIndex        =   0
      Top             =   0
      Width           =   8775
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
Dim mapW As Long 'in tiles
Dim mapH As Long
Dim offsetX As Long
Dim offsetY As Long
Dim zoom As Long 'pixels per tile, default=2

Dim cursorX As Long 'The pixel being moused-over
Dim cursorY As Long
Dim locX As Long 'The map location being moused-over (accounting for offset and zoom)
Dim locY As Long

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

Function updateLoc()
    locX = (cursorX + offsetX) * 32 / zoom
    locY = (cursorY + offsetY) * 32 / zoom

    statusBar.Panels(2).Text = locX & ", " & locY
End Function

Function draw()
    statusBar.Panels(1).Text = zoom & "x"
    updateLoc

    picMap.Cls
    picMap.AutoRedraw = True
    
    Dim minX As Long
    Dim minY As Long
    Dim maxX As Long
    Dim maxY As Long
    minX = offsetX / zoom
    minY = offsetY / zoom
    maxX = minX + picMap.ScaleWidth / zoom
    maxY = minY + picMap.ScaleHeight / zoom
    bind minX, 0, mapW
    bind minY, 0, mapH
    bind maxX, 0, mapW
    bind maxY, 0, mapH
    
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

Private Sub Form_Resize()
    statusBar.Top = Form1.height - 945
    'statusBar.Width = Form1.width
    picMap.width = Form1.width
    picMap.height = statusBar.Top
    draw
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

Private Sub picMap_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    cursorX = X
    cursorY = Y
    updateLoc
End Sub
