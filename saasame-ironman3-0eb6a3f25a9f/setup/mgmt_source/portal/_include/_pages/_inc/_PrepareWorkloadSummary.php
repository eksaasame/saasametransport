<?php
	if ($_SESSION['REPLICA_SETTINGS']['PriorityAddr'] != '0.0.0.0'){$PRIORITY_ADDR = $_SESSION['REPLICA_SETTINGS']['PriorityAddr'];}else{$PRIORITY_ADDR = 'Not Specified';}

	$INTERVAL_SETTING = $_SESSION['REPLICA_SETTINGS']['IntervalMinutes'];
	
	if ($_SESSION['REPLICA_SETTINGS']['SnapshotsNumber'] == 0){$SNAPSHOT_NUMBER = 'Unlimited';}elseif($_SESSION['REPLICA_SETTINGS']['SnapshotsNumber'] == -1){$SNAPSHOT_NUMBER = 'Disabled';}else{$SNAPSHOT_NUMBER = $_SESSION['REPLICA_SETTINGS']['SnapshotsNumber'];}
	if ($_SESSION['REPLICA_SETTINGS']['BufferSize'] == 0){$BUFFER_SIZE = 'Unlimited';}else{$BUFFER_SIZE = $_SESSION['REPLICA_SETTINGS']['BufferSize'].'GB';}
	if ($_SESSION['REPLICA_SETTINGS']['StartTime'] == ''){$REPL_START_TIME = 'Run Now';}else{$REPL_START_TIME = $_SESSION['REPLICA_SETTINGS']['StartTime'];}
	if ($_SESSION['REPLICA_SETTINGS']['WorkerThreadNumber'] == 0){$WORK_NUMBER = 'Default';}else{$WORK_NUMBER = $_SESSION['REPLICA_SETTINGS']['WorkerThreadNumber'];}
	if ($_SESSION['REPLICA_SETTINGS']['LoaderThreadNumber'] == 0){$LOADER_NUMBER = 'Default';}else{$LOADER_NUMBER = $_SESSION['REPLICA_SETTINGS']['LoaderThreadNumber'];}
	if ($_SESSION['REPLICA_SETTINGS']['LoaderTriggerPercentage'] == 0){$LOADER_TRIGGER = 'Run at start';}else{$LOADER_TRIGGER = $_SESSION['REPLICA_SETTINGS']['LoaderTriggerPercentage'].'%';}
	if ($_SESSION['REPLICA_SETTINGS']['ExportPath'] != ''){$EXPORT_PATH = $_SESSION['REPLICA_SETTINGS']['ExportPath'];}else{$EXPORT_PATH = 'NA';}
	if ($EXPORT_PATH != 'NA'){if ($_SESSION['REPLICA_SETTINGS']['ExportType'] == 0){$EXPORT_TYPE = 'VHD';}else{$EXPORT_TYPE = 'VHDX';}}else{$EXPORT_TYPE = 'NA';}
	if (isset($_SESSION['REPLICA_SETTINGS']['SetDiskCustomizedId'])){$SET_DISK_CUSTOMIZED_ID = 'Enable';}else{$SET_DISK_CUSTOMIZED_ID = 'Disable';}	
	
	if (isset($_SESSION['REPLICA_SETTINGS']['CreateByPartition'])){$CREATE_BY_PARTITION = 'Enable';}else{$CREATE_BY_PARTITION = 'Disable';}	
	if (isset($_SESSION['REPLICA_SETTINGS']['ChecksumVerify'])){$CHECKSUM_VERIFY = 'Enable';}else{$CHECKSUM_VERIFY = 'Disable';}	
	if (isset($_SESSION['REPLICA_SETTINGS']['UseBlockMode'])){$BLOCK_MODE_ENABLE = 'Enable';}else{$BLOCK_MODE_ENABLE = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['SchedulePause'])){$SCHEDULE_PAUSE = 'Enable';}else{$SCHEDULE_PAUSE = 'Disable';}

	if (isset($_SESSION['REPLICA_SETTINGS']['DataCompressed'])){$DATA_COMPRESSED = 'Enable';}else{$DATA_COMPRESSED = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['DataChecksum'])){$DATA_CHECKSUM = 'Enable';}else{$DATA_CHECKSUM = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['FileSystemFilter'])){$FILE_SYSTEM_FILTER = 'Enable';}else{$FILE_SYSTEM_FILTER = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['ExtraGB'])){$EXTRA_GB = 'Enable';}else{$EXTRA_GB = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['EnableCDR'])){$ENABLE_CDR = 'Enable';}else{$ENABLE_CDR = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['IsAzureBlobMode'])){$BLOB_MODE = 'Enable';}else{$BLOB_MODE = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['IsPackerDataCompressed'])){$PACKER_DATA_COMPRESSED = 'Enable';}else{$PACKER_DATA_COMPRESSED = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['ReplicationRetry'])){$REPLICATION_RETRY = 'Enable';}else{$REPLICATION_RETRY = 'Disable';}
	if (isset($_SESSION['REPLICA_SETTINGS']['PackerEncryption'])){$PACKER_ENCRYPTION = 'Enable';}else{$PACKER_ENCRYPTION = 'Disable';}
	if ($_SESSION['REPLICA_SETTINGS']['ExcludedPaths'] != ''){$EXCLUDED_PATHS = $_SESSION['REPLICA_SETTINGS']['ExcludedPaths'];}else{$EXCLUDED_PATHS = 'NA';}
		
	if (isset($_SESSION['REPLICA_SETTINGS']['VMWARE_STORAGE'])){
		$VMWARE_ESX = $_SESSION['REPLICA_SETTINGS']['VMWARE_ESX'];
		$VMWARE_STORAGE = $_SESSION['REPLICA_SETTINGS']['VMWARE_STORAGE'];
		$VMWARE_FOLDER = $_SESSION['REPLICA_SETTINGS']['VMWARE_FOLDER'];
		$VMWARE_THIN_PROVISIONED = $_SESSION['REPLICA_SETTINGS']['VMWARE_THIN_PROVISIONED'];
	}
	else{
		$VMWARE_ESX = 'NA';
		$VMWARE_STORAGE = 'NA';
		$VMWARE_FOLDER = 'NA';
		$VMWARE_THIN_PROVISIONED = 'NA';
	}	
