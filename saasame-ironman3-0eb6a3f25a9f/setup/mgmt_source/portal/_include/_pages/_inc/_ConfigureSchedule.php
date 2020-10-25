<?php
	if (isset($_SESSION['EDIT_REPLICA_UUID'])){$REPLICA_UUID = $_SESSION['EDIT_REPLICA_UUID'];}else{$REPLICA_UUID = null;}
	if (isset($_SESSION['LAUN_UUID'])){$LAUNCHER_UUID = $_SESSION['LAUN_UUID'];}else{$LAUNCHER_UUID = null;}
	if (isset($_SESSION['LOAD_UUID'])){$LOADER_UUID = $_SESSION['LOAD_UUID'];}else{$LOADER_UUID = null;}
	if (isset($_SESSION['SERV_TYPE'])){$SERVER_TYPE = $_SESSION['SERV_TYPE'];}else{$SERVER_TYPE = null;}
	if (isset($_SESSION['HOST_TYPE'])){$HOST_TYPE = $_SESSION['HOST_TYPE'];}else{$HOST_TYPE = null;}
	if (isset($_SESSION['HOST_OS'])){$HOST_OS = $_SESSION['HOST_OS'];}else{$HOST_OS = null;}
	
	if (isset($_SESSION['HOST_UUID'])){$HOST_UUID = $_SESSION['HOST_UUID'];}else{$HOST_UUID = null;}
	if (isset($_SESSION['PRIORITY_ADDR'])){$PRIORITY_ADDR = $_SESSION['PRIORITY_ADDR'];}else{$PRIORITY_ADDR = null;}
	if (isset($_SESSION['CLOUD_TYPE'])){$CLOUD_TYPE = $_SESSION['CLOUD_TYPE'];}else{$CLOUD_TYPE = null;}
	
	if (isset($_SESSION['REPLICA_SETTINGS']['CloudMappingDisk'])){
		$DISK_MAP_COUNT = array_unique(explode(",",$_SESSION['REPLICA_SETTINGS']['CloudMappingDisk']));
		$EXPORT_JOB_READY = ((count($DISK_MAP_COUNT) == 1) AND in_array("CreateOnDemand",$DISK_MAP_COUNT))?true:false;
	}
	else{
		$EXPORT_JOB_READY = false;
	}
