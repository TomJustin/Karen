Option Explicit

' Legacy convenience launcher. The current package starts Karen.exe directly.
Dim fso, shell, appDir, exePath

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

appDir = fso.GetParentFolderName(WScript.ScriptFullName)
exePath = fso.BuildPath(appDir, "Karen.exe")

If Not fso.FileExists(exePath) Then
    MsgBox "未找到 Karen.exe。请确认已完整解压安装包。", vbExclamation, "Karen"
    WScript.Quit 1
End If

shell.CurrentDirectory = appDir
shell.Run """" & exePath & """", 1, False