?>
<script>
$( document ).ready(function() {
	<!-- Exec -->
	QueryHostInformation();
		
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

	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#BackToReplicaInterval').prop('disabled', false);
		$('#BackToMgmtPrepareWorkload').prop('disabled', false);
		
		setTimeout(function(){			
			$('#ReplicaSubmitAndRun').prop('disabled', false);
			$('#ReplicaSubmitAndRun').removeClass('btn-default').addClass('btn-primary');
		},1000);
	});
	
	/* Query Select Host */
	function QueryHostInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryHostInformation',
				 'HOST_UUID' :'<?php echo $_SESSION['HOST_UUID']; ?>'				 
			},
			success:function(jso)
			{
				var SKIP_DISK = '<?php echo $_SESSION['REPLICA_SETTINGS']['SkipDisk']; ?>'.split(',');
				
				if (jso.HOST_TYPE == 'Virtual')
				{
					HOST_TYPE = 'Packer for VMware';
					
					$('#HostSummaryTable > tbody').append('\
						<tr><td width="210px"><?php echo _("Machine ID"); ?></td>	<td>'+jso.HOST_INFO.uuid+'</td></tr>\
						<tr><td width="210px"><?php echo _("Host Name"); ?></td>	<td>'+jso.HOST_NAME+'</td></tr>\
						<tr><td width="210px"><?php echo _("Host Type"); ?></td>	<td>'+HOST_TYPE+'</td></tr>\
					');
					i = 0;
					$.each(jso.HOST_DISK, function(DiskKey,DiskValue)
					{
						/* Default Strike Style */
						strike = '';
						$.each(SKIP_DISK, function(SkipKey,SkipValue)
						{
							if (DiskValue.DISK_UUID == SkipValue)
							{
								strike = 'strike_line';
							}
						});						
					
						$('#HostSummaryTable').append('<tr><td width="210px"><?php echo ('Disk'); ?> '+i+'</td><td class="'+strike+'">'+DiskValue.DISK_NAME+' ('+Math.ceil(DiskValue.DISK_SIZE/1024/1024/1024)+' GB)</td></tr>');
						i++;
					});
					
					QueryServerInformation(jso.HOST_SERV.SERV_UUID);
					CARR_UUID = jso.HOST_SERV.OPEN_HOST;					
				}
				else
				{
					$('#HostSummaryTable > tbody').append('\
						<tr><td width="210px"><?php echo _("Host Name"); ?></td>	<td>'+jso.HOST_NAME+'</td></tr>\
						<tr><td width="210px"><?php echo _("Host Address"); ?></td> <td>'+jso.HOST_ADDR+'</td></tr>\
						<tr><td width="210px"><?php echo _("Host Type"); ?></td>	<td>'+jso.HOST_TYPE+'</td></tr>\
					');
					i = 0;
					
					$.each(jso.HOST_DISK, function(DiskKey,DiskValue)
					{
						/* Default Strike Style */
						strike = '';
						$.each(SKIP_DISK, function(SkipKey,SkipValue)
						{
							if (DiskValue.DISK_UUID == SkipValue)
							{
								strike = 'strike_line';
							}
						});
						
						$('#HostSummaryTable').append('<tr><td width="210px"><?php echo _('Disk'); ?> '+i+'</td><td class="'+strike+'">'+DiskValue.DISK_NAME+' ('+Math.ceil(DiskValue.DISK_SIZE/1024/1024/1024)+' GB)</td></tr>');
						i++;					
					});
					
					$('#VirtualPackerTable').remove();
					
					CARR_UUID = jso.HOST_SERV.SERV_UUID;
				}
				
				/* OVERWRITEN CARRIER UUID FOR CASCADED REPLICATION */
				CARR_UUID = ('<?php echo $_SESSION['REPLICA_SETTINGS']['MultiLayerProtection']; ?>' == 'cascaded')?'<?php echo $_SESSION['REPLICA_SETTINGS']['CascadedCarrier']; ?>':CARR_UUID;
				
				/* TRIGGER TO DISPLAY TRANSPORT SERVER INFORMATION */
				QueryServerInformation(CARR_UUID);
				
				$('#IntervalSetting > tbody').append('<tr><td width="210px"><?php echo _("Max Snapshot"); ?></td>		 							 <td><?php echo $SNAPSHOT_NUMBER; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Buffer Size"); ?></td>				 <td><?php echo $BUFFER_SIZE; ?></td></tr>\
													  <tr><td width="210px"><?php echo _("Start Time"); ?></td>	 							 		 <td><?php echo $REPL_START_TIME.' | ('.$INTERVAL_SETTING.')'; ?></td></tr>\
													  <tr><td width="210px"><?php echo _("Packer Thread"); ?></td>	 							 	 <td><?php echo $WORK_NUMBER; ?></td></tr>\
													  <tr><td width="210px"><?php echo _("Replication Thread"); ?></td> 							 <td><?php echo $LOADER_NUMBER; ?></td></tr>\
													  <tr><td width="210px"><?php echo _("Replication Trigger %"); ?></td> 							 <td><?php echo $LOADER_TRIGGER; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Image Export Path"); ?></td>			 <td><?php echo str_replace('\\','\\\\',$EXPORT_PATH); ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Image Export Type"); ?></td>			 <td><?php echo $EXPORT_TYPE; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Use Customized Id"); ?></td>			 <td><?php echo $SET_DISK_CUSTOMIZED_ID; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Create by Partition Size"); ?></td>	 <td><?php echo $CREATE_BY_PARTITION; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Continuous Data Replication"); ?></td><td><?php echo $ENABLE_CDR; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Retry Replication"); ?></td>			 <td><?php echo $REPLICATION_RETRY; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Packer Encryption"); ?></td>			 <td><?php echo $PACKER_ENCRYPTION; ?></td></tr>\
													  <tr class="super_hidden"><td width="210px"><?php echo _("Replication Exclude Paths"); ?></td>	 <td><?php echo $EXCLUDED_PATHS; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("Image Checksum"); ?></td>		 	 <td><?php echo $CHECKSUM_VERIFY; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("Job Task Overlapping"); ?></td>		 <td><?php echo $BLOCK_MODE_ENABLE; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("Schedule Pause"); ?></td>			 <td><?php echo $SCHEDULE_PAUSE; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("Data Compression"); ?></td>			 <td><?php echo $DATA_COMPRESSED; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("Data Checksum"); ?></td>				 <td><?php echo $DATA_CHECKSUM; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("File System Filter"); ?></td>		 <td><?php echo $FILE_SYSTEM_FILTER; ?></td></tr>\
													  <tr class="debug_hidden"><td width="210px"><?php echo _("Extra GB"); ?></td>					 <td><?php echo $EXTRA_GB; ?></td></tr>\
													');
				
				if( '<?php echo $VMWARE_STORAGE; ?>' != 'NA' ){
					$('#IntervalSetting > tbody').append(
						'<tr><td width="210px"><?php echo _("Target ESX"); ?></td>				<td><?php echo $VMWARE_ESX; ?></td></tr>\
						<tr><td width="210px"><?php echo _("Target datastore"); ?></td>			<td><?php echo $VMWARE_STORAGE; ?></td></tr>\
						<tr><td width="210px"><?php echo _("VM Folder"); ?></td>				<td><?php echo $VMWARE_FOLDER; ?></td></tr>\
						<tr><td width="210px"><?php echo _("Thin Provisioning"); ?></td>		<td><?php echo $VMWARE_THIN_PROVISIONED; ?></td></tr>'
					);
				}

				if( <?php echo (isset($_SESSION["CLOUD_TYPE"]) && $_SESSION["CLOUD_TYPE"] == "Azure")?"true":"false"?> ){
					$('<tr><td width="210px"><?php echo _("Use Azure Blob Mode"); ?></td><td><?php echo $BLOB_MODE; ?></td></tr>').appendTo('#IntervalSetting');
				}
				
				if ( '<?php echo $_SESSION["HOST_OS"]; ?>' == "WINDOWS" &&  '<?php echo $_SESSION["HOST_TYPE"]; ?>' == "Physical") {
					$('<tr><td width="210px"><?php echo _("Packer Data compression"); ?></td><td><?php echo $PACKER_DATA_COMPRESSED; ?></td></tr>').appendTo('#IntervalSetting');
				}

				ChangeOptionLevel();
				QueryConnectionInformationByServUUID(jso.HOST_TYPE);
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
	/* Query Select Server::Host */
	function QueryServerInformation(SERV_UUID){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryServerInformation',
				 'SERV_UUID' : SERV_UUID
			},
			success:function(jso)
			{
				SERV_TYPE = jso.SERV_TYPE;
				SERV_TYPE = SERV_TYPE.replace(/\s/g, '');
				if (SERV_TYPE == 'VirtualPacker')
				{					
					$('#'+SERV_TYPE+'Table > tbody').append('\
						<tr><td width="210px"><?php echo _("Transport Server"); ?></td>	<td>'+jso.HOST_NAME+'</td></tr>\
						<tr><td width="210px"><?php echo _("Server Address"); ?></td>	<td>'+jso.SERV_ADDR+'</td></tr>\
						<tr><td width="210px"><?php echo _("Packer Type"); ?></td>	<td><?php echo _("Packer for VMware"); ?></td></tr>\
						<tr><td width="210px"><?php echo _("ESX Address"); ?></td>	<td>'+jso.SERV_MISC['ADDR']+' ('+jso.SERV_MISC['USER']+')</td></tr>\
					');
				}
				else
				{
					$('#'+SERV_TYPE+'Table > tbody').append('\
						<tr><td width="210px"><?php echo _("Transport Server"); ?></td>	<td>'+jso.HOST_NAME+'</td></tr>\
						<tr><td width="210px"><?php echo _("Server Address"); ?></td>	<td>'+jso.SERV_ADDR+'</td></tr>\
						<tr><td width="210px"><?php echo _("Packer Type"); ?></td>		<td>'+jso.SERV_TYPE+'</td></tr>\
					');
				}				
			},
			error: function(xhr)
			{
				
			}
		});
	}
		
	
	/* Query Connection	*/
	function QueryConnectionInformationByServUUID(HOST_TYPE){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryConnectionInformationByServUUID',
				 'HOST_UUID' :CARR_UUID,
				 'LOAD_UUID' :'<?php echo $_SESSION['LOAD_UUID']; ?>',
				 'LAUN_UUID' :'<?php echo $_SESSION['LAUN_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{					
					$('#ConnectionTable > tbody').append(
						'<tr><td width="210px"><?php echo _("Source Server"); ?></td>									  <td>'+jso.CARR_INFO.HOST_NAME+' ('+jso.CARR_INFO.SERV_ADDR+')</td></tr>\
						 <tr><td width="210px"><?php echo _("Target Server"); ?></td>									  <td>'+jso.SERV_INFO.HOST_NAME+' ('+jso.SERV_INFO.SERV_ADDR+')</td></tr>\
						 <tr class="carrier_hidden"><td width="210px"><?php echo _("Carrier Priority Address"); ?></td>	  <td><?php echo $PRIORITY_ADDR; ?></td></tr>\
						 <tr><td width="210px"><?php echo _("Connection Type"); ?></td>									  <td><div id="ConnTypeText"></div></td></tr>\
						 <tr class="webDAV_hidden"><td width="210px"><?php echo _("Preferred Connection Address"); ?></td><td><?php echo $_SESSION['REPLICA_SETTINGS']['WebDavPriorityAddr']; ?></td></tr>\
						 <tr><td width="210px"><?php echo _("Management Address"); ?></td>								  <td>'+document.domain+'</td></tr>'
					);
					
					if (jso.CONN_DATA.CONN_TYPE == 'LocalFolder')
					{
						$('#ConnTypeText').text("Local");
					}
					else
					{
						$(".webDAV_hidden").show();
						if (jso.CARR_INFO.SERV_INFO.direct_mode == true)
						{
							$('#ConnTypeText').text("RPC");
						}
						else
						{
							$('#ConnTypeText').text("HTTPS");
						}
					}
					
					if (HOST_TYPE == 'Physical')
					{
						$(".carrier_hidden").show();
					}
			
					if('<?php echo $_SESSION['REPLICA_SETTINGS']['MultiLayerProtection']; ?>' != 'NoSeries'){
						$('<tr><td width="210px"><?php echo _("Multi Layer Protection"); ?></td><td style="text-transform:capitalize;"><?php echo $_SESSION['REPLICA_SETTINGS']['MultiLayerProtection']; ?></td></tr>').appendTo('#ConnectionTable');
					}
					
				}
				else
				{
					$('#ConnectionTable > tbody').append('<tr><td class="organisationnumber">-</td>\
					<td colspan="8">No Connection</td>\
					</tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Click And Run Replica Async */
	function BeginToRunReplicaAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    			:'BeginToRunReplicaAsync',
				 'ACCT_UUID' 			:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 			:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'HOST_UUID' 			:'<?php echo $_SESSION['HOST_UUID']; ?>',
				 'CARR_UUID' 			:CARR_UUID,
				 'LOAD_UUID' 			:'<?php echo $_SESSION['LOAD_UUID']; ?>',
				 'LAUN_UUID' 			:'<?php echo $_SESSION['LAUN_UUID']; ?>',
				 'REPLICA_SETTINGS'		:'<?php echo json_encode($_SESSION['REPLICA_SETTINGS']); ?>',
				 'MGMT_ADDR' 			:document.domain
			},	
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					message: '<?php echo _("Replication job submitted."); ?>',
					draggable: true,
					closable: false,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						window.location.href = "MgmtPrepareWorkload"
					},
				});	
			}			
		});
	}
	
	/* Submit Trigger */
	$(function(){
		$("#BackToMgmtPrepareWorkload").click(function(){
			window.location.href = "MgmtPrepareWorkload";
		})
		
		$("#BackToReplicaInterval").click(function(){
			window.location.href = "ConfigureSchedule";
		})
		
		$("#ReplicaSubmitAndRun").one("click" ,function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			$('#BackToMgmtPrepareWorkload').prop('disabled', true);
			$('#BackToReplicaInterval').prop('disabled', true);
			$('#ReplicaSubmitAndRun').prop('disabled', true);
			$('#ReplicaSubmitAndRun').removeClass('btn-primary').addClass('btn-default');
			BeginToRunReplicaAsync();
		})
	});
});
</script>
<style>
th, td {
    text-align: left;
	padding: 4px;
	height: 38px;
}

