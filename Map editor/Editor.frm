VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form frmEditor 
   AutoRedraw      =   -1  'True
   Caption         =   "Form1"
   ClientHeight    =   16650
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   22590
   DrawWidth       =   4684
   LinkTopic       =   "Form1"
   ScaleHeight     =   16650
   ScaleWidth      =   22590
   StartUpPosition =   3  'Windows Default
   WindowState     =   2  'Maximized
   Begin VB.PictureBox picSettings 
      Height          =   7335
      Index           =   1
      Left            =   3240
      ScaleHeight     =   7275
      ScaleWidth      =   3195
      TabIndex        =   5
      Top             =   360
      Width           =   3255
      Begin VB.TextBox txtSpawnTime 
         Height          =   285
         Left            =   1200
         TabIndex        =   15
         Text            =   "10"
         Top             =   4200
         Width           =   735
      End
      Begin VB.TextBox txtSpawnRadius 
         Height          =   285
         Left            =   1200
         TabIndex        =   16
         Text            =   "100"
         Top             =   3960
         Width           =   735
      End
      Begin VB.TextBox txtSpawnQuantity 
         Height          =   285
         Left            =   1200
         TabIndex        =   14
         Text            =   "1"
         Top             =   3720
         Width           =   735
      End
      Begin VB.ListBox List1 
         Height          =   510
         Left            =   120
         Style           =   1  'Checkbox
         TabIndex        =   12
         Top             =   4920
         Width           =   2175
      End
      Begin VB.PictureBox picSpawnPreview 
         Height          =   2895
         Left            =   120
         ScaleHeight     =   2835
         ScaleWidth      =   2835
         TabIndex        =   8
         Top             =   720
         Width           =   2895
      End
      Begin VB.ComboBox cmbSpawnType 
         Height          =   315
         Left            =   120
         Style           =   2  'Dropdown List
         TabIndex        =   7
         Top             =   360
         Width           =   2895
      End
      Begin VB.Label Label6 
         Caption         =   "Terrain whitelist"
         Height          =   255
         Left            =   120
         TabIndex        =   13
         Top             =   4680
         Width           =   1215
      End
      Begin VB.Label Label5 
         Caption         =   "Respawn (m):"
         Height          =   255
         Left            =   120
         TabIndex        =   11
         Top             =   4200
         Width           =   975
      End
      Begin VB.Label Label4 
         Caption         =   "Radius:"
         Height          =   255
         Left            =   120
         TabIndex        =   10
         Top             =   3960
         Width           =   975
      End
      Begin VB.Label Label3 
         Caption         =   "Quantity:"
         Height          =   255
         Left            =   120
         TabIndex        =   9
         Top             =   3720
         Width           =   975
      End
      Begin VB.Label Label2 
         Caption         =   "Object type:"
         Height          =   255
         Left            =   120
         TabIndex        =   6
         Top             =   120
         Width           =   1095
      End
   End
   Begin VB.PictureBox picSettings 
      Height          =   7335
      Index           =   0
      Left            =   0
      ScaleHeight     =   7275
      ScaleWidth      =   3195
      TabIndex        =   3
      Top             =   360
      Width           =   3255
      Begin VB.Label Label1 
         Caption         =   "(Terrain)"
         Height          =   255
         Left            =   240
         TabIndex        =   4
         Top             =   2160
         Width           =   1575
      End
   End
   Begin ComctlLib.TabStrip tabs 
      Height          =   375
      Left            =   0
      TabIndex        =   2
      Top             =   0
      Width           =   3255
      _ExtentX        =   5741
      _ExtentY        =   661
      MultiRow        =   -1  'True
      Style           =   1
      ImageList       =   "tabImages"
      _Version        =   327682
      BeginProperty Tabs {0713E432-850A-101B-AFC0-4210102A8DA7} 
         NumTabs         =   2
         BeginProperty Tab1 {0713F341-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Terrain"
            Key             =   "tabTerrain"
            Object.Tag             =   ""
            ImageVarType    =   2
            ImageIndex      =   1
         EndProperty
         BeginProperty Tab2 {0713F341-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Spawn points"
            Key             =   "tabSpawn"
            Object.Tag             =   ""
            ImageVarType    =   2
            ImageIndex      =   2
         EndProperty
      EndProperty
   End
   Begin ComctlLib.StatusBar statusBar 
      Align           =   2  'Align Bottom
      Height          =   255
      Left            =   0
      TabIndex        =   1
      Top             =   16395
      Width           =   22590
      _ExtentX        =   39846
      _ExtentY        =   450
      SimpleText      =   ""
      _Version        =   327682
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   3
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Object.Width           =   1764
            MinWidth        =   1764
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.PictureBox picMap 
      AutoRedraw      =   -1  'True
      BackColor       =   &H00FF8080&
      ForeColor       =   &H00C0FFFF&
      Height          =   2760
      Left            =   19560
      ScaleHeight     =   180
      ScaleMode       =   3  'Pixel
      ScaleWidth      =   173
      TabIndex        =   0
      Top             =   13320
      Width           =   2655
   End
   Begin ComctlLib.ImageList tabImages 
      Left            =   18720
      Top             =   15240
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327682
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   2
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "Editor.frx":0000
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "Editor.frx":005E
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Menu mnuLoad 
      Caption         =   "&Load"
      Begin VB.Menu mnuLoadMap 
         Caption         =   "&Map"
      End
      Begin VB.Menu mnuLoadTerrain 
         Caption         =   "&Terrain"
      End
      Begin VB.Menu mnuObjects 
         Caption         =   "&Objects and NPCs"
      End
      Begin VB.Menu mnuLoadSpawnPoints 
         Caption         =   "&Spawn points"
      End
      Begin VB.Menu mnuGap1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuLoadAll 
         Caption         =   "&All"
         Shortcut        =   ^O
      End
   End
   Begin VB.Menu mnuSave 
      Caption         =   "&Save"
      Begin VB.Menu mnuSaveSpawnPoints 
         Caption         =   "&Spawn points"
      End
      Begin VB.Menu mnuGap2 
         Caption         =   "-"
      End
      Begin VB.Menu mnuSaveAll 
         Caption         =   "&All"
         Shortcut        =   ^S
      End
   End
   Begin VB.Menu mnuMisc 
      Caption         =   "&Misc"
      Begin VB.Menu mnuRefresh 
         Caption         =   "&Refresh map"
         Shortcut        =   {F5}
      End
   End
End
Attribute VB_Name = "frmEditor"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim DATA_PATH As String
Dim terrainColors(256) As Long
Dim spawnPoints() As SpawnPoint
Dim objectTypes() As ObjectType
Dim map() As Integer
Dim mapW As Long 'in tiles
Dim mapH As Long
Public offsetX As Long 'in picMap pixels
Public offsetY As Long
Public zoom As Double 'pixels per tile, default=1

Dim cursorX As Long 'The pixel being moused-over
Dim cursorY As Long
Dim locX As Long 'The map location being moused-over (accounting for offset and zoom)
Dim locY As Long

Dim mapDC As Long
Dim mapBM As Long
Dim oldMapDC As Long
Dim mapWP As Long
Dim mapHP As Long
Dim drawFont As Long

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
    offsetX = offsetX - 200 / zoom
    clipOffset
    draw
End Function

Function panRight()
    offsetX = offsetX + 200 / zoom
    clipOffset
    draw
End Function

Function panUp()
    offsetY = offsetY - 200 / zoom
    clipOffset
    draw
End Function

Function panDown()
    offsetY = offsetY + 200 / zoom
    clipOffset
    draw
End Function

Function updateLoc()
    locX = offsetX * 16 + cursorX / (zoom) * 32 - 16
    locY = offsetY * 16 + cursorY / (zoom) * 32
    Dim tileX, tileY As Long
    tileY = locY \ 32
    tileX = IIf(tileY Mod 2 = 0, locX + 16, locX) \ 32
    
    statusBar.Panels(2).Text = tileX & ", " & tileY
    statusBar.Panels(3).Text = locX & ", " & locY
End Function

Function draw()
    statusBar.Panels(1).Text = zoom & "x"
    updateLoc

    picMap.Cls
    picMap.AutoRedraw = True
    
    ' Push
    Dim oldFont As Long
    oldFont = SelectObject(picMap.hDC, drawFont)

    ' Map
    StretchBlt _
            picMap.hDC, 0, 0, picMap.ScaleWidth, picMap.ScaleHeight, _
            mapDC, offsetX, offsetY, picMap.ScaleWidth * 2 / zoom, picMap.ScaleHeight * 2 / zoom, _
            vbSrcCopy

    ' Spawn points
    Dim i As Integer
    For i = 1 To UBound(spawnPoints)
        With spawnPoints(i)
            picMap.Circle (convertX(.x), convertY(.y)), convertLength(.radius), vbWhite
            Dim name As String
            name = objectTypes(.type).name
            TextOut picMap.hDC, convertX(.x) - Len(name) / 2 * 6, convertY(.y) - 4.5, name, Len(name)
            
        End With
    Next i
    
    ' Prospective spawn point
    If tabs.SelectedItem.index = 2 Then ' 2 = Spawn points
        picMap.Circle (convertX(locX), convertY(locY)), convertLength(Val(txtSpawnRadius.Text)), vbYellow
    End If
    
    ' Pop
    SelectObject picMap.hDC, oldFont
    
    picMap.Refresh

End Function

Private Function findObjectType(id As String) As Integer
    Dim i As Integer
    For i = 1 To UBound(objectTypes)
        If objectTypes(i).id = id Then findObjectType = i
    Next i
End Function

Private Sub Form_Load()
    DATA_PATH = App.Path
    DATA_PATH = Left(DATA_PATH, InStrRev(DATA_PATH, "\"))
    DATA_PATH = DATA_PATH & "Data\"
    
    drawFont = CreateFont(9, 6, 400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Dina")
    
    zoom = 2
    
    tabs.SelectedItem = tabs.tabs(2)
    picSettings(1).ZOrder 0
    mnuLoadAll_Click
End Sub

Private Sub Form_Resize()
    statusBar.Top = Me.height - 945
    Dim i As Integer
    For i = 1 To picSettings.Count
        With picSettings(i - 1)
            .Left = 0
            .Top = tabs.Top + tabs.height
            .height = statusBar.Top - .Top
            .width = tabs.width
        End With
    Next i
    With picMap
        .Top = 0
        .Left = picSettings(0).width
        .width = Me.width - .Left
        .height = statusBar.Top
    End With
    draw
End Sub

Private Sub Form_Unload(Cancel As Integer)
  SelectObject mapDC, oldMapDC
  DeleteObject mapBM
  DeleteDC mapDC
  DeleteObject drawFont
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
            X1 = (x - 1) * 2 + IIf(y Mod 2 = 0, 1, 0) - offsetX
            Y1 = (y - 1) * 2 - offsetY
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

Private Sub mnuLoadAll_Click()
    mnuLoadTerrain_Click
    mnuLoadMap_Click
    mnuObjects_Click
    mnuLoadSpawnPoints_Click
End Sub

Private Sub mnuLoadMap_Click()
    Dim xDoc As DOMDocument
    Set xDoc = loadXML(DATA_PATH & "map.xml")
    
    Dim root As IXMLDOMNode
    Set root = xDoc.documentElement
    Dim size As IXMLDOMNode
    Set size = root.selectSingleNode("size")
    mapW = getAttr(size, "x")
    mapH = getAttr(size, "y")
    ReDim map(mapW, mapH)
    
    Dim row As IXMLDOMNode
    For Each row In root.selectNodes("row")
        Dim y As Integer
        y = getAttr(row, "y")
        Dim tiles As String
        tiles = getAttrString(row, "terrain")
        Dim x As Integer
        For x = 1 To mapW
            map(x, y) = Asc(Mid(tiles, x, 1))
        Next x
    Next
    
    generateMapImage
    
End Sub

Private Sub mnuObjects_Click()
    Dim xDoc As DOMDocument
    Set xDoc = loadXML(DATA_PATH & "objectTypes.xml")
    
    Dim root As IXMLDOMNode
    Set root = xDoc.documentElement
    Dim entries As IXMLDOMNodeList
    Set entries = root.selectNodes("objectType")
    ReDim objectTypes(entries.length)
    Dim entry As IXMLDOMNode
    Dim i As Integer
    i = 1
    For Each entry In entries
        With objectTypes(i)
            .id = getAttrString(entry, "id")
            .name = getAttrString(entry, "name")
        End With
        i = i + 1
    Next
    
    Set xDoc = loadXML(DATA_PATH & "npcTypes.xml")
    Set root = xDoc.documentElement
    Set entries = root.selectNodes("npcType")
    Dim oldSize As Integer
    oldSize = UBound(objectTypes)
    ReDim Preserve objectTypes(oldSize + entries.length)
    For Each entry In entries
        With objectTypes(i)
            .id = getAttrString(entry, "id")
            .name = getAttrString(entry, "name")
        End With
        i = i + 1
    Next
    
    ' Populate stuff
    For i = 1 To UBound(objectTypes)
        cmbSpawnType.AddItem (objectTypes(i).name)
    Next i
    cmbSpawnType.ListIndex = 0
    
End Sub

Private Sub mnuLoadSpawnPoints_Click()
    Dim xDoc As DOMDocument
    Set xDoc = loadXML(DATA_PATH & "spawnPoints.xml")
    
    Dim root As IXMLDOMNode
    Set root = xDoc.documentElement
    Dim entries As IXMLDOMNodeList
    Set entries = root.selectNodes("spawnPoint")
    ReDim spawnPoints(entries.length)
    Dim entry As IXMLDOMNode
    Dim i As Integer
    i = 1
    For Each entry In entries
        With spawnPoints(i)
            .type = findObjectType(getAttrString(entry, "type"))
            .quantity = getAttr(entry, "quantity")
            .radius = getAttr(entry, "radius")
            .respawnTime = getAttr(entry, "respawnTime")
            .x = getAttr(entry, "x")
            .y = getAttr(entry, "y")
        End With
        i = i + 1
    Next
End Sub

Private Sub mnuLoadTerrain_Click()
    Dim xDoc As DOMDocument
    Set xDoc = loadXML(DATA_PATH & "terrain.xml")
    
    Dim root As IXMLDOMNode
    Set root = xDoc.documentElement
    Dim entries As IXMLDOMNodeList
    Set entries = root.selectNodes("terrain")
    Dim terrain As IXMLDOMNode
    For Each terrain In entries
        Dim colorString As String
        Dim index As String
        colorString = getAttrString(terrain, "color")
        index = getAttrString(terrain, "index")
        terrainColors(Asc(index)) = strToColor(colorString)
    Next
End Sub

Private Sub mnuRefresh_Click()
    draw
End Sub

Private Sub mnuSaveAll_Click()
    mnuSaveSpawnPoints_Click
End Sub

Private Sub writeAttr(name As String, attr As Variant)
    Print #1, " " & name & "=" & """" & attr & """";
End Sub

Private Sub mnuSaveSpawnPoints_Click()
    Open DATA_PATH + "spawnPoints.xml" For Output As #1
    Print #1, "<root>"
    Dim i As Integer
    For i = 1 To UBound(spawnPoints)
        With spawnPoints(i)
            Print #1, "<spawnPoint";
            writeAttr "type", objectTypes(.type).id
            writeAttr "quantity", .quantity
            writeAttr "radius", .radius
            writeAttr "respawnTime", .respawnTime
            writeAttr "x", .x
            writeAttr "y", .y
            Print #1, " />"
        End With
    Next i
    Print #1, "</root>"
    Close #1
End Sub

Private Sub picMap_Click()
    If tabs.SelectedItem.index = 2 Then ' 2 = Spawn points
        Dim NewIndex As Integer
        NewIndex = UBound(spawnPoints) + 1
        ReDim Preserve spawnPoints(NewIndex)
        Dim sp As SpawnPoint
        With sp
            .type = cmbSpawnType.ListIndex + 1
            .quantity = txtSpawnQuantity.Text
            .radius = txtSpawnRadius.Text
            .respawnTime = txtSpawnTime.Text * 60 * 1000
            .x = locX
            .y = locY
        End With
        spawnPoints(NewIndex) = sp
        draw
    End If
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
    
    draw
End Sub


Private Sub tabs_Click()
    picSettings(tabs.SelectedItem.index - 1).ZOrder 0
End Sub

Private Sub txtSpawnQuantity_Validate(Cancel As Boolean)
    Cancel = Not IsNumeric(txtSpawnQuantity.Text)
End Sub
