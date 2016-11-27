VERSION 5.00
Begin VB.Form Form1 
   AutoRedraw      =   -1  'True
   Caption         =   "Form1"
   ClientHeight    =   9645
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   11460
   LinkTopic       =   "Form1"
   ScaleHeight     =   9645
   ScaleWidth      =   11460
   StartUpPosition =   3  'Windows Default
   Begin VB.PictureBox picMap 
      BackColor       =   &H00FF8080&
      Height          =   3735
      Left            =   1440
      ScaleHeight     =   3675
      ScaleWidth      =   7155
      TabIndex        =   0
      Top             =   1560
      Width           =   7215
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
Dim mapWidth As Integer
Dim mapHeight As Integer


Function draw()
    Dim x As Integer
    Dim y As Integer
    For x = 1 To mapWidth
        For y = 1 To mapHeight
            Dim color As Long
            color = terrainColors(map(x, y))
            picMap.PSet (x * Screen.TwipsPerPixelX, y * Screen.TwipsPerPixelY), color
        Next y
    Next x
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
    mapWidth = CInt(size.Attributes.getNamedItem("x").nodeValue)
    mapHeight = CInt(size.Attributes.getNamedItem("y").nodeValue)
    ReDim map(mapWidth, mapHeight)
    
    Dim row As IXMLDOMNode
    For Each row In root.selectNodes("row")
        Dim y As Integer
        y = row.Attributes.getNamedItem("y").nodeValue
        Dim tiles As String
        tiles = row.Attributes.getNamedItem("terrain").nodeValue
        Dim x As Integer
        For x = 1 To mapWidth
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