.special{
	padding-left:5px;
}

.carrier_hidden{
	display:none;
}

.webDAV_hidden{
	display:none;
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
					<li style='width:20%'><a>				<?php echo _("Step 3 - Configure Workload"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 4 - Configure Replication"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 5 - Replication Summary"); ?></a></li>
				</ul>
			</div>
						
			<div id='form_block_wizard'>
				<table id="HostSummaryTable">
					<thead>
					<tr>
						<th colspan="2"><?php echo _("Host Information"); ?></th>						
					</tr>
					</thead>				
					<tbody></tbody>
				</table>
		
				<table id="ConnectionTable">
					<thead>
					<tr>
						<th colspan="2"><?php echo _("Transport Server Information"); ?></th>						
					</tr>
					</thead>				
					<tbody></tbody>
				</table>
				
				<table id="VirtualPackerTable">
					<thead>
					<tr>
						<th colspan="2"><?php echo _("Packer for VMware Information"); ?></th>						
					</tr>
					</thead>				
					<tbody></tbody>
				</table>
				
				<table id="IntervalSetting">
					<thead>
					<tr>
						<th colspan="2"><?php echo _("Replication Configuration"); ?></th>						
					</tr>
					</thead>				
					<tbody></tbody>
				</table>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToReplicaInterval' 		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtPrepareWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='ReplicaSubmitAndRun' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Run"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>