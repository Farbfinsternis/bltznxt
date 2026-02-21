; Banner aus dem Netz lesen
;-----------------------------------------------------------
Function LoadWebImage(webFile$) 
    If BlitzGet(webFile$,CurrentDir(),"banner.jpg") 
        image=LoadImage("banner.jpg")  
    EndIf 
    Return image 
End Function 

Function BlitzGet (webFile$, saveDir$, saveFile$) 
	If Left(webFile$,7)="http://" Then webFile$=Right(webFile$,Len(webFile$)-7) 

    slash=Instr(webFile$,"/") 
    If slash 
		webHost$=Left(webFile$,slash-1) 
        webFile$=Right(webFile$,Len(webFile$)-slash+1) 
    Else 
        webHost$=webFile$ 
        webFile$="/" 
    EndIf 
         
    If Right(saveDir$,1)<>"\" Then saveDir$=saveDir$+"\" 

    If saveFile$="" 
        If webFile="/" 
            saveFile$="Unknown file.txt" 
        Else 
            For findSlash=Len(webFile$) To 1 Step -1 
                testForSlash$=Mid(webFile$,findSlash,1) 
                If testForSlash$="/" 
                    saveFile$=Right(webFile$,Len(webFile$)-findSlash) 
                    Exit 
                EndIf 
            Next 
            If saveFile$="" Then saveFile$="Unknown file.txt" 
        EndIf 
    EndIf 

    www=OpenTCPStream(webHost$,80) 
    If www 
        WriteLine www,"GET "+webFile$+" HTTP/1.1" 
        WriteLine www,"Host: "+webHost$ 
        WriteLine www,"User-Agent: blox'n'balls" 
        WriteLine www,"Accept: */*" 
        WriteLine www,"" 
   
        Repeat 
            header$=ReadLine(www) 
            If Left(header$,16)="Content-Length: " 
                bytesToRead=Right(header$,Len(header$)-16) 
            EndIf 
        Until header$="" Or (Eof(www)) 
         
        If bytesToRead=0 Then Goto skipDownLoad 
         
        save=WriteFile(saveDir$+saveFile$) 
        If Not save Then Goto skipDownload 

        For readWebFile=1 To bytesToRead 
            If Not Eof(www) Then WriteByte save,ReadByte(www) 
            tReadWebFile=readWebFile 
            ;If tReadWebFile Mod 100=0 Then BytesReceived(readWebFile,bytesToRead) 
        Next 

        CloseFile save 
         
        If(readWebFile-1)=bytesToRead 
            success=1 
        EndIf 
         
        ;BytesReceived(bytesToRead,bytesToRead) 
         
        .skipDownload 
        CloseTCPStream www 
	EndIf 
   
    Return success 
     
End Function 

;-----------------------------------------------------------
Function BytesReceived(posByte,totalBytes) 
    Cls 
    Text 20,20,"Downloading file -- please wait..." 
    Text 20,40,"Received: "+posByte+"/"+totalBytes+" bytes ("+Percent(posByte,totalBytes)+"%)" 
    Flip 
End Function 

; ----------------------------------------------------------------------------- 
; Handy percentage function 
; ----------------------------------------------------------------------------- 
Function Percent (part#, total#) 
    Return Int(100*(part/total)) 
End Function 