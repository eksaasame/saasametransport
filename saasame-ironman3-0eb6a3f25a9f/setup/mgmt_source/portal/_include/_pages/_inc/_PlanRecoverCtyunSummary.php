<?php
if (isset($_SESSION['EDIT_PLAN']))
{
	$PLAN_UUID				= $_SESSION['EDIT_PLAN'] -> PlanUUID;
	$_SESSION['SERV_REGN']	= $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$_SESSION['REPL_UUID']  = $_SESSION['EDIT_PLAN'] -> ReplUUID;
	$_SESSION['CLOUD_TYPE'] = $_SESSION['EDIT_PLAN'] -> CloudType;
	$_SESSION['VENDOR_NAME']= $_SESSION['EDIT_PLAN'] -> VendorName;
	$_SESSION['SERV_UUID']	= $_SESSION['EDIT_PLAN'] -> TransportUUID;
}
else
{
	$PLAN_UUID = 'xyzzy';
}

if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM'){$RECY_TYPE = 'Planned Migration';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DR'){$RECY_TYPE = 'Disaster Recovery';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DT'){$RECY_TYPE = 'Development Testing';}
if ($_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'] == ''){$PRE_FILE_NAME = 'Not Applicable';}else{$PRE_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'];}
if ($_SESSION['SERVICE_SETTINGS']['rcvy_post_script'] == ''){$POST_FILE_NAME = 'Not Applicable';}else{$POST_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_post_script'];}
if (!isset($_SESSION['SERVICE_SETTINGS']['is_datamode_boot'])){$IS_DATAMODE_BOOTABLE = false; $DATAMODE_BOOTABLE = 'Data Only';}else{$IS_DATAMODE_BOOTABLE = true; $DATAMODE_BOOTABLE = 'Yes';}
?>
<script>
$( document ).ready(function() {
	<!-- Exec -->
	QueryReplicaInformation();
	
	/* Determine URL Segment */
	DetermineSegment('EditPlanRecoverCtyunSummary');
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
		$('#BackToLastPage').prop('disabled', false);
		$('#CancelToMgmt').prop('disabled', false);
		
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
				var RECY_TYPE = '<?php echo $RECY_TYPE; ?>';			
				
				$('#ReplicaSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Host Information"); ?></th></tr>\
					 <tr><td width="165px"><?php echo _("Recover Type"); ?></td>	<td>'+RECY_TYPE+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Host Name"); ?></td>		<td>'+jso.HOST_NAME+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Host Address"); ?></td>	<td>'+jso.HOST_ADDR+'</td></tr>\
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
					QuerySubnetInformation();
					QuerySecurityGroupInformation();
					
					$('#RecoveryScript > tbody').append(
						'<tr><th colspan="2"><?php echo _("Recovery Script"); ?></th></tr>\
						<tr><td width="165px"><?php echo _("Recovery Pre Script"); ?></td>	<td><?php echo $PRE_FILE_NAME; ?></td></tr>\
						<tr><td width="165px"><?php echo _("Recovery Post Script"); ?></td>	<td><?php echo $POST_FILE_NAME; ?></td></tr>\
					');					
				}
				else
				{
					QueryDataModeInstance();
				}
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
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    	:'QueryFlavorInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'FLAVOR_ID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['flavor_id']; ?>'				 
			},
			success:function(jso)
			{
				$('#FlavorSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Flavor Information"); ?></th></tr>\
					 <tr><td width="165px"><?php echo _("Name"); ?></td>	 <td>'+jso.flavorName+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Type"); ?></td>	 <td>'+jso.name+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Number of CPU"); ?></td><td>'+jso.cpuNum+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Memory"); ?></td>		 <td>'+jso.memSize+'GB</td></tr>\
				');
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Query Network Configure */
	function QuerySubnetInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    	:'QuerySubnetInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SUBNET_UUID'  :'<?php echo $_SESSION['SERVICE_SETTINGS']['network_uuid']; ?>'				 
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					if (value.resVlanId == '<?php echo $_SESSION['SERVICE_SETTINGS']['subnet_uuid']; ?>')
					{
						NETWORK_NAME 	= value.name;
						NETWORK_CIDR 	= value.cidr;
						NETWORK_REGION 	= value.regionId;
					}
				});
				
				$('#NetworkSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Network Information"); ?></th></tr>\
					 <tr><td width="165px"><?php echo _("Name"); ?></td>			<td>'+NETWORK_NAME+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Private Address"); ?></td>	<td><?php echo $_SESSION['SERVICE_SETTINGS']['private_address_id']; ?></td></tr>\
					 <tr><td width="165px"><?php echo _("CIDR"); ?></td>			<td>'+NETWORK_CIDR+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Region"); ?></td>			<td>'+NETWORK_REGION+'</td></tr>\
					');
				
				QueryPublicAddresses();				
			},
			error: function(xhr)
			{
				
			}
		});	
	}
		
	/* Query Public IP Information */
	function QueryPublicAddresses(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    		:'QueryPublicAddresses',
				 'CLUSTER_UUID' 	:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'PUBLIC_ADDR_ID'  	:'<?php echo $_SESSION['SERVICE_SETTINGS']['elastic_address_id']; ?>'				 
			},
			success:function(jso)
			{
				if (typeof(jso.publicip) != 'undefined')
				{
					$('#NetworkSummaryTable > tbody').append('<tr><td width="165px">Public IP</td><td>'+jso.publicip.public_ip_address+'</td></tr>');
				}
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
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    	:'QuerySecurityGroupInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SGROUP_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['sgroup_uuid']; ?>'
			},
			success:function(jso)
			{
				$('#SecurityGroupSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Security Group Information"); ?></th></tr>\
					 <tr><td width="165px"><?php echo _("Name"); ?></td> <td>'+jso.name+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Region"); ?></td> <td>'+jso.zoneName+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Zone Id"); ?></td> <td>'+jso.zoneId+'</td></tr>\
					');	
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
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_REGN'  	:'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'HOST_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['datamode_instance']; ?>'		
			},
			success:function(jso)
			{
				//server_info = jso.servers != 'undefined' ? jso.servers[0]:jso.server;
				
				$.each(jso.server.addresses[Object.keys(jso.server.addresses)[0]], function(addr_key,addr_value)
				{
					if (addr_value['OS-EXT-IPS:type'] == 'fixed')
					{
						fixed_addr = addr_value.addr;
					}
													
					if (addr_value['OS-EXT-IPS:type'] == 'floating')
					{
						floating_addr = addr_value.addr;
					}
					else
					{
						floating_addr = 'None';
					}
				});				
				
				$('#DataModeInstance > tbody').append(
					'<tr><th colspan="2"><?php echo _("DataMode Instance Information"); ?></th></tr>\
					 <tr><td width="165px"><?php echo _("Instance Name"); ?></td>		<td>'+jso.server.name+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Instance Id"); ?></td>			<td>'+jso.server.id+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Private IP Address"); ?></td>	<td>'+fixed_addr+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Public IP Address"); ?></td>	<td>'+floating_addr+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Power Status"); ?></td>		<td>'+jso.server.status+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Security Groups"); ?></td>		<td>'+jso.server.security_groups[0].name+'</td></tr>\
					 <tr><td width="165px"><?php echo _("Data Mode Boot Disk"); ?></td>	<td><?php echo $DATAMODE_BOOTABLE; ?></td></tr>\
				');
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	/* Begin To Save Recover Plan */
	function SaveRecoverPlan(){
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
	function UpdateRecoverPlan(){
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
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverPlan";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = urlPrefix+"PlanInstanceCtyunConfigurations";
		})		
		
		$("#SaveRecoverPlan").one("click" ,function(){
			RECOVER_PLAN = {
							'ReplUUID'		 	:'<?php echo $_SESSION['REPL_UUID']; ?>',
							'TransportUUID'		:'<?php echo $_SESSION['SERV_UUID']; ?>',
							'ServiceRegin'   	:'<?php echo $_SESSION['SERV_REGN']; ?>',
							'CloudType'		 	:'<?php echo $_SESSION['CLOUD_TYPE']; ?>',
							'VendorName'	 	:'<?php echo $_SESSION['VENDOR_NAME']; ?>',
							'RecoverType'	 	:'<?php echo $_SESSION['RECY_TYPE']; ?>',
							'ClusterUUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
							'flavor_id'			:'<?php echo $_SESSION['SERVICE_SETTINGS']['flavor_id']; ?>',
							'network_uuid'		:'<?php echo $_SESSION['SERVICE_SETTINGS']['network_uuid']; ?>',
							'sgroup_uuid'		:'<?php echo $_SESSION['SERVICE_SETTINGS']['sgroup_uuid']; ?>',
							'elastic_address_id':'<?php echo $_SESSION['SERVICE_SETTINGS']['elastic_address_id']; ?>',	
							'private_address_id':'<?php echo $_SESSION['SERVICE_SETTINGS']['private_address_id']; ?>',
							'rcvy_pre_script'	:'<?php echo $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']; ?>',
							'rcvy_post_script'	:'<?php echo $_SESSION['SERVICE_SETTINGS']['rcvy_post_script']; ?>',
							'hostname_tag'		:'<?php echo $_SESSION['SERVICE_SETTINGS']['hostname_tag']; ?>',
							'volume_uuid'		:'<?php echo $_SESSION['VOLUME_UUID']; ?>',
							'HostType'			:'<?php echo $_SESSION['HOST_TYPE']; ?>',
							'OSType'			:'<?php echo $_SESSION['REPL_OS_TYPE']; ?>',
							'datamode_instance' :'<?php echo $_SESSION['SERVICE_SETTINGS']['datamode_instance']; ?>',	/* NEW */
							'is_datamode_boot'  :'<?php echo $IS_DATAMODE_BOOTABLE; ?>'									/* NEW */
						};
						
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');			
			$('#CancelToMgmt').prop('disabled', true);
			$('#BackToLastPage').prop('disabled', true);
			$('#SaveRecoverPlan').prop('disabled', true);
			$('#SaveRecoverPlan').removeClass('btn-primary').addClass('btn-default');
			
			if (urlPrefix == '')
			{
				SaveRecoverPlan();
			}
			else
			{
				UpdateRecoverPlan();
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
					<button id='BackToLastPage' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='CancelToMgmt' 		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SaveRecoverPlan' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Save"); ?></button>	
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>