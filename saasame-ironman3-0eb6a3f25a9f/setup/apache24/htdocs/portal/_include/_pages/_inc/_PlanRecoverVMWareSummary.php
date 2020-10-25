<?php

	if(isset($_SESSION['EDIT_PLAN']) && $_SESSION['EDIT_PLAN'] != false)
	{
		$_SESSION['RECY_TYPE'] 			= $_SESSION['EDIT_PLAN'] -> RecoverType;
		$_SESSION['SERVICE_SETTINGS']['CPU']		= $_SESSION['EDIT_PLAN'] -> CPU;
		$_SESSION['SERVICE_SETTINGS']['Memory']			= $_SESSION['EDIT_PLAN'] -> Memory;
		$_SESSION['SERVICE_SETTINGS']['network_uuid']		= $_SESSION['EDIT_PLAN'] -> network_uuid;
		$_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']	= $_SESSION['EDIT_PLAN'] -> rcvy_pre_script;
		$_SESSION['SERVICE_SETTINGS']['rcvy_post_script']	= $_SESSION['EDIT_PLAN'] -> rcvy_post_script;
		$_SESSION['VENDOR_NAME'] = $_SESSION['EDIT_PLAN'] -> VendorName;
	
		$BACK_TO = 'EditPlanInstanceVMWareConfigurations';
		$PLAN_UUID = $_SESSION['EDIT_PLAN']->PlanUUID;
	}
	else
	{
		$PLAN_UUID = false;
		$BACK_TO = 'PlanInstanceVMWareConfigurations';
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
	
	DetermineSegment('EditPlanRecoverVMWareSummary');
	
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();
		if (URL_SEGMENT == SET_SEGMENT)
		{
			urlPrefix = 'Edit';
			$("#SaveRecoverPlan").text("<?php echo _('Update'); ?>");
		}
		else
		{
			urlPrefix = '';
		}
	}
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#BackToInstanceConfigurations').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
		
		setTimeout(function(){
			/* Change Submit Status */
			$('#SaveRecoverPlan').prop('disabled', false);
			$('#SaveRecoverPlan').removeClass('btn-default').addClass('btn-primary');		
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
					<tr><td width="200px"><?php echo _("Recovery Pre Script"); ?></td>	<td><?php echo $PRE_FILE_NAME; ?></td></tr>\
					<tr><td width="200px"><?php echo _("Recovery Post Script"); ?></td>	<td><?php echo $POST_FILE_NAME; ?></td></tr>\
				');
				
				var migration_executed = jso.JOBS_JSON.migration_executed;				
				if (typeof(migration_executed) != "undefined" && migration_executed !== null && migration_executed == true)
				{
					$('#TRIGGER_SYNC').prop('disabled', true);
				}
				
				QRecoveryInformation();
				
			
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
			 <tr><td width="200px"><?php echo _("ESX Datastores"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['VMWARE_STORAGE']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("OS Type"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['OSTYPE_DISPLAY']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("SCSI Controller Type"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['VMWARE_SCSI_CONTROLLER']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("VM Folder"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['VMWARE_FOLDER']; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("CPU"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]['CPU']; ?> core</td></tr>\
			 <tr><td width="200px"><?php echo _("Memory"); ?></td> <td><?php echo $_SESSION["SERVICE_SETTINGS"]["Memory"]; ?> MB</td></tr>\
			 <tr><td width="200px"><?php echo _("Firmware"); ?></td> <td><?php echo $_SESSION["SERVICE_SETTINGS"]["FIRMWARE"]?"FIRMWARE EFI":"FIRMWARE BIOS"; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("Convert"); ?></td>	<td><?php echo $_SESSION["SERVICE_SETTINGS"]["CONVERT"] == "true"?"Yes":"No"; ?> </td></tr>\
			 <tr><td width="200px"><?php echo _("VM Tool"); ?></td> <td><?php echo $_SESSION["SERVICE_SETTINGS"]["VM_TOOL"] == "true"?"Yes":"No"; ?> </td></tr>\
			 <?php
			 if( isset( $_SESSION["SERVICE_SETTINGS"]['NETWORK_UUID'] ))
			 {
				for( $i = 0; $i < count( $_SESSION["SERVICE_SETTINGS"]['NETWORK_UUID'] ); $i++){
					echo '<tr><td width="200px">'._("Network").' '.$i.'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_UUID"][$i]["network"].'</td></tr>\\';
					echo '<tr><td width="200px">'._("VM Network Adapter").' '.$i.'</td>	<td>'.$_SESSION["SERVICE_SETTINGS"]["NETWORK_UUID"][$i]["type"].'</td></tr>\\';
				}
			 }

			?>
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
	
	/* Begin To Save Recover Plan */
	function SaveRecoverPlan( RECOVER_PLAN ){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'       :'SaveRecoverPlan',
				 'ACCT_UUID'    :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID'    :'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'RECOVER_PLAN'	:RECOVER_PLAN
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _("Service Message"); ?>",
					message: "<?php echo _("New recovery plan added."); ?>",
					draggable: true,
					closable: false,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						window.location.href = "MgmtRecoverPlan";
					},
				});
			},
		});
	}

	/* Begin To Update Recover Plan */
	function UpdateRecoverPlan( RECOVER_PLAN ){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'       :'UpdateRecoverPlan',
				 'PLAN_UUID'    :'<?php echo $PLAN_UUID; ?>',
				 'RECOVER_PLAN'	:RECOVER_PLAN
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _("Service Message"); ?>",
					message: "<?php echo _("Recovery plan updated."); ?>",
					draggable: true,
					closable: false,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						window.location.href = "MgmtRecoverPlan";
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
		
		$("#SaveRecoverPlan").one("click" ,function(){

		var net =<?php echo isset($_SESSION['SERVICE_SETTINGS']['NETWORK_SETTING'])? json_encode($_SESSION['SERVICE_SETTINGS']['NETWORK_SETTING']  ):"null";?>;
		var net_uuid = <?php echo json_encode( $_SESSION['SERVICE_SETTINGS']['NETWORK_UUID'] ); ?>;
		
		var RECOVER_PLAN = {
				'ReplUUID'		 	:'<?php echo $_SESSION['REPL_UUID']; ?>',
				'ServiceRegin'   	:'<?php echo $_SESSION['SERV_REGN']; ?>',
				'CloudType'		 	:'<?php echo $_SESSION['CLOUD_TYPE']; ?>',
				'VendorName'	 	:'<?php echo $_SESSION['VENDOR_NAME']; ?>',
				'RecoverType'	 	:'<?php echo $_SESSION['RECY_TYPE']; ?>',
				'network_uuid'	 	: net_uuid,
				'CPU'	 			:'<?php echo $_SESSION['SERVICE_SETTINGS']['CPU']; ?>',
				'Memory'	 		:'<?php echo $_SESSION['SERVICE_SETTINGS']['Memory']; ?>',
				'VMWARE_STORAGE'	:'<?php echo $_SESSION['SERVICE_SETTINGS']['VMWARE_STORAGE']; ?>',
				'VMWARE_FOLDER'	 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['VMWARE_FOLDER']; ?>',
				'ClusterUUID'	 	:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				'VM_NAME'		 	:'<?php echo $_SESSION['SERV_UUID']; ?>',
				'ServerUUID'	 	:'<?php echo $_SESSION['SERVER_UUID']; ?>',
				'rcvy_pre_script'  	:'<?php echo $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']; ?>',
				'rcvy_post_script' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['rcvy_post_script']; ?>',
				'hostname_tag'		:'<?php echo $_SESSION['SERVICE_SETTINGS']['hostname_tag']; ?>',
				'HostType'		 	:'<?php echo $_SESSION['REPL_OS_TYPE']; ?>',
				"VMWARE_SCSI_CONTROLLER" : '<?php echo $_SESSION['SERVICE_SETTINGS']['VMWARE_SCSI_CONTROLLER']; ?>',
				"CONVERT" : <?php echo $_SESSION["SERVICE_SETTINGS"]["CONVERT"] == "true"?"true":"false"; ?>,
				"VM_TOOL" : <?php echo $_SESSION["SERVICE_SETTINGS"]["VM_TOOL"] == "true"?"true":"false"; ?>,
				"FIRMWARE" : '<?php echo $_SESSION['SERVICE_SETTINGS']['FIRMWARE']; ?>',
				"NETWORK_SETTING" : net,
				"OSTYPE_DISPLAY"	:'<?php echo $_SESSION['SERVICE_SETTINGS']['OSTYPE_DISPLAY']; ?>'
			};
						
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');	
			$('#BackToMgmtRecoverWorkload').prop('disabled', true);
			$('#BackToInstanceConfigurations').prop('disabled', true);
			$('#SaveRecoverPlan').prop('disabled', true);
			$('#SaveRecoverPlan').removeClass('btn-primary').addClass('btn-default');
			
			if (urlPrefix == '')
			{
				SaveRecoverPlan( RECOVER_PLAN );
			}
			else
			{
				UpdateRecoverPlan( RECOVER_PLAN );
			}

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
					<li style='width:25%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 3 - Configure Instance"); ?></a></li>
					<li style='width:25%' class='active'><a><?php echo _("Step 4 - Recover Summary"); ?></a></li>
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
					<button id='SaveRecoverPlan' 			class='btn btn-default pull-right btn-lg' disabled><?php echo _("Save"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>