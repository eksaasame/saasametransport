<?php
	if($_SESSION['PLAN_JSON'] != false)
	{
		$PLAN_JSON = json_decode($_SESSION['PLAN_JSON']);
		$_SESSION['RECY_TYPE'] 			= $PLAN_JSON -> RecoverType;
		$_SESSION['SERVICE_SETTINGS']['CPU']		= $PLAN_JSON -> CPU;
		$_SESSION['SERVICE_SETTINGS']['Memory']			= $PLAN_JSON -> Memory;
		$_SESSION['SERVICE_SETTINGS']['network_uuid']		= $PLAN_JSON -> network_uuid;
		$_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']	= $PLAN_JSON -> rcvy_pre_script;
		$_SESSION['SERVICE_SETTINGS']['rcvy_post_script']	= $PLAN_JSON -> rcvy_post_script;
		$_SESSION['SERVICE_SETTINGS']['VMWARE_STORAGE']		= $PLAN_JSON -> VMWARE_STORAGE;
		$_SESSION['SERVICE_SETTINGS']['VMWARE_FOLDER']		= $PLAN_JSON -> VMWARE_FOLDER;
		$_SESSION['SERVICE_SETTINGS']['NETWORK_UUID']		= json_decode( json_encode( $PLAN_JSON -> network_uuid ), true );
		$_SESSION['SERVICE_SETTINGS']['hostname_tag']		= $PLAN_JSON -> hostname_tag;
		$_SESSION['SERVICE_SETTINGS']['VMWARE_SCSI_CONTROLLER']		= $PLAN_JSON -> VMWARE_SCSI_CONTROLLER;
		$_SESSION['SERVICE_SETTINGS']['FIRMWARE']		= $PLAN_JSON -> FIRMWARE;
		$_SESSION['SERVICE_SETTINGS']['CONVERT']		= $PLAN_JSON -> CONVERT;
		$_SESSION['SERVICE_SETTINGS']['VM_TOOL']		= $PLAN_JSON -> VM_TOOL;
		$_SESSION['SERVICE_SETTINGS']['OSTYPE_DISPLAY']	= $PLAN_JSON -> OSTYPE_DISPLAY;
		$_SESSION['SERVICE_SETTINGS']['snap_uuid']			= 'UseLastSnapshot';
	
		$BACK_TO = 'SelectRecoverHost';
		$PLAN_UUID = $_SESSION['PLAN_UUID'];
		$_SESSION['SNAP_UUID'] = 'UseLastSnapshot';
	}
	else
	{
		$PLAN_UUID = false;
		$BACK_TO = 'InstanceVMWareConfigurations';
	}
	if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM'){$RECY_TYPE = 'Planned Migration';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DR'){$RECY_TYPE = 'Disaster Recovery';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DT'){$RECY_TYPE = 'Development Testing';}
	if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM'){$TRIGGER_DISPLAY = '';}else{$TRIGGER_DISPLAY = 'display:none';}
	if ($_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'] == ''){$PRE_FILE_NAME = 'Not Applicable';}else{$PRE_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'];}
	if ($_SESSION['SERVICE_SETTINGS']['rcvy_post_script'] == ''){$POST_FILE_NAME = 'Not Applicable';}else{$POST_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_post_script'];}
?>
<script>
$( document ).ready(function() {
	<!-- Exec -->
	QueryReplicaInformation();
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#BackToInstanceConfigurations').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
		
		setTimeout(function(){
			/* Change Submit Status */
			$('#ServiceSubmitAndRun').prop('disabled', false);
			$('#ServiceSubmitAndRun').removeClass('btn-default').addClass('btn-primary');		
		},1000);
	});
	
	/* Query Select Replica */
	function QueryReplicaInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryReplicaInformation',
				 'REPL_UUID' :'<?php echo $_SESSION['REPL_UUID']; ?>'				 
			},
			success:function(jso)
			{
				$('#ReplicaSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Host Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Host Name"); ?></td>		<td>'+jso.HOST_NAME+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Host Address"); ?></td>	<td>'+jso.HOST_ADDR+'</td></tr>\
				');
				
				$('#RecoveryScript > tbody').append(
					'<tr><th colspan="2"><?php echo _("Recovery Script"); ?></th></tr>\
					<tr><td width="200px"><?php echo _("Pre Recovery Script"); ?></td>	<td><?php echo $PRE_FILE_NAME; ?></td></tr>\
					<tr><td width="200px"><?php echo _("Post Recovery Script"); ?></td>	<td><?php echo $POST_FILE_NAME; ?></td></tr>\
				');
				
				var migration_executed = jso.JOBS_JSON.migration_executed;				
				if (typeof(migration_executed) != "undefined" && migration_executed !== null && migration_executed == true)
				{
					$('#TRIGGER_SYNC').prop('disabled', true);
				}
				
				QRecoveryInformation();
				
				SNAP_UUID = '<?php echo $_SESSION['SNAP_UUID']; ?>';
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Flavor Config */
	function QRecoveryInformation(){

		$('#FlavorSummaryTable > tbody').append(
			'<tr><th colspan="2"><?php echo _("Recovery Information"); ?></th></tr>\
			 <tr><td width="200px"><?php echo _("VM Name"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['hostname_tag']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("OS Type"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['OSTYPE_DISPLAY']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("ESX Datastores"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['VMWARE_STORAGE']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("SCSI Controller Type"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['VMWARE_SCSI_CONTROLLER']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("VM Folder"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['VMWARE_FOLDER']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("CPU"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['CPU']; ?> core</td></tr>\
			 <tr><td width="200px"><?php echo _("Memory"); ?></td> <td><?php echo $_SESSION["SERVICE_SETTINGS"]["Memory"]; ?> MB</td></tr>\
			 <tr><td width="200px"><?php echo _("Firmware"); ?></td> <td><?php echo $_SESSION["SERVICE_SETTINGS"]["FIRMWARE"]?"FIRMWARE EFI":"FIRMWARE BIOS"; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("Convert"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]["CONVERT"] == "true"?"Yes":"No"; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("VMware Tools"); ?></td> <td><?php echo $_SESSION["SERVICE_SETTINGS"]["VM_TOOL"] == "true"?"Yes":"No"; ?> </td></tr>\
			 <?php
			 if( isset( $_SESSION["SERVICE_SETTINGS"]['NETWORK_UUID'] )){
				 
				for( $i = 0; $i < count( $_SESSION["SERVICE_SETTINGS"]['NETWORK_UUID'] ); $i++){
					echo '<tr><td width="200px">'._("Network").' '.$i.'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_UUID"][$i]["network"].'</td></tr>\\';
					echo '<tr><td width="200px">'._("VM Network Adapter").' '.$i.'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_UUID"][$i]["type"].'</td></tr>\\';
				}
			 }
			?><tr><td width="200px"><?php echo _("Snapshot ID"); ?></td>		<td><?php echo $_SESSION["SNAP_UUID"]; ?></td></tr>\
		');
		
		$('#NetworkSummaryTable > tbody').append(
			'<?php
				
				if( isset( $_SESSION["SERVICE_SETTINGS"]['NETWORK_SETTING'] ) ){
					end( $_SESSION["SERVICE_SETTINGS"]['NETWORK_SETTING'] );
					$lastkey = key( $_SESSION["SERVICE_SETTINGS"]['NETWORK_SETTING'] );
				}
				else
					$lastkey = 0;
				
				if( isset( $_SESSION["SERVICE_SETTINGS"]['NETWORK_SETTING'] ) ){
					for( $i = 0; $i <= $lastkey ; $i++)
					{
						if( !isset( $_SESSION["SERVICE_SETTINGS"]["NETWORK_SETTING"][$i] ) )
							continue;
						
						echo '<tr><th colspan="2">'._("Network").' '.$i.'</th></tr>\\';
						echo '<tr><td width="200px">'._("IP Address").'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_SETTING"][$i]["ip"].'</td></tr>\\';
						echo '<tr><td width="200px">'._("Subnet").'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_SETTING"][$i]["subnet"].'</td></tr>\\';
						echo '<tr><td width="200px">'._("Gateway").'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_SETTING"][$i]["gateway"].'</td></tr>\\';
						echo '<tr><td width="200px">'._("DNS").'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_SETTING"][$i]["dns"].'</td></tr>\\';
						echo '<tr><td width="200px">'._("MAC address").'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_SETTING"][$i]["mac"].'</td></tr>\\';
					}
				}
			?>
		');
	}
	
	/* Click And Run Service On Azure */
	function BeginToRunServiceAsync(){
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				'ACTION'       	:'BeginToRunVMWareServiceAsync',
				 'ACCT_UUID'    	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID'    	:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'REPL_UUID'    	:'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'RECY_TYPE'		:'<?php echo $_SESSION['RECY_TYPE']; ?>',
				 'PLAN_UUID'	   	:'<?php echo $PLAN_UUID; ?>',
				 'SERVICE_SETTINGS'	:'<?php echo json_encode($_SESSION['SERVICE_SETTINGS']); ?>',
				 'TRIGGER_SYNC' 	:$('#TRIGGER_SYNC:checkbox:checked').val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _("Service Message"); ?>",
					message: "<?php echo _("New recovery process added."); ?>",
					draggable: true,
					closable: false,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						window.location.href = "MgmtRecoverWorkload";
					},
				});
			},
		});
	}
	

	/* Submit Trigger */
	$(function(){
		$("#BackToMgmtRecoverWorkload").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToInstanceConfigurations").click(function(){
			window.location.href = "<?php echo $BACK_TO; ?>";
		})
		
		$("#ServiceSubmitAndRun").one("click" ,function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');	
			$('#BackToMgmtRecoverWorkload').prop('disabled', true);
			$('#BackToInstanceConfigurations').prop('disabled', true);
			$('#ServiceSubmitAndRun').prop('disabled', true);
			$('#ServiceSubmitAndRun').removeClass('btn-primary').addClass('btn-default');
			BeginToRunServiceAsync();
		})
	});
});
</script>

<style>
	.right {
		 float: right;
	}
</style>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:18%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:22%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 3 - Select Snapshot"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 4 - Configure Instance"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 5 - Recovery Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<!--
				<table id="TriggerSync">
					<tbody>
						<div class='form-group right' style='<?php echo $TRIGGER_DISPLAY; ?>'>
							Trigger data synchronize: <input id="TRIGGER_SYNC" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox">
						</div>
					</tbody>
				</table>	
				-->
				
				<table id="ReplicaSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="SnapshotSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="FlavorSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="SecurityGroupSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="NetworkSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="RecoveryScript">
					<tbody></tbody>
				</table>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToInstanceConfigurations' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>	
					<button id='BackToMgmtRecoverWorkload' 		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='ServiceSubmitAndRun' 			class='btn btn-default pull-right btn-lg' disabled><?php echo _("Run"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>