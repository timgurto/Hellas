VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1 
   AutoRedraw      =   -1  'True
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
      DrawStyle       =   5  'Transparent
      ForeColor       =   &H00C0FFFF&
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
Dim offsetX As Long 'in map pixels
Dim offsetY As Long
Dim zoom As Double 'pixels per tile, default=1

Dim cursorX As Long 'The pixel being moused-over
Dim cursorY As Long
Dim locX As Long 'The map location being moused-over (accounting for offset and zoom)
Dim locY As Long

Dim mapDC As Long
Dim mapBM As Long
Dim oldMapDC As Long
Dim mapWP As Long
Dim mapHP As Long

Function clipOffset()
    If offsetX <= 0 Then
        offsetX = 0
    ElseIf offsetX > mapWP - picMap.ScaleWidth / (zoom / 2) Then
        offsetX = mapWP - picMap.ScaleWidth / (zoom / 2)
    End If
    If offsetY <= 0 Then
        offsetY = 0
    ElseIf offsetY > mapHP - picMap.ScaleHeight / (zoom / 2) Then
        offsetY = mapHP - picMap.ScaleHeight / (zoom / 2)
    End If
    
    ' Center if map is smaller than screen
    If picMap.ScaleWidth / (zoom / 2) > mapWP Then offsetX = (mapWP - picMap.ScaleWidth / (zoom / 2)) / 2
    If picMap.ScaleHeight / (zoom / 2) > mapHP Then offsetY = (mapHP - picMap.ScaleHeight / (zoom / 2)) / 2
End Function

Function zoomIn()
    zoom = zoom * 2
    offsetX = offsetX + picMap.ScaleWidth / (zoom / 2) / 2
    offsetY = offsetY + picMap.ScaleHeight / (zoom / 2) / 2
    clipOffset
    draw
End Function

Function zoomOut()
    zoom = zoom / 2
    offsetX = offsetX - picMap.ScaleWidth / (zoom / 2) / 4
    offsetY = offsetY - picMap.ScaleHeight / (zoom / 2) / 4
    clipOffset
    draw
End Function

Function panLeft()
    offsetX = offsetX - 100 / zoom
    clipOffset
    draw
End Function

Function panRight()
    offsetX = offsetX + 100 / zoom
    clipOffset
    draw
End Function

Function panUp()
    offsetY = offsetY - 100 / zoom
    clipOffset
    draw
End Function

Function panDown()
    offsetY = offsetY + 100 / zoom
    clipOffset
    draw
End Function

Function updateLoc()
    locX = (cursorX + offsetX) * 16 / zoom '16, not 32, because tiles are represented by 2x2 pixels and not 1x1.
    locY = (cursorY + offsetY) * 16 / zoom

    statusBar.Panels(2).Text = locX & ", " & locY
End Function

Function draw()
    statusBar.Panels(1).Text = zoom & "x"
    updateLoc

    picMap.Cls
    picMap.AutoRedraw = True

    StretchBlt _
            picMap.hdc, 0, 0, picMap.ScaleWidth, picMap.ScaleHeight, _
            mapDC, offsetX, offsetY, picMap.ScaleWidth * 2 / zoom, picMap.ScaleHeight * 2 / zoom, _
            vbSrcCopy
    
    picMap.Refresh

End Function

Private Sub Form_Load()
    DATA_PATH = App.Path
    DATA_PATH = left(DATA_PATH, InStrRev(DATA_PATH, "\"))
    DATA_PATH = DATA_PATH & "Data\"
    
    zoom = 2
    
    mnuLoadAll_Click
End Sub

Private Sub Form_Resize()
    statusBar.top = Form1.height - 945
    picMap.width = Form1.width
    picMap.height = statusBar.top
    draw
End Sub

Private Sub Form_Unload(Cancel As Integer)
  SelectObject mapDC, oldMapDC
  DeleteObject mapBM
  DeleteDC mapDC
End Sub

Private Sub mnuLoadAll_Click()
    mnuLoadTerrain_Click
    mnuLoadMap_Click
End Sub

Function generateMapImage()
    ' In case this isn't the first time it's been generated
    If mapBM <> 0 Then
        SelectObject mapDC, oldMapDC
        DeleteObject mapBM
        DeleteDC mapDC
    End If
    
    mapWP = mapW * 2 + 1
    mapHP = mapH * 2

    mapDC = CreateCompatibleDC(GetDC(0))
    mapBM = CreateCompatibleBitmap(GetDC(0), mapWP, mapHP)
    oldMapDC = SelectObject(mapDC, mapBM)
    
    ' Create terrain pens
    Dim numTerrains As Integer
    numTerrains = UBound(terrainColors)
    Dim pens() As Long
    ReDim pens(0 To numTerrains)
    Dim i As Integer
    For i = 0 To numTerrains
        pens(i) = CreatePen(vbSolid, 0, terrainColors(i))
    Next i
    
    ' Draw terrain
    Dim x As Integer
    Dim y As Integer
    For x = 1 To mapW
        For y = 1 To mapH
            Dim X1 As Long
            Dim Y1 As Long
            X1 = x * 2 + IIf(y Mod 2 = 0, 1, 0) - 1 - offsetX
            Y1 = y * 2 - 1 - offsetY
            Dim color As Long
            color = terrainColors(map(x, y))
            SelectObject mapDC, pens(map(x, y))
            'If map(x, y) = 0 Then
                'Dim pen As Long
                'pen = CreatePen(vbSolid, 1, color)
                Rectangle mapDC, X1, Y1, X1 + 2, Y1 + 2
                'DeleteObject pen
            'End If
        Next y
    Next x
    
    'Delete terrain pens
    For i = 1 To numTerrains
        DeleteObject pens(i)
    Next i

End Function

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
        Dim y As Integer
        y = row.Attributes.getNamedItem("y").nodeValue
        Dim tiles As String
        tiles = row.Attributes.getNamedItem("terrain").nodeValue
        Dim x As Integer
        For x = 1 To mapW
            map(x, y) = Mid(tiles, x, 1)
        Next x
    Next
    
    generateMapImage
        
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

Private Sub picMap_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    cursorX = x
    cursorY = y
    updateLoc
End Sub


