Option Explicit

Dim fso, shell, appDir, exePath, request, attempt, ready

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

appDir = fso.GetParentFolderName(WScript.ScriptFullName)
exePath = fso.BuildPath(appDir, "Karen.exe")

If Not fso.FileExists(exePath) Then
    MsgBox "未找到 Karen.exe。请确认你已完整解压安装包。", vbExclamation, "Karen"
    WScript.Quit 1
End If

' Anchor all relative paths to the directory containing Karen.exe.
shell.CurrentDirectory = appDir
shell.Run """" & exePath & """", 0, False

' Wait until the local backend responds before opening the browser.
ready = False

For attempt = 1 To 40
    WScript.Sleep 500

    On Error Resume Next
    Set request = CreateObject("WinHttp.WinHttpRequest.5.1")
    request.SetTimeouts 500, 500, 500, 1000
    request.Open "GET", "http://127.0.0.1:8080/api/config/status", False
    request.Send

    If Err.Number = 0 Then
        If request.Status = 200 Then
            ready = True
        End If
    End If

    Err.Clear
    On Error GoTo 0

    If ready Then Exit For
Next

If ready Then
    shell.Run "http://127.0.0.1:8080/", 1, False
Else
    MsgBox "Karen 未能在 20 秒内启动。" & vbCrLf & _
           "请检查端口 8080 是否被占用，或查看 Windows 安全软件是否阻止了程序。", _
           vbExclamation, "Karen"
End If
