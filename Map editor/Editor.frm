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

Function draw()
    picMap.Cls
    picMap.AutoRedraw = True
    
    Dim x As Integer
    Dim y As Integer
    For x = 1 To mapW
        For y = 1 To mapH
            Dim X1 As Long
            Dim Y1 As Long
            X1 = x * 2 + IIf(y Mod 2 = 0, 1, 0) - 1
            Y1 = y * 2 - 1
            Dim color As Long
            color = terrainColors(map(x, y))
            picMap.Line (X1, Y1)-(X1 + 1, Y1 + 1), color, BF
        Next y
    Next x
    
    picMap.Refresh
End Function

Private Sub Form_Load()
    DATA_PATH = App.Path
    DATA_PATH = Left(DATA_PATH, InStrRev(DATA_PATH, "\"))
    DATA_PATH = DATA_PATH & "Data\"
    
    picMap.Cls
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
        Dim y As Integer
        y = row.Attributes.getNamedItem("y").nodeValue
        Dim tiles As String
        tiles = row.Attributes.getNamedItem("terrain").nodeValue
        Dim x As Integer
        For x = 1 To mapW
            map(x, y) = Mid(tiles, x, 1)
        Next x
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
