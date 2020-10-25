
Const ForReading = 1
Const ForWriting = 2

Set objFSO = CreateObject("Scripting.FileSystemObject")
objStartFolder = Wscript.Arguments(0)

Set objFolder = objFSO.GetFolder(objStartFolder)
Wscript.Echo objFolder.Path
Set colFiles = objFolder.Files
For Each objFile in colFiles
    Wscript.Echo objFile.Path
    Set objRead = objFSO.OpenTextFile( objFile.Path, ForReading)
	strText = objRead.ReadAll
	objRead.Close
	
	strText = Replace(strText, "CreateJobDetail", "create_job_detail")
	strText = Replace(strText, "ReplicaJobDetail", "replica_job_detail")
	strText = Replace(strText, "LoaderJobDetail", "loader_job_detail")
	strText = Replace(strText, "LauncherJobDetail", "launcher_job_detail")
	strText = Replace(strText, "CreatePackerJobDetail", "create_packer_job_detail")
	strText = Replace(strText, "TransportMessage", "transport_message")

	Set objWrite = objFSO.OpenTextFile( objFile.Path, ForWriting)
	objWrite.WriteLine strText
	objWrite.Close
Next

