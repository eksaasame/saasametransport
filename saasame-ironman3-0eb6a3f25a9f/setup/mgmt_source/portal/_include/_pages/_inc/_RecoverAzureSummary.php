<?php
	if($_SESSION['PLAN_JSON'] != false)
	{
		$PLAN_JSON = json_decode($_SESSION['PLAN_JSON']);
		$_SESSION['RECY_TYPE'] = $PLAN_JSON -> RecoverType;
		$_SESSION['SERVICE_SETTINGS']['snap_uuid'] 			= 'UseLastSnapshot';
		$_SESSION['SERVICE_SETTINGS']['sgroup_uuid'] 		= $PLAN_JSON -> sgroup_uuid;
		$_SESSION['SERVICE_SETTINGS']['flavor_id'] 			= $PLAN_JSON -> flavor_id;
		$_SESSION['SERVICE_SETTINGS']['network_uuid'] 		= $PLAN_JSON -> network_uuid;
		$_SESSION['SERVICE_SETTINGS']['elastic_address_id']	= $PLAN_JSON -> elastic_address_id;
		$_SESSION['SERVICE_SETTINGS']['private_address_id']	= $PLAN_JSON -> private_address_id;
		$_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']	= $PLAN_JSON -> rcvy_pre_script;
		$_SESSION['SERVICE_SETTINGS']['rcvy_post_script']	= $PLAN_JSON -> rcvy_post_script;
		$_SESSION['SERVICE_SETTINGS']['disk_type']			= $PLAN_JSON -> disk_type;
		$_SESSION['SERVICE_SETTINGS']['availability_set']	= $PLAN_JSON -> availability_set;
		$_SESSION['SERVICE_SETTINGS']['hostname_tag']		= $PLAN_JSON -> hostname_tag;
		$_SESSION['SERVICE_SETTINGS']['datamode_instance']	= $PLAN_JSON -> datamode_instance;
	
		$PLAN_UUID = $_SESSION['PLAN_UUID'];
		$REPLICA_DISK = implode(',',$PLAN_JSON -> CloudDisk);
		$BACK_TO = 'SelectRecoverHost';
	}
	else
	{
		$PLAN_UUID = false;
		$REPLICA_DISK = false;
		$BACK_TO = 'InstanceAzureConfigurations';
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
	
	function QueryDataModeInstance(){

		<?php $serverInfo =  explode('/',$_SESSION['SERVICE_SETTINGS']['datamode_instance']); ?>
		
		var serverName = '<?php echo isset($serverInfo["8"])?$serverInfo["8"]:''; ?>';
		var resourceGroup = '<?php echo isset($serverInfo["4"])?$serverInfo["4"]:''; ?>';
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_NAME' 	:serverName,
				 'HOST_REGN'	:'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'SERV_UUID'	:'<?php echo $_SESSION['SERVER_UUID']; ?>',
				 'HOST_RG'		:resourceGroup
			},
			success:function(jso)
			{
				$('#DataModeInstance > tbody').append(
					'<tr><th colspan="2"><?php echo _("DataMode Instance Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Instance Name"); ?></td>		<td>'+jso.name+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Location"); ?></td>	<td>'+jso.location+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Private IP Address"); ?></td>	<td>'+jso.private_ip+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Public IP Address"); ?></td>		<td>'+jso.public_ip+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Security Groups"); ?></td>		<td>'+jso.security_group+'</td></tr>\
				');

			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
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
					 <tr><td width="200px"><?php echo _("Public IP"); ?></td>		<td><?php echo $_SESSION['SERVICE_SETTINGS']['elastic_address_id']; ?></td></tr>\
					 <tr><td width="200px"><?php echo _("Specified Private IP"); ?></td>	<td><?php echo ($_SESSION['SERVICE_SETTINGS']['private_address_id'] != '')?$_SESSION['SERVICE_SETTINGS']['private_address_id']:'DHCP'; ?></td></tr>\
				');
				
				var migration_executed = jso.JOBS_JSON.migration_executed;				
				if (typeof(migration_executed) != "undefined" && migration_executed !== null && migration_executed == true)
				{
					$('#TRIGGER_SYNC').prop('disabled', true);
				}
				
				DATAMODE_INSTANCE = '<?php echo $_SESSION['SERVICE_SETTINGS']['datamode_instance']; ?>';
				if (DATAMODE_INSTANCE == 'NoAssociatedDataModeInstance')
				{				
					QueryFlavorInformation();
					QueryNetworkInformation();
					QuerySecurityGroupInformation();				
				
					$('#RecoveryScript > tbody').append(
						'<tr><th colspan="2"><?php echo _("Recovery Script"); ?></th></tr>\
						 <tr><td width="200px"><?php echo _("Recovery Pre Script"); ?></td>	<td><?php echo $PRE_FILE_NAME; ?></td></tr>\
						 <tr><td width="200px"><?php echo _("Recovery Post Script"); ?></td>	<td><?php echo $POST_FILE_NAME; ?></td></tr>\
					');
				}
				else
				{
					QueryDataModeInstance();
				}
				
				SNAP_UUID = '<?php echo $_SESSION['SERVICE_SETTINGS']['snap_uuid']; ?>';
				if (SNAP_UUID != 'Planned_Migration' && SNAP_UUID != "false")
				{
					QuerySnapshotInformation();
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Flavor Config */
	function QueryFlavorInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    	:'QueryFlavorInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'FLAVOR_ID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['flavor_id']; ?>'				 
			},
			success:function(jso)
			{
				$('#FlavorSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("VM Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Instance Type Name"); ?></td>	<td>'+jso.name+'</td></tr>\
					 <tr><td width="200px"><?php echo _("vCPU"); ?></td>				<td>'+jso.numberOfCores+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Memory"); ?></td>				<td>'+jso.memoryInMB/1024+'GB'+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Max Disk Mount"); ?></td>		<td>'+jso.maxDataDiskCount+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Availability Set"); ?></td>	<td>'+'<?php $Availibility = explode( '/', $_SESSION['SERVICE_SETTINGS']['availability_set'] );
					 echo ( $_SESSION['SERVICE_SETTINGS']['availability_set'] === false )?"no use":end($Availibility) ; ?>'+'</td></tr>\
				');
				
				QueryServerInformation();
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	function QueryServerInformation(){
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryServerInformation',
				 'SERV_UUID' : <?php echo '"'.$_SESSION['SERVER_UUID'].'"'; ?>
			},
			success:function(jso)
			{
				if( jso.SERV_INFO.is_azure_mgmt_disk )
					$('#FlavorSummaryTable > tbody').append(
						'<tr><td width="200px"><?php echo _("Disk Type"); ?></td>			<td>'+'<?php echo $_SESSION['SERVICE_SETTINGS']['disk_type']; ?>'+'</td></tr>\
				');
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Network Config */
	function QueryNetworkInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    	:'QueryNetworkInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'NETWORK_UUID' :'<?php echo $_SESSION['SERVICE_SETTINGS']['network_uuid']; ?>'				 
			},
			success:function(jso)
			{
				var subnet = jso.id.split("/");
				$('#NetworkSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Network Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Virtual Network"); ?></td>				<td>'+subnet[8]+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Subnet"); ?></td>						<td>'+subnet[10]+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Address Prefix"); ?></td>				<td>'+jso.properties.addressPrefix+'</td></tr>\
					');
				},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Query Snapshot Information */
	function QuerySnapshotInformation(){
		
		if( <?php echo (isset($_SESSION['SERVICE_SETTINGS']["IsAzureBlobMode"] ) && $_SESSION['SERVICE_SETTINGS']["IsAzureBlobMode"] )? $_SESSION['SERVICE_SETTINGS']["IsAzureBlobMode"]: "false"  ?>)
		{
			BLOB_MODE = true;
		}
		else if( <?php echo (isset($_SESSION["IsAzureBlobMode"] ) && $_SESSION["IsAzureBlobMode"] )? $_SESSION["IsAzureBlobMode"]: "false"  ?>){
			BLOB_MODE = true;
		}
		else
		{
			BLOB_MODE = false;
		}
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    	:'QuerySnapshotInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'SNAPSHOT_ID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['snap_uuid']; ?>',
				 'REPLICA_UUID'	:'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'REPLICA_DISK'	:'<?php echo $REPLICA_DISK; ?>',
				 'BLOB_MODE'	:BLOB_MODE				 
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					$('#SnapshotSummaryTable > tbody').append(
						'<tr><th colspan="2"><?php echo _("Snapshot Information - Disk"); ?> '+key+'</th></tr>\
						 <tr><td width="200px"><?php echo _("Snapshot Size"); ?></td> <td>'+value.size+'GB</td></tr>\
						 <tr><td width="200px"><?php echo _("Description"); ?></td>	  <td>'+value.description+'</td></tr>\
						 <tr><td width="200px"><?php echo _("Created at"); ?></td>	  <td>'+value.created_at+'</td></tr>\
					');
				});
			},
			error: function(xhr)
			{
				
			}
		});
	}
			
	/* Query Security Group Config */
	function QuerySecurityGroupInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    	:'QuerySecurityGroupInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'SGROUP_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['sgroup_uuid']; ?>'		
			},
			success:function(jso)
			{
				$('#SecurityGroupSummaryTable > tbody').append(
					'<tr><th colspan="2">Security Group Information</th></tr>\
					 <tr><td width="200px">Security Group Name</td>	<td>'+jso.name+'</td></tr>\
					');
			},
			error: function(xhr)
			{
				
			}
		});	
	}

	
	/* Click And Run Service On Azure */
	function BeginToRunServiceAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'       	:'BeginToRunServiceAsync',
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
				
				<table id="DataModeInstance">
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