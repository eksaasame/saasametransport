<?php
if ($_SESSION['PLAN_JSON'] != false)
{
	$PLAN_JSON = json_decode($_SESSION['PLAN_JSON']);
	$_SESSION['RECY_TYPE'] 								= $PLAN_JSON -> RecoverType;
	$_SESSION['SERVICE_SETTINGS']['snap_uuid'] 			= 'UseLastSnapshot';
	$_SESSION['SERVICE_SETTINGS']['sgroup_uuid'] 		= $PLAN_JSON -> sgroup_uuid;
	$_SESSION['SERVICE_SETTINGS']['flavor_id'] 			= $PLAN_JSON -> flavor_id;
	$_SESSION['SERVICE_SETTINGS']['network_uuid'] 		= $PLAN_JSON -> network_uuid;
	$_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']	= $PLAN_JSON -> rcvy_pre_script;
	$_SESSION['SERVICE_SETTINGS']['rcvy_post_script']	= $PLAN_JSON -> rcvy_post_script;
	$_SESSION['SERVICE_SETTINGS']['elastic_address_id']	= $PLAN_JSON -> elastic_address_id;
	$_SESSION['SERVICE_SETTINGS']['private_address_id']	= $PLAN_JSON -> private_address_id;
	$_SESSION['SERVICE_SETTINGS']['hostname_tag']		= $PLAN_JSON -> hostname_tag;
	$_SESSION['SERVICE_SETTINGS']['datamode_instance']  = $PLAN_JSON -> datamode_instance;
	$REPLICA_DISK = implode(',',$PLAN_JSON -> CloudDisk);
	$PLAN_UUID = $_SESSION['PLAN_UUID'];
	
	$BACK_TO = 'SelectRecoverHost';
}
else
{
	$PLAN_UUID = false;
	$REPLICA_DISK = false;
	$BACK_TO = 'InstanceConfigurations';
}

