Attribute VB_Name = "Types"
Option Explicit


Public Type Bitmap
    hDC As Long
    hBM As Long
    oldhDC As Long
    width As Long
    height As Long
End Type

Public Type SpawnPoint
    type As String
    quantity As Integer
    radius As Long
    respawnTime As Long
    x As Long
    y As Long
    terrainWhitelist() As Integer
End Type

Public Type ObjectType
    id As String
    name As String
End Type