?>
<script>
$( document ).ready(function() {
	/* Define Number List */
	IntervalAndSnapshotSettings();
	QueryWorkerThreadNumber();
	QueryLoaderThreadNumber();
	QueryLoaderTriggerNumber();
	QueryBufferSize();
	QueryImageType();
	DetermineSegment('EditConfigureSchedule');
	
	/* Change Option Level */
	ChangeOptionLevel();	
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#BackToConfigureWorkload').prop('disabled', false);
		$('#BackToMgmtPrepareWorkload').prop('disabled', false);
		$('#PrepareWorkloadSummary').prop('disabled', false);
		$('#PrepareWorkloadSummary').removeClass('btn-default').addClass('btn-primary');
		ExportControl();
		$('.selectpicker').selectpicker('refresh');
	});
	
	/* Change Option Level */
	function ChangeOptionLevel(){
		option_level = '<?php echo $_SESSION['optionlevel']; ?>';
		if (option_level == 'SUPERUSER')
		{
			$('.super_hidden').show();
		}
		if (option_level == 'DEBUG')
		{
			$('.super_hidden').show();
			$('.debug_hidden').show();
		}
	}

	/* Export Control */
	function ExportControl(){
		service_type = '<?php echo $SERVER_TYPE; ?>';
		if (service_type == 'LocalRecoverKit')
		{
			$("#INPUT_EXPORT_PATH").prop("disabled", true);
		}
	}
	
	/* Determine URL Segment */
	function DetermineSegment(SET_SEGMENT){
		document.cookie = "input_path=;expires=Thu, 01 Jan 1970 00:00:01 GMT;"; //Reset input path cookie
		URL_SEGMENT = window.location.pathname.split('/').pop();
				
		if (URL_SEGMENT == SET_SEGMENT)
		{
			repl_uuid = '<?php echo $REPLICA_UUID; ?>';
			QueryReplicaConfiguration(repl_uuid,false); 
		}
		else
		{
			priority_addr = '<?php echo $PRIORITY_ADDR; ?>';
			cloud_type = '<?php echo $CLOUD_TYPE; ?>';
			host_uuid = '<?php echo $HOST_UUID; ?>';
			export_job_ready = '<?php echo $EXPORT_JOB_READY; ?>';
			$("#EXPORT_IMAGE_TYPE").prop("disabled", !export_job_ready);
			
			init_cascaded = true;
			MultiLayerTransport();
			$("#MULTI_LAYER_PROTECTION").change(MultiLayerTransport);
		}
	}

	/* Query Select Host */
	function QueryHostInformation(HOST_UUID,IS_INIT,IS_CASCADED){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryHostInformation',
				 'HOST_UUID' :HOST_UUID			 
			},
			success:function(jso)
			{
				if (jso.HOST_TYPE == 'Physical')
				{
					$("#DISPLAY_PACKER_DATA_COMPRESSION").show();
					if (jso.IS_DIRECT == true)
					{
						$("#DISPLAY_PACKER_ENCRYPTION").show();					
					}
				}
				
				if (IS_INIT == true)
				{
					if (jso.HOST_TYPE == 'Virtual')
					{
						CARR_UUID = jso.HOST_SERV.OPEN_HOST.split("|")[0];
					}
					else
					{
						CARR_UUID = jso.HOST_SERV.SERV_UUID;
					}
					QueryConnectionInformation();
				}
				
				/* Excluded Paths */
				if (jso.HOST_TYPE != 'Physical')
				{
					$("#DISPLAY_EXCLUDED_PATHS").hide();
				}
				
				/* Partition Control */
				if (jso.HOST_TYPE == 'Virtual')
				{
					$("#CREATE_BY_PARTITION").bootstrapToggle('disable');
					$("#DISPLAY_CREATE_BY_PARTITION").hide();
				}
				
				/* CDR Control */
				if (jso.HOST_TYPE == 'Virtual' || jso.OS_TYPE != 'WINDOWS')
				{
					$("#ENABLE_CDR").bootstrapToggle('disable');
					$("#DISPLAY_ENABLE_CDR").hide();
				}

				/* Cascade Display Control */
				if (IS_CASCADED == true)
				{
					$("#DISPLAY_PACKER_DATA_COMPRESSION").css("display", "none");
					$("#DISPLAY_PACKER_ENCRYPTION").css("display", "none");
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	var ESX_INFO = null;	
	function getVMWareStorage(){
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'    :'getVMWareStorage',
				 'LAUN_UUID' :'<?php echo $LAUNCHER_UUID; ?>'	
			},
			success:function( response )
			{
				$('#VMWARE_ESX').empty();
				$('#VMWARE_STORAGE').empty();
				$('#VMWARE_FOLDER').empty();
				
				ESX_INFO = response.EsxInfo;
						
				for( esx in response.EsxInfo){
					
					$('#VMWARE_ESX').append(new Option( response.EsxInfo[esx].name, response.EsxInfo[esx].name, true, false));
					
				}
				
				for( key in response.EsxInfo[0].datastores ){
					$('#VMWARE_STORAGE').append(new Option(response.EsxInfo[0].datastores[key], response.EsxInfo[0].datastores[key], true, false));
				}
				
				for( key in response.EsxInfo[0].folder_path ){
					$('#VMWARE_FOLDER').append(new Option(response.EsxInfo[0].folder_path[key], response.EsxInfo[0].folder_path[key], true, false));
				}
			},
			error: function(xhr)
			{
			}
		});
	}
	
	$("#VMWARE_ESX").change(
		function(){
			
			var select_esx = $("#VMWARE_ESX").val();

			$('#VMWARE_STORAGE').empty();
			$('#VMWARE_FOLDER').empty();
			
			$.each(ESX_INFO, function(key,value)
			{
				if( select_esx == value.name )
				{
					for( key in value.datastores){
						$('#VMWARE_STORAGE').append(new Option(value.datastores[key], value.datastores[key], true, false));
					}
					
					for( key in value.folder_path){
						$('#VMWARE_FOLDER').append(new Option(value.folder_path[key], value.folder_path[key], true, false));
					}
				}
			});
			
			$('.selectpicker').selectpicker('refresh');
		});	
	
	/* Query Connection	*/
	function QueryConnectionInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryConnectionInformationByServUUID',
				 'HOST_UUID' :CARR_UUID,
				 'LOAD_UUID' :'<?php echo $LOADER_UUID; ?>',
				 'LAUN_UUID' :'<?php echo $LAUNCHER_UUID; ?>'	
			},
			success:function(jso)
			{
				$("#WEBDAV_PRIORITY_ADDR option").remove();
				
				if (jso.CONN_DATA.CONN_TYPE == 'LocalFolder')
				{
					$('#WEBDAV_PRIORITY_ADDR').append(new Option('<?php echo _('LocalFolder'); ?>', 'LocalFolder', true, false));
				}
				else
				{
					if (jso.CARR_INFO.SERV_INFO.direct_mode == true)
					{
						$.each(jso.CARR_INFO.SERV_ADDR, function(source_key,source_value)
						{
							$('#WEBDAV_PRIORITY_ADDR').append(new Option(source_value, source_value, true, false));
						});						
					}
					else
					{
						$.each(jso.SERV_INFO.SERV_ADDR, function(source_key,source_value)
						{
							$('#WEBDAV_PRIORITY_ADDR').append(new Option(source_value, source_value, true, false));
						});
					}
					$("#WEBDAV_PRIORITY_ADDR").prop("disabled", false);					
				}				
				$('.selectpicker').selectpicker('refresh');
				
				/* SHOW AZURE BLOB MODE SWITCH*/
				if(jso.VENDOR_NAME == 'AzureBlob')
				{
					$("#DISPLAY_AZURE_BLOB_MODE").show();
					
					if (jso.SERV_INFO.SERV_INFO.is_azure_mgmt_disk == false)
					{
						$('#AZURE_BLOB_MODE').prop('checked', true).change();
						$("#AZURE_BLOB_MODE").bootstrapToggle('disable');
					}

					if (jso.SERV_INFO.SERV_INFO.is_promote == true)
					{
						$('#AZURE_BLOB_MODE').prop('checked', true).change();
						$("#AZURE_BLOB_MODE").bootstrapToggle('disable');
					}
				}

				if(jso.CLOUD_TYPE == 'VMWare'){
					
					getVMWareStorage();
					
					$("#VMWARE_STORAGE_MODE").show();
					$("#VMWARE_ESX_MODE").show();
					$("#VMWARE_FOLDER_MODE").show();
					$("#VMWARE_THIN_PROVISIONED_MODE").show();
					
					$('#VMWARE_THIN_PROVISIONED').prop('checked', true).change();
				}
				
				$('#PrepareWorkloadSummary').prop('disabled', false);
				$('#PrepareWorkloadSummary').removeClass('btn-default').addClass('btn-primary');
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Interval And Snapshot Settings */
	function IntervalAndSnapshotSettings(){
		$('#DISABLE_INTERVAL').change(function(){
			if ($('#DISABLE_INTERVAL').is(':checked') == true)
			{
				$('#INPUT_INTERVAL').val('Disabled').prop('disabled', true);				
			} 
			else 
			{
				$('#INPUT_INTERVAL').val('60').prop('disabled', false);				
			}
		});		
		
		$('#DISABLE_SNAPSHOT').change(function(){
			if ($('#DISABLE_SNAPSHOT').is(':checked') == true)
			{
				$('#INPUT_ROTATION').val('Disabled').prop('disabled', true);				
			} 
			else 
			{
				$('#INPUT_ROTATION').val('3').prop('disabled', false);				
			}
		});
	}
	
	/* Query Worker Thread Number */
	function QueryWorkerThreadNumber(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'WorkerThreadNumber'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#WORK_THREAD').append(new Option(value,key, true, false));
					});
					$('select[id=WORK_THREAD]').val(4);
					$('.selectpicker').selectpicker('refresh');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Loader Thread Number */
	function QueryLoaderThreadNumber(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'LoaderThreadNumber'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#LOADER_THREAD').append(new Option(value,key, true, false));
					});
					$('select[id=LOADER_THREAD]').val(1);
					$('.selectpicker').selectpicker('refresh');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Loader Thread Number */
	function QueryLoaderTriggerNumber(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'LoaderTriggerPercentage'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#LOADER_TRIGGER').append(new Option(value,key, true, false));
					});
					$('select[id=LOADER_TRIGGER]').val(0);
					$('.selectpicker').selectpicker('refresh');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query BufferSize GB */
	function QueryBufferSize(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'QueryBufferSize'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#BUFFER_SIZE').append(new Option(value,key, true, false));
					});
					$('select[id=BUFFER_SIZE]').val(0);
					$('.selectpicker').selectpicker('refresh');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Image Type */
	function QueryImageType(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'QueryImageType'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#EXPORT_IMAGE_TYPE').append(new Option(value,key, true, false));
					});
					$('select[id=EXPORT_IMAGE_TYPE]').val(-1);
					$('.selectpicker').selectpicker('refresh');
					//$("#EXPORT_IMAGE_TYPE").prop("disabled", true);
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Replica Snapshot Script */
	function QueryReplicaSnapshotScript(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'QueryReplicaSnapshotScript',
				 'REPL_UUID' :'<?php echo $REPLICA_UUID; ?>'
			},
			success:function(snapshot_script)
			{
				$('#QuerySnaphostScript').prop('disabled', true);
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _('Replication Snapshot Script'); ?>',
						cssClass: 'snapshot-script-dialog',
						message: snapshot_script,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: true,
						onhidden: function(dialogRef){
							$('#QuerySnaphostScript').prop('disabled', false);
						},
						buttons:
						[
							{
								label: '<?php echo _('Save'); ?>',
								cssClass: 'btn-primary',
								action: function(dialogRef)
								{
									Pre_Snapshot_Script  = $('#PreSnapshotScript').val();
									Post_Snapshot_Script = $('#PostSnapshotScript').val();
									
									dialogRef.close();
								}
							}
						]				
					});
				}, 0);
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Replica Excluded Paths */
	var input_path = null;
	function QueryReplicaExcludedPaths(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'QueryReplicaExcludedPaths',
				 'REPL_UUID' :'<?php echo $REPLICA_UUID; ?>'
			},
			success:function(excluded_paths_input)
			{
				$('#QueryExcludedPaths').prop('disabled', true);
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _('Replication Exclude Paths'); ?>',
						cssClass: 'excluded-paths-dialog',
						message: excluded_paths_input,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: true,
						onhidden: function(dialogRef){
							$('#QueryExcludedPaths').prop('disabled', false);
						},
						buttons:
						[
							{
								label: '<?php echo _('Save'); ?>',
								cssClass: 'btn-primary',
								action: function(dialogRef)
								{
									input_path = $('#ExcludedPaths').val().split("\n").join();
									document.cookie = "input_path="+input_path+"";									
									dialogRef.close();
								}
							}
						]				
					});
				}, 0);
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Multi-layer Trnasport */
	function MultiLayerTransport(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'GetCascadedReplicaInfo',
				 'PACKER_UUID':host_uuid
			},
			success:function(jso)
			{
				$('#PrepareWorkloadSummary').prop('disabled', true);
				$('#PrepareWorkloadSummary').removeClass('btn-primary').addClass('btn-default');
				
				if (jso.count != 0)
				{
					if (init_cascaded == true)
					{
						$("#MULTI_LAYER_PROTECTION").prop('disabled',false);
						$('.selectpicker').selectpicker('refresh');
					}
					
					if ($("#MULTI_LAYER_PROTECTION").val() == 'cascaded')
					{
						Last_layer_transport = Object.values(jso.transport)[0].target_machine_id;
						GetTransportInfoByMachineId(Last_layer_transport);
					}
						
					if ($("#MULTI_LAYER_PROTECTION").val() == 'parallel')
					{
						QueryHostInformation(host_uuid,true,true);
					}

					QueryReplicaConfiguration(jso.replica,true);
					HiddenDisplayOptionForCascade();					
					BackToLastPage = "SelectTargetTransportServer";
				}
				else
				{
					cascaded_carrier = false;
					QueryHostInformation(host_uuid,true,false);
					$("#DISPLAY_MULTI_LAYER_PROTECTION").css("display", "none");
					$('#MULTI_LAYER_PROTECTION').append(new Option("No", "NoSeries", true, true));
					$("#MULTI_LAYER_PROTECTION").val("NoSeries");
					BackToLastPage = "ConfigureWorkload";
				}
			},
			error: function(xhr)
			{

			}
		});		
	}
	
	/* Hidden Display Option For Cascade */
	function HiddenDisplayOptionForCascade()
	{
		/* Default */
		$("#DISPLAY_INPUT_INTERVAL").css("display", "none");
		$("#DISPLAY_INPUT_ROTATION").css("display", "none");
		$("#DISPLAY_WORK_THREAD").css("display", "none");
		$("#DISPLAY_REPL_START_TIME").css("display", "none");
		$("#DISPLAY_PACKER_DATA_COMPRESSION").css("display", "none");
		$("#DISPLAY_SNAPSHOT_SCRIPT").css("display", "none");
		
		/* Advanced */
		$("#DISPLAY_CREATE_BY_PARTITION").css("display", "none");
		$("#DISPLAY_ENABLE_CDR").css("display", "none");
		$("#DISPLAY_RETRY_REPLICATION").css("display", "none");
		$("#DISPLAY_PACKER_ENCRYPTION").css("display", "none");
		$("#DISPLAY_EXCLUDED_PATHS").css("display", "none");
		
		/* Debug */
		//$("#DISPLAY_SCHEDULE_PAUSE").css("display", "none");
		$("#DISPLAY_FILE_SYSTEM_FILTER").css("display", "none");
	}
	
	/* Get Transport Info By MachineId */
	function GetTransportInfoByMachineId(MACHINE_ID){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'GetTransportInfoByMachineId',
				 'MACHINE_ID':MACHINE_ID
			},
			success:function(jso)
			{
				cascaded_carrier = jso.ServiceId.Carrier;
				CARR_UUID = jso.ServiceId.Carrier;
				QueryConnectionInformation();
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Replica Configuration */
	function QueryReplicaConfiguration(REPL_UUID,INIT_CASCADED){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'      :'QueryReplicaConfiguration',
				 'REPL_UUID'   :REPL_UUID
			},
			success:function(jso)
			{
				if (INIT_CASCADED == false)
				{
					/* QUERY HOST INFORMATION */
					QueryHostInformation(jso.host_uuid,false,false);
			
					/* QUERY CLOUD TYPE*/
					cloud_type = jso.cloud_type;
					
					/* DISPLAY AZURE BLOB SWITCH */
					if( jso.is_azure_blob_mode == true ){
						$("#DISPLAY_AZURE_BLOB_MODE").show();		
					}
					
					<!-- Default -->
					/* QUERY AND SET INTERVAL */
					if (jso.triggers[0].interval == 2147483647)
					{
						trigger_interval = 0;
					}
					else
					{
						trigger_interval = jso.triggers[0].interval;
					}
					
					if (trigger_interval == 0)
					{
						$("#DISABLE_INTERVAL").prop("checked", true);
						$("#INPUT_INTERVAL").val("Disabled");
						$("#INPUT_INTERVAL").prop("disabled", true);	
					}
					else
					{				
						$("#INPUT_INTERVAL").val(trigger_interval);
						$("#INPUT_INTERVAL").prop("disabled", jso.edit_lock);
					}
					
					/* QUERY AND SET SNAPSHOT ROTATION NUMBER */
					if (jso.snapshot_rotation == -1)
					{
						$("#DISABLE_SNAPSHOT").prop("checked", true);
						$("#INPUT_ROTATION").val("Disabled");
						$("#INPUT_ROTATION").prop("disabled", true);					
					}
					else
					{
						$("#INPUT_ROTATION").val(jso.snapshot_rotation);
						$("#INPUT_ROTATION").prop("disabled", jso.edit_lock);
					}
		
					/* QUERY AND SET WORK THREAD NUMBER */
					$("#WORK_THREAD").val(jso.worker_thread_number);
					$("#WORK_THREAD").prop("disabled", jso.edit_lock);	
		
					/* QUERY AND SET START TIME */
					$("#START_TIME").val(jso.user_set_time);
					$("#REPL_START_TIME").val(jso.triggers[0].start);				
					
					/* QUERY AND SET LOADER THREAD NUMBER */
					$("#LOADER_THREAD").val(jso.loader_thread_number);
					$("#LOADER_THREAD").prop("disabled", jso.edit_lock);
					
					/* QUERY AND SET LOADER TRIGGER NUMBER */
					$("#LOADER_TRIGGER").val(jso.loader_trigger_percentage);
					$("#LOADER_TRIGGER").prop("disabled", true);
					
					/* QUERY AND SET WEBDAV PRIORITY ADDR */
					$('#WEBDAV_PRIORITY_ADDR').append(new Option(jso.webdav_priority_addr,jso.webdav_priority_addr, true, false));
					$("#WEBDAV_PRIORITY_ADDR").val(jso.webdav_priority_addr);
					$("#WEBDAV_PRIORITY_ADDR").prop("disabled", true);
					
					/* AZURE BLOB MODE */
					$("#AZURE_BLOB_MODE").prop('checked', jso.is_azure_blob_mode).change();
					$("#AZURE_BLOB_MODE").prop("disabled", true);
					
					/* PACKER DATA COMPRESSION */
					$("#PACKER_DATA_COMPRESSION").prop('checked', jso.is_packer_data_compressed).change();
					
					/* SET SNAPSHOT SCRIPT */
					if (jso.priority_addr != '127.0.0.1')
					{
						$('#DISPLAY_SNAPSHOT_SCRIPT').show();		
						$('#QuerySnaphostScript').removeClass('btn-default').addClass('btn-primary');
						$('#QuerySnaphostScript').prop('disabled', false);
					}				
					Pre_Snapshot_Script = jso.pre_snapshot_script;
					Post_Snapshot_Script = jso.post_snapshot_script;
					<!-- Default -->
					
					<!-- Advanced -->
					/* QUERY EXPORT PATH */
					$("#INPUT_EXPORT_PATH").val(jso.export_path);
					$("#INPUT_EXPORT_PATH").prop("disabled", true);
				
					/* QUERY EXPORT IMAGE TYPE */
					$("#EXPORT_IMAGE_TYPE").val(jso.export_type);
					$("#EXPORT_IMAGE_TYPE").prop("disabled", true);
					
					/* SET DISK CUSTOMIZED ID*/
					$('#SET_DISK_CUSTOMIZED_ID').prop('checked', jso.set_disk_customized_id).change();
					$("#SET_DISK_CUSTOMIZED_ID").bootstrapToggle('disable');
					
					/* CREATE DISK BY PARTITION */
					$('#CREATE_BY_PARTITION').prop('checked', jso.create_by_partition).change();
					$("#CREATE_BY_PARTITION").bootstrapToggle('disable');

					/* CONTINUOUS DATA REPLICATION */
					$('#ENABLE_CDR').prop('checked', jso.is_continuous_data_replication).change();
					
					/* RETRY REPLICATION */
					$('#RETRY_REPLICATION').prop('checked', jso.always_retry).change();
					
					/* RETRY REPLICATION */
					$('#PACKER_ENCRYPTION').prop('checked', jso.is_encrypted).change();
					
					/* QUERY EXCLUDED PATHS */
					document.cookie = "input_path="+jso.excluded_paths;
					
					/* VMWARE CLOUD */
					if(cloud_type == 'VMWare'){
						
						$("#VMWARE_STORAGE_MODE").show();
						$("#VMWARE_ESX_MODE").show();
						$("#VMWARE_FOLDER_MODE").show();
						$("#VMWARE_THIN_PROVISIONED_MODE").show();
						
						$('#VMWARE_STORAGE').append(new Option(jso.VMWARE_STORAGE, jso.VMWARE_STORAGE, true, false));
						$("#VMWARE_STORAGE").prop("disabled", true);
						
						$('#VMWARE_ESX').append(new Option(jso.VMWARE_ESX, jso.VMWARE_ESX, true, false));
						$("#VMWARE_ESX").prop("disabled", true);
						
						$('#VMWARE_FOLDER').append(new Option(jso.VMWARE_FOLDER, jso.VMWARE_FOLDER, true, false));
						$("#VMWARE_FOLDER").prop("disabled", true);
						
						$('#VMWARE_THIN_PROVISIONED').prop('checked', jso.VMWARE_THIN_PROVISIONED).change();
						$("#VMWARE_THIN_PROVISIONED").bootstrapToggle('disable');
					}			
					
					/* POST LOADER SCRIPT */
					$('#POST_LOADER_SCRIPT').val(jso.post_loader_script);
					<!-- Advanced -->
					
					<!-- Debug -->
					/* QUERY AND SET CHECKSUM VERIFY */
					$('#SCHEDULE_PAUSE').prop('checked', jso.schedule_pause).change();
					$("#SCHEDULE_PAUSE").prop("disabled", jso.edit_lock);
							
					/* FILE SYSTEN FILTER */
					$('#FILE_SYSTEM_FILTER').prop('checked', jso.file_system_filter).change();				
					$("#FILE_SYSTEM_FILTER").bootstrapToggle('disable');
							
					/* QUERY AND SET EXTRA GB */
					$('#PLUS_EXTRA_GB').prop('checked', jso.extra_gb).change();
					$("#PLUS_EXTRA_GB").bootstrapToggle('disable');
					
					/* DISPLAY MULTI-LAYER PROTECTION */
					cascaded_carrier = jso.cascaded_carrier;
					if(jso.multi_layer_protection == 'NoSeries')
					{
						$("#DISPLAY_MULTI_LAYER_PROTECTION").css("display", "none");						
					}
					else
					{
						$("#MULTI_LAYER_PROTECTION").val(jso.multi_layer_protection);
						host_uuid = jso.host_uuid;
						init_cascaded = false;						
						MultiLayerTransport();
					}
					<!-- Debug -->
							
					<!-- Hidden -->			
					/* QUERY AND SET REPLICA BUFFER SIZE */
					$("#BUFFER_SIZE").val(jso.buffer_size);
					$("#BUFFER_SIZE").prop("disabled", true);
					
					
					/* QUERY AND SET CHECKSUM VERIFY */
					$('#CHECKSUM_VERIFY').prop('checked', jso.ChecksumToggle).change();
					//$("#CHECKSUM_VERIFY").prop("disabled", true);
					$("#CHECKSUM_VERIFY").bootstrapToggle('disable');
					
					/* QUERY AND SET BLOCK MODE */
					$('#BLOCK_MODE').prop('checked', !jso.block_mode_enable).change();
					$("#BLOCK_MODE").prop("disabled", true);
					//$("#BLOCK_MODE").prop("disabled", jso.edit_lock);				
					
					/* QUERY AND SET CHECKSUM VERIFY */
					$('#IS_COMPRESSED').prop('checked', jso.is_compressed).change();
					$("#IS_COMPRESSED").prop("disabled", jso.edit_lock);

					/* QUERY AND SET CHECKSUM VERIFY */	
					$('#IS_CHECKSUM').prop('checked', jso.is_checksum).change();
					$("#IS_CHECKSUM").prop("disabled", jso.edit_lock);
									
					/* QUERY CARRIER PRIORITY ADDRESS */
					priority_addr = jso.priority_addr;
					<!-- Hidden -->	
					
					/* REFRESH SELECT */
					$('.selectpicker').selectpicker('refresh');
					
					/* FORMAT BUTTON */
					$("#BackToConfigureWorkload").remove();				
					$("#PrepareWorkloadSummary").text("<?php echo _('Update'); ?>");
				}
				else
				{
					/* CASCADE JOB USE */
					skip_disk = jso.skip_disk;
					cloud_mapping_disk = jso.cloud_mapping_disk;
					pre_snapshot_script = jso.pre_snapshot_script;
					post_loader_script = jso.post_snapshot_script;
					
					/* QUERY AND SET CHECKSUM VERIFY */
					$('#SCHEDULE_PAUSE').prop('checked', jso.schedule_pause).change();
					$("#SCHEDULE_PAUSE").prop("disabled", (jso.schedule_pause == true)?false:true);
					
					if (jso.source_transport_uuid == jso.target_transport_uuid)
					{
						$("#MULTI_LAYER_PROTECTION option[value='parallel']").remove();
					}
				}
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Replica Configuration */
	function ReplicaConfiguration(){

		/* Exclude Path */
		var excluded_paths = input_path == null ? null : input_path.replace(/[\\"']/g, '\\$&');
		
		/* DOM REPLICA SETTINGS */
		var REPLICA_SETTINGS = {
								'IntervalMinutes':($("#INPUT_INTERVAL").val() === 'Disabled') ? 0 : $("#INPUT_INTERVAL").val(),
								'SnapshotsNumber':($("#INPUT_ROTATION").val() === 'Disabled') ? -1 : $("#INPUT_ROTATION").val(),
								'BufferSize':$("#BUFFER_SIZE").val(),
								'StartTime':$("#REPL_START_TIME").val(),
								'WorkerThreadNumber':$("#WORK_THREAD").val(),
								'LoaderThreadNumber':$("#LOADER_THREAD").val(),
								'LoaderTriggerPercentage':$("#LOADER_TRIGGER").val(),								
								'ExportPath':$("#INPUT_EXPORT_PATH").val(),
								'ExportType':$("#EXPORT_IMAGE_TYPE").val(),
								'SetDiskCustomizedId':$("#SET_DISK_CUSTOMIZED_ID:checkbox:checked").val(),
								'CreateByPartition':$("#CREATE_BY_PARTITION:checkbox:checked").val(),
								'EnableCDR':$("#ENABLE_CDR:checkbox:checked").val(),
								'ChecksumVerify':$("#CHECKSUM_VERIFY:checkbox:checked").val(),
								'UseBlockMode':$("#BLOCK_MODE:checkbox:checked").val(),
								'SchedulePause':$("#SCHEDULE_PAUSE:checkbox:checked").val(),
								'DataCompressed':$("#IS_COMPRESSED:checkbox:checked").val(),
								'DataChecksum':$("#IS_CHECKSUM:checkbox:checked").val(),
								'FileSystemFilter':$("#FILE_SYSTEM_FILTER:checkbox:checked").val(),
								'ExtraGB':$("#PLUS_EXTRA_GB:checkbox:checked").val(),
								'WebDavPriorityAddr':$("#WEBDAV_PRIORITY_ADDR").val(),
								'PriorityAddr':priority_addr,
								'ReplicationRetry':$("#RETRY_REPLICATION:checkbox:checked").val(),
								'PackerEncryption':$("#PACKER_ENCRYPTION:checkbox:checked").val(),
								'IsAzureBlobMode':$("#AZURE_BLOB_MODE:checkbox:checked").val(),
								'IsPackerDataCompressed':$("#PACKER_DATA_COMPRESSION:checkbox:checked").val(),
								'TimeZone':'<?php echo $_SESSION['timezone']; ?>',
								'ExcludedPaths':excluded_paths,
								'PostLoaderScript':$("#POST_LOADER_SCRIPT").val(),
								'CloudType':cloud_type,
								'MultiLayerProtection':$("#MULTI_LAYER_PROTECTION").val(),
								'CascadedCarrier':cascaded_carrier
						};
						
		if( cloud_type == "VMWare")	{
			REPLICA_SETTINGS["VMWARE_STORAGE"] = $('#VMWARE_STORAGE').val();
			REPLICA_SETTINGS["VMWARE_ESX"] = $('#VMWARE_ESX').val();
			REPLICA_SETTINGS["VMWARE_FOLDER"] = $('#VMWARE_FOLDER').val();
			REPLICA_SETTINGS["VMWARE_THIN_PROVISIONED"] = $('#VMWARE_THIN_PROVISIONED').is(":checked");
		}
			
		if (typeof skip_disk !== 'undefined') {
			REPLICA_SETTINGS['SkipDisk'] = skip_disk;
			REPLICA_SETTINGS['CloudMappingDisk'] = cloud_mapping_disk;
			REPLICA_SETTINGS['PreSnapshotScript'] = pre_snapshot_script;
			REPLICA_SETTINGS['PostSnapshotScript'] = post_loader_script;
		}
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 		    :'ReplicaConfiguration',
				 'SERVICE_TYPE'			:service_type,
				 'SERVICE_LAUNCHER'		:'<?php echo $LAUNCHER_UUID; ?>',
				 'REPLICA_SETTINGS'		:REPLICA_SETTINGS		 
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = "PrepareWorkloadSummary";
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _('Service Message'); ?>',
						message: jso.msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
	/* Update Replica Settings */
	function UpdateReplicaConfiguration(){
		
		/* Exclude Path */
		var excluded_paths = input_path == null ? null : input_path.replace(/[\\"']/g, '\\$&');
				
		/* DOM UPDATE REPLICA SETTINGS */
		var UPDATE_REPLICA_SETTINGS = {
										'IntervalMinutes':($("#INPUT_INTERVAL").val() === 'Disabled') ? 0 : $("#INPUT_INTERVAL").val(),
										'SnapshotsNumber':($("#INPUT_ROTATION").val() === 'Disabled') ? -1 : $("#INPUT_ROTATION").val(),
										'BufferSize':$("#BUFFER_SIZE").val(),
										'StartTime':$("#REPL_START_TIME").val(),
										'WorkerThreadNumber':$("#WORK_THREAD").val(),
										'LoaderThreadNumber':$("#LOADER_THREAD").val(),
										'LoaderTriggerPercentage':$("#LOADER_TRIGGER").val(),										
										'ExportPath':$("#INPUT_EXPORT_PATH").val(),
										'ExportType':$("#EXPORT_IMAGE_TYPE").val(),
										'SetDiskCustomizedId':$("#SET_DISK_CUSTOMIZED_ID:checkbox:checked").val(),
										'CreateByPartition':$("#CREATE_BY_PARTITION:checkbox:checked").val(),
										'EnableCDR':$("#ENABLE_CDR:checkbox:checked").val(),
										'ChecksumVerify':$("#CHECKSUM_VERIFY:checkbox:checked").val(),
										'UseBlockMode':$("#BLOCK_MODE:checkbox:checked").val(),
										'SchedulePause':$("#SCHEDULE_PAUSE:checkbox:checked").val(),
										'DataCompressed':$("#IS_COMPRESSED:checkbox:checked").val(),
										'DataChecksum':$("#IS_CHECKSUM:checkbox:checked").val(),
										'FileSystemFilter':$("#FILE_SYSTEM_FILTER:checkbox:checked").val(),
										'ExtraGB':$("#PLUS_EXTRA_GB:checkbox:checked").val(),
										'WebdavPriorityAddr':$("#WEBDAV_PRIORITY_ADDR").val(),
										'PreSnapshotScript':Pre_Snapshot_Script,
										'PostSnapshotScript':Post_Snapshot_Script,
										'PriorityAddr':priority_addr,
										'ReplicationRetry':$("#RETRY_REPLICATION:checkbox:checked").val(),
										'PackerEncryption':$("#PACKER_ENCRYPTION:checkbox:checked").val(),										
										'IsAzureBlobMode':$("#AZURE_BLOB_MODE:checkbox:checked").val(),
										'IsPackerDataCompressed':$("#PACKER_DATA_COMPRESSION:checkbox:checked").val(),
										'TimeZone':'<?php echo $_SESSION['timezone']; ?>',
										'ExcludedPaths':excluded_paths,
										'PostLoaderScript':$("#POST_LOADER_SCRIPT").val(),
										'CloudType':cloud_type,
										'MultiLayerProtection':$("#MULTI_LAYER_PROTECTION").val(),
										'CascadedCarrier':cascaded_carrier
									};
		
		if( cloud_type == "VMWare")	{
			UPDATE_REPLICA_SETTINGS["VMWARE_STORAGE"] = $('#VMWARE_STORAGE').val();
			UPDATE_REPLICA_SETTINGS["VMWARE_ESX"] = $('#VMWARE_ESX').val();
			UPDATE_REPLICA_SETTINGS["VMWARE_FOLDER"] = $('#VMWARE_FOLDER').val();
			UPDATE_REPLICA_SETTINGS["VMWARE_THIN_PROVISIONED"] = $('#VMWARE_THIN_PROVISIONED').is(":checked");
		}
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 		 	:'UpdateReplicaConfiguration',
				 'REPL_UUID'		 	:'<?php echo $REPLICA_UUID; ?>',
				 'REPLICA_SETTINGS'		:UPDATE_REPLICA_SETTINGS
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _('Service Message'); ?>',
						message: jso.msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtPrepareWorkload";
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _('Service Message'); ?>',
						message: jso.msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/*
		Submit Trigger
	*/
	$(function(){
		$("#BackToMgmtPrepareWorkload").click(function(){
			window.location.href = "MgmtPrepareWorkload";
		})
		
		$("#BackToConfigureWorkload").click(function(){
			window.location.href = BackToLastPage;
		})
		
		$("#QuerySnaphostScript").click(function(){
			QueryReplicaSnapshotScript();
		})
		
		$("#QueryExcludedPaths").click(function(){
			QueryReplicaExcludedPaths();
		})
		
		$("#PrepareWorkloadSummary").click(function(){
			if (URL_SEGMENT == 'ConfigureSchedule')
			{
				ReplicaConfiguration();
			}
			else
			{
				$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
				UpdateReplicaConfiguration();
			}
		})
	});
});
</script>

<style type="text/css">
	.form-control {
		width:100%;
		display: block;
	}
	
	.form-control-inline{
		width:185px;		
		display:inline;
	}
	.float_row{
		width:264px;
		float:left;
		margin-right:18px;		
	}
</style>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-files-o fa-fw"></i>&nbsp;<?php echo _("Replication"); ?>
				</div>										
			</div>		
	
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:20%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 2 - Select Target"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 3 - Configure Host"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 4 - Configure Replication"); ?></a></li>
					<li style='width:20%'><a>		 		<?php echo _("Step 5 - Replication Summary"); ?></a></li>
				</ul>
			</div>
	
			<div id='form_block_wizard'>
				<div class="panel-group">
					<div class="panel" style="border:0px; background-color:#F4F4F4;">
						<div class="panel-heading hue-blue" style="border-radius: 0px;"><i class="fa fa-anchor" aria-hidden="true"></i> <?php echo _("Default"); ?></div>
						<div class="panel-body">
						
							<div class="float_row" id="DISPLAY_INPUT_INTERVAL">
								<label for='comment'><i class="fa fa-clock-o" aria-hidden="true"></i> <?php echo _("Interval (Minutes)"); ?></label>
								<div class='form-group'>
									<input type='text' id='INPUT_INTERVAL' class='form-control form-control-inline' value='60' placeholder='Interval Minutes'>
									<label class="checkbox-inline"><input type="checkbox" id="DISABLE_INTERVAL" value=""><?php echo _("Disabled"); ?></label>	
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_INPUT_ROTATION">
								<label for='comment'><i class="fa fa-camera-retro" aria-hidden="true"></i> <?php echo _("Max Snapshot"); ?></label>
								<div class='form-group'>									
									<input type='text' id='INPUT_ROTATION' class='form-control form-control-inline' value='3' placeholder='Number Snapshot'>
									<label class="checkbox-inline"><input type="checkbox" id="DISABLE_SNAPSHOT" value=""><?php echo _("Disabled"); ?></label>									
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_WORK_THREAD">
								<label for='comment'><i class="fa fa-caret-square-o-left" aria-hidden="true"></i> <?php echo _("Packer Thread"); ?></label>
								<div class='form-group'>
									<select id="WORK_THREAD" class="selectpicker" data-width="264px"></select>	
								</div>
							</div>					
						
							<div class="float_row" id="DISPLAY_REPL_START_TIME" style="padding-bottom:15px;">
								<label for='comment'><i class="fa fa-calendar" aria-hidden="true"></i> <?php echo _("Start Time"); ?></label>								
								<div class="input-group date form_time" data-date="" data-date-format="yyyy-mm-dd hh:ii:00" data-link-field="REPL_START_TIME" data-link-format="yyyy-mm-dd hh:ii:00" style='width:264px;'>
									<input class="form-control jquery_time" id="START_TIME" size="16" value="" type="text" placeholder="<?php echo _('Run Now'); ?>" readonly>
									<span class="input-group-addon"><span class="glyphicon glyphicon-remove fa fa-times"></span></span>
									<span class="input-group-addon"><span class="glyphicon glyphicon-time fa fa-calendar-check-o"></span></span>					
								</div>
								<input type="hidden" class="form-control" id="REPL_START_TIME" value="" />							
							</div>
								
							<div class="float_row">
								<label for='comment'><i class="fa fa-tasks" aria-hidden="true"></i> <?php echo _("Replication Thread"); ?></label>
								<div class='form-group'>
									<select id="LOADER_THREAD" class="selectpicker" data-width="264px"></select>	
								</div>						
							</div>
							
							<div class="float_row">
								<label for='comment'><i class="fa fa-percent" aria-hidden="true"></i> <?php echo _("Replication Trigger %"); ?></label>
								<div class='form-group'>				
									<select id="LOADER_TRIGGER" class="selectpicker" data-width="264px"></select>
								</div>
							</div>
							
							<div class="float_row">
								<label for='comment'><i class="fa fa-exchange" aria-hidden="true"></i> <?php echo _("Preferred Connection Address"); ?></label>
								<div class='form-group'>				
									<select id="WEBDAV_PRIORITY_ADDR" class="selectpicker" data-width="264px" disabled></select>
								</div>
							</div>

							<div class="float_row" id="DISPLAY_PACKER_DATA_COMPRESSION" style="display:none;">
								<label for='comment'><i class="fa fa-file-archive-o" aria-hidden="true"></i> <?php echo _("Packer Compression"); ?></label>
								<div class='form-group'>
									<input id="PACKER_DATA_COMPRESSION" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_AZURE_BLOB_MODE" style="display:none;">
								<label for='comment'><i class="fa fa-hdd-o" aria-hidden="true"></i> <?php echo _("Create By Azure Blob Mode"); ?></label>
								<div class='form-group'>
									<input id="AZURE_BLOB_MODE" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" id="VMWARE_ESX_MODE" style="display:none;">
								<label for='comment'><i class="fa fa-hdd-o" aria-hidden="true"></i> <?php echo _("VMWare ESX"); ?></label>
								<div class='form-group'>
									<select id="VMWARE_ESX" class="selectpicker" data-width="264px"></select>	
								</div>
							</div>		
							
							<div class="float_row" id="VMWARE_STORAGE_MODE" style="display:none;">
								<label for='comment'><i class="fa fa-database" aria-hidden="true"></i> <?php echo _("ESX Datastores"); ?></label>
								<div class='form-group'>
									<select id="VMWARE_STORAGE" class="selectpicker" data-width="264px"></select>	
								</div>
							</div>
							
							<div class="float_row" id="VMWARE_FOLDER_MODE" style="display:none;">
								<label for='comment'><i class="fa fa-folder-o" aria-hidden="true"></i> <?php echo _("VM Folder"); ?></label>
								<div class='form-group'>
									<select id="VMWARE_FOLDER" class="selectpicker" data-width="264px"></select>	
								</div>
							</div>
							
							<div class="float_row" id="VMWARE_THIN_PROVISIONED_MODE" style="display:none;">
								<label for='comment'><i class="fa fa-circle-thin" aria-hidden="true"></i> <?php echo _("Thin Provisioning"); ?></label>
								<div class='form-group'>
									<input id="VMWARE_THIN_PROVISIONED" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" checked data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" style="display:none;" id="DISPLAY_SNAPSHOT_SCRIPT">
								<label for='comment'><i class="fa fa-pencil-square-o" aria-hidden="true"></i> <?php echo _("Edit Snapshot Script"); ?></label>
								<div class='form-group'>
									<button id='QuerySnaphostScript' class='btn btn-default pull-left btn-md' style="width:264px; text-align:left;" disabled><?php echo _("Pre/Post Snapshot Script"); ?></button>
								</div>
							</div>
							
						</div>
					</div>
					
					<div class="super_hidden" style="padding-top:15px;"></div>
					
					<div class="panel super_hidden" style="border:0px;">
						<div class="panel-heading hue-green" style="border-radius:0px;"><i class="fa fa-envira" aria-hidden="true"></i> <?php echo _("Advanced"); ?></div>
						<div class="panel-body" style="background-color:#F4F4F4;">
						
							<div class="float_row">
								<label for='comment'><i class="fa fa-file-image-o" aria-hidden="true"></i> <?php echo _("Image Export Type"); ?></label>
								<div class='form-group'>
									<select id="EXPORT_IMAGE_TYPE" class="selectpicker" data-width="264px" onChange="ImageExportOnChange()"></select>	
								</div>						
							</div>
							
							<div class="float_row">
								<label for='comment'><i class="fa fa-file-o" aria-hidden="true"></i> <?php echo _("Image Export Path"); ?></label>
								<div class='form-group'>				
									<input type='text' id='INPUT_EXPORT_PATH' class='form-control' value='' placeholder='<?php echo _('Export Path'); ?>' disabled>	
								</div>
							</div>					
							
							<div class="float_row">
								<label for='comment'><i class="fa fa-id-badge" aria-hidden="true"></i> <?php echo _("Set disk customized id"); ?></label>
								<div class='form-group'>				
									<input id="SET_DISK_CUSTOMIZED_ID" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
														
							<div class="float_row" id="DISPLAY_CREATE_BY_PARTITION">
								<label for='comment'><i class="fa fa-delicious" aria-hidden="true"></i> <?php echo _("Create by Partition Size"); ?></label>
								<div class='form-group'>
									<input id="CREATE_BY_PARTITION" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_EXCLUDED_PATHS" style="width:264px;">
								<label for='comment'><i class="fa fa-filter" aria-hidden="true"></i> <?php echo _("Exclude Paths"); ?></label>
								<div class='form-group'>
									<button id='QueryExcludedPaths' class='btn btn-primary btn-md' style="width:150px; text-align:left;"><?php echo _("Exclude Paths"); ?></button>
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_ENABLE_CDR">
								<label for='comment'><i class="fa fa-window-restore" aria-hidden="true"></i> <?php echo _("Continuous Data Replication"); ?></label>
								<div class='form-group'>
									<input id="ENABLE_CDR" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_RETRY_REPLICATION">
								<label for='comment'><i class="fa fa-retweet" aria-hidden="true"></i> <?php echo _("Retry Replication"); ?></label>
								<div class='form-group'>
									<input id="RETRY_REPLICATION" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" checked data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
														
							<div class="float_row" id="DISPLAY_PACKER_ENCRYPTION" style="display:none;">
								<label for='comment'><i class="fa fa-qrcode" aria-hidden="true"></i> <?php echo _("Packer Encryption"); ?></label>
								<div class='form-group'>
									<input id="PACKER_ENCRYPTION" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_POST_LOADER_SCRIPT">
								<label for='comment'><i class="fa fa-file-code-o" aria-hidden="true"></i> <?php echo _("Post Loader Script"); ?></label>
								<div class='form-group'>
									<textarea class='form-control' rows="2" cols="60" style="overflow:hidden; resize:none;" id="POST_LOADER_SCRIPT" placeholder="<?php echo _("Input Command Here"); ?>"></textarea>
								</div>
							</div>
							
						</div>
					</div>
					
					<div class="debug_hidden" style="padding-top:15px;"></div>
					
					<div class="panel debug_hidden" style="border: 0px;">
						<div class="panel-heading hue-orange" style="border-radius: 0px;"><i class="fa fa-bug" aria-hidden="true"></i> <?php echo _("Debug"); ?></div>
						<div class="panel-body" style="background-color:#F4F4F4;">
						
							<div class="float_row" id="DISPLAY_SCHEDULE_PAUSE">
								<label for='comment'><i class="fa fa-pause" aria-hidden="true"></i> <?php echo _("Schedule Pause"); ?></label>
								<div class='form-group'>
									<input id="SCHEDULE_PAUSE" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>	
							
							<div class="float_row" id="DISPLAY_FILE_SYSTEM_FILTER">
								<label for='comment'><i class="fa fa-filter" aria-hidden="true"></i> <?php echo _("File System Filter"); ?></label>
								<div class='form-group'>
									<input id="FILE_SYSTEM_FILTER" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" checked data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row">
								<label for='comment'><i class="fa fa-plus" aria-hidden="true"></i> <?php echo _("Plus extra GB"); ?></label>
								<div class='form-group'>
									<input id="PLUS_EXTRA_GB" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" checked data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
								</div>
							</div>
							
							<div class="float_row" id="DISPLAY_MULTI_LAYER_PROTECTION">
								<label for='comment'><i class="fa fa-connectdevelop" aria-hidden="true"></i> <?php echo _("Multi-layer protection"); ?></label>
								<div class='form-group'>
									<select id="MULTI_LAYER_PROTECTION" class="selectpicker" data-width="264px" disabled>
										<option value="cascaded"><?php echo _("Cascaded Replication"); ?></option>
										<option value="parallel"><?php echo _("Parallel Replicaion"); ?></option>										
									</select>
								</div>
							</div>							
						</div>
					</div>
				</div>
			</div>
			<!-- HIDDEN DISPLAY -->
			<div style="display:none">
				<div>
					<label for='comment'><?php echo _("Buffer Size"); ?></label>
					<div class='form-group'>
						<select id="BUFFER_SIZE" class="selectpicker" data-width="264px"></select>	
					</div>						
				</div>
			
				<div>
					<label for='comment'><?php echo _("Image Checksum"); ?></label>
					<div class='form-group'>
						<input id="CHECKSUM_VERIFY" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
					</div>
				</div>
			
				<div>
					<label for='comment'><?php echo _("Job Task Overlapping"); ?></label>
					<div class='form-group'>
						<input id="BLOCK_MODE" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
					</div>
				</div>
			
				<div>
					<label for='comment'><?php echo _("Data Compression"); ?></label>
					<div class='form-group'>
						<input id="IS_COMPRESSED" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" checked data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
					</div>
				</div>					
				
				<div>
					<label for='comment'><?php echo _("Data Checksum"); ?></label>
					<div class='form-group'>
						<input id="IS_CHECKSUM" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
					</div>
				</div>
			</div>			
			
			<!-- TIMEPICKER PARAMETER -->
			<script type="text/javascript">
				$('.form_time').datetimepicker({
					weekStart: 1,
					todayBtn:  1,
					autoclose: 1,
					todayHighlight: 1,
					startView: 1,
					minView: 0,
					maxView: 1,
					forceParse: 0,
					fontAwesome: 1,
					minuteStep: 15
				});					
			</script>            
			
			<div id='title_block_wizard'>			
				<div class='btn-toolbar'>
					<button id='BackToConfigureWorkload' 	class='btn btn-default pull-left  btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtPrepareWorkload' 	class='btn btn-default pull-left  btn-lg' disabled><?php echo _("Cancel"); ?></button>						
					<button id='PrepareWorkloadSummary' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>						
				</div>
			</div>
		</div> <!-- id: page -->
	</div> <!-- id: wrapper_block-->
</div>
<script>
	/* Image Export OnChange */
	function ImageExportOnChange(){
		if ($("#EXPORT_IMAGE_TYPE").val() == -1)
		{
			$("#INPUT_EXPORT_PATH").prop("disabled", true).val("");
		}
		else
		{
			$("#INPUT_EXPORT_PATH").prop("disabled", false);
		}		
	}
</script>