if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM'){$RECY_TYPE = 'Planned Migration';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DR'){$RECY_TYPE = 'Disaster Recovery';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DT'){$RECY_TYPE = 'Development Testing';}
if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM'){$TRIGGER_DISPLAY = '';}else{$TRIGGER_DISPLAY = 'display:none';}
if ($_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'] == ''){$PRE_FILE_NAME = _('Not Applicable');}else{$PRE_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'];}
if ($_SESSION['SERVICE_SETTINGS']['rcvy_post_script'] == ''){$POST_FILE_NAME = _('Not Applicable');}else{$POST_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_post_script'];}
?>
<script>
$( document ).ready(function() {
	<!-- Exec -->
	QueryReplicaInformation();
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#BackToInstanceConfiguration').prop('disabled', false);
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
				var RECY_TYPE = '<?php echo _($RECY_TYPE); ?>';			
				
				$('#ReplicaSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Host Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Recover Type"); ?></td>	<td>'+RECY_TYPE+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Host Name"); ?></td>		<td>'+jso.HOST_NAME+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Host Address"); ?></td>	<td>'+jso.HOST_ADDR+'</td></tr>\
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
								
				RECOVERY_TYPE = '<?php echo $_SESSION['RECY_TYPE']; ?>';
				if (RECOVERY_TYPE == 'RECOVERY_PM')
				{
					QueryVolumeInformation();					
				}
				else
				{
					QuerySnapshotInformation();
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Snapshot Information */
	function QuerySnapshotInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QuerySnapshotInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SNAP_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['snap_uuid']; ?>',
				 'REPLICA_DISK'	:'<?php echo $REPLICA_DISK; ?>',
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					$('#SnapshotSummaryTable > tbody').append(
						'<tr><th colspan="2"><?php echo _("Snapshot Information - Disk"); ?> '+key+'</th></tr>\
						 <tr><td width="200px"><?php echo _("Description"); ?></td>	<td>'+jso[key].snapshot.description+'</td></tr>\
						 <tr><td width="200px"><?php echo _("Size"); ?></td>		<td>'+jso[key].snapshot.size+'GB</td></tr>\
						 <tr><td width="200px"><?php echo _("Created at"); ?></td>	<td>'+jso[key].snapshot.created_at+'</td></tr>\
					');
				});
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Volume Information */	
	function QueryVolumeInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QueryVolumeInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'REPL_UUID' 	:'<?php echo $_SESSION['REPL_UUID']; ?>',	
				 'VOLUME_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['snap_uuid']; ?>'
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					$('#SnapshotSummaryTable > tbody').append(
						'<tr><th colspan="2"><?php echo _("Disk Information"); ?> '+key+'</th></tr>\
						 <tr><td width="200px"><?php echo _("Disk Name"); ?></td>	<td>'+jso[key].volume.cloud_disk_name+'</td></tr>\
						 <tr><td width="200px"><?php echo _("Disk Id"); ?></td>		<td>'+jso[key].volume.cloud_disk_id+'</td></tr>\
						 <tr><td width="200px"><?php echo _("Size"); ?></td>		<td>'+jso[key].volume.size+'GB</td></tr>\
						 <tr><td width="200px"><?php echo _("Created at"); ?></td>	<td>'+jso[key].volume.created_at+'</td></tr>\
					');
				});				
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Query Flavor Configure */
	function QueryFlavorInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    :'QueryFlavorInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'FLAVOR_ID' :'<?php echo $_SESSION['SERVICE_SETTINGS']['flavor_id']; ?>'				 
			},
			success:function(jso)
			{
				$('#FlavorSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Flavor Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Flavor Name"); ?></td>	<td>'+jso.flavor.name+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Number of vCPU"); ?></td>	<td>'+jso.flavor.vcpus+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Memory"); ?></td>		<td>'+jso.flavor.ram+'MB</td></tr>\
				');
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Query Network Configure */
	function QueryNetworkInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QueryNetworkInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'NETWORK_UUID' :'<?php echo $_SESSION['SERVICE_SETTINGS']['network_uuid']; ?>'				 
			},
			success:function(jso)
			{
				$('#NetworkSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Network Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Network Name"); ?></td>	<td>'+jso.network.name+'</td></tr>\
					');
				
				QueryFloatingIpInformation();				
				QuerySubnetInformation(jso.network.subnets[0]);
			},
			error: function(xhr)
			{
				
			}
		});	
	}
		
	/* Query Network Subnet Configure */
	function QuerySubnetInformation(SubnetUUID){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    :'QuerySubnetInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SUBNET_UUID' :SubnetUUID				 
			},
			success:function(jso)
			{
				$('#NetworkSummaryTable > tbody').append(
					'<tr><td width="200px"><?php echo _("Private Address"); ?></td>		<td><?php echo $_SESSION['SERVICE_SETTINGS']['private_address_id']; ?></td></tr>\
					 <tr><td width="200px"><?php echo _("Network CIDR"); ?></td>		<td>'+jso.subnet.cidr+'</td></tr>\
					 <tr><td width="200px"><?php echo _("DHCP enabled"); ?></td>		<td>'+jso.subnet.enable_dhcp+'</td></tr>\
					 <tr><td width="200px"><?php echo _("DHCP Address range"); ?></td>	<td>'+jso.subnet.allocation_pools[0]['start']+' to '+jso.subnet.allocation_pools[0]['end']+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Network Gateway"); ?></td>		<td>'+jso.subnet.gateway_ip+'</td></tr>\
					 <tr><td width="200px"><?php echo _("DNS Server"); ?></td>			<td>'+jso.subnet.dns_nameservers[0]+'</td></tr>\
					');				
			},
			error: function(xhr)
			{
				
			}
		});	
	}
		
	/* Query Security Group Configure */
	function QuerySecurityGroupInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QuerySecurityGroupInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SGROUP_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['sgroup_uuid']; ?>'
			},
			success:function(jso)
			{
				$('#SecurityGroupSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Security Group Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Security Group Name"); ?></td> <td>'+jso.security_group.name+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Description"); ?></td>	<td>'+jso.security_group.description+'</td></tr>\
					');	
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Query Floating IP Information */
	function QueryFloatingIpInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QueryFloatingIpInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'FLOATING_ID'  :'<?php echo $_SESSION['SERVICE_SETTINGS']['elastic_address_id']; ?>'				 
			},
			success:function(jso)
			{
				if (typeof(jso.floatingip) != 'undefined')
				{
					$('#NetworkSummaryTable > tbody').append('<tr><td width="200px">Floating IP</td><td>'+jso.floatingip.floating_ip_address+'</td></tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Query DataMode Instance */
	function QueryDataModeInstance(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['datamode_instance']; ?>'		
			},
			success:function(jso)
			{
				fixed_addr = 'Not Applicable';
				floating_addr = 'Not Applicable';
				$.each(jso.server.addresses, function(addr_key,addr_info)
				{
					$.each(addr_info, function(addr_index,addr_value)
					{
						if (addr_value['OS-EXT-IPS:type'] == 'fixed')
						{
							fixed_addr = addr_value.addr;							
						}
					
						if (addr_value['OS-EXT-IPS:type'] == 'floating')
						{
							floating_addr = addr_value.addr;
						}
					});				
				});
			
				$('#DataModeInstance > tbody').append(
					'<tr><th colspan="2"><?php echo _("DataMode Instance Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Instance Name"); ?></td>		<td>'+jso.server.name+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Instance Id"); ?></td>			<td>'+jso.server.id+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Availability Zone"); ?></td>	<td>'+jso.server["OS-EXT-SRV-ATTR:host"]+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Private IP Address"); ?></td>	<td>'+fixed_addr+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Floating IP Address"); ?></td>	<td>'+floating_addr+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Power Status"); ?></td>		<td>'+jso.server.status+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Security Groups"); ?></td>		<td>'+jso.server.security_groups[0].name+'</td></tr>\
				');
				/*
				if (typeof jso[0].Instances[0].Tags != 'undefined')
				{
					$.each(jso[0].Instances[0].Tags, function(tag_key, tag_value)
					{
						if (tag_value.Key == 'Name' && tag_value.Value != '')
						{
							$('#DataModeInstance > tbody').append('<tr><td width="200px"><?php echo _("Instance Name"); ?></td>	<td>'+tag_value.Value+'</td></tr>');
						}
					});
				}*/
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Begin To Run Recover Service */
	function BeginToRunServiceAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'          :'BeginToRunServiceAsync',
				 'ACCT_UUID'       :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID'       :'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'REPL_UUID'       :'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'RECY_TYPE'	   :'<?php echo $_SESSION['RECY_TYPE']; ?>',
				 'PLAN_UUID'	   :'<?php echo $PLAN_UUID; ?>',
				 'SERVICE_SETTINGS':'<?php echo json_encode($_SESSION['SERVICE_SETTINGS']); ?>',
				 'TRIGGER_SYNC'   :$('#TRIGGER_SYNC:checkbox:checked').val()
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
		
		$("#BackToInstanceConfiguration").click(function(){
			window.location.href = "<?php echo $BACK_TO; ?>";
		})
		
		$("#ServiceSubmitAndRun").one("click" ,function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');			
			$('#BackToMgmtRecoverWorkload').prop('disabled', true);
			$('#BackToInstanceConfiguration').prop('disabled', true);
			//$('#ServiceSubmitAndRun').prop('disabled', true);
			//$('#ServiceSubmitAndRun').removeClass('btn-primary').addClass('btn-default');
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
					<li style='width:21%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:21%'><a>				<?php echo _("Step 3 - Select Disk / Snapshot"); ?></a></li>
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
				
				<table id="DataModeInstance">
					<tbody></tbody>
				</table>
				
				<table id="FlavorSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="NetworkSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="SecurityGroupSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="SnapshotSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="RecoveryScript">
					<tbody></tbody>
				</table>					
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToInstanceConfiguration' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>	
					<button id='BackToMgmtRecoverWorkload' 		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='ServiceSubmitAndRun' 			class='btn btn-default pull-right btn-lg' disabled><?php echo _("Run"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>