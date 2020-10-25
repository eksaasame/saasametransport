<?php 
if (isset($_SESSION['EDIT_PLAN']))
{
	$PLAN_UUID				= $_SESSION['EDIT_PLAN'] -> PlanUUID;
	$_SESSION['SERV_REGN']	= $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$_SESSION['REPL_UUID']  = $_SESSION['EDIT_PLAN'] -> ReplUUID;
	$_SESSION['CLOUD_TYPE'] = $_SESSION['EDIT_PLAN'] -> CloudType;
	$_SESSION['VENDOR_NAME']= $_SESSION['EDIT_PLAN'] -> VendorName;
}
else
{
	$PLAN_UUID = 'xyzzy';
}

if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM'){$RECY_TYPE = 'Planned Migration';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DR'){$RECY_TYPE = 'Disaster Recovery';}elseif($_SESSION['RECY_TYPE'] == 'RECOVERY_DT'){$RECY_TYPE = 'Development Testing';}
if ($_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'] == ''){$PRE_FILE_NAME = 'Not Applicable';}else{$PRE_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script'];}
if ($_SESSION['SERVICE_SETTINGS']['rcvy_post_script'] == ''){$POST_FILE_NAME = 'Not Applicable';}else{$POST_FILE_NAME = $_SESSION['SERVICE_SETTINGS']['rcvy_post_script'];}
?>
<script>
	<!-- Exec -->
	QueryReplicaInformation();
	
	DetermineSegment('EditPlanRecoverAliyunSummary');
	
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
		},1500);
	});
	
	function QueryDataModeInstance(){

		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_REGN'	:'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'SERV_UUID'	:'<?php echo $_SESSION['SERVER_UUID']; ?>',
				 'HOST_ID':DATAMODE_INSTANCE
			},
			success:function(jso)
			{
				$('#DataModeInstance > tbody').append(
					'<tr><th colspan="2"><?php echo _("DataMode Instance Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Instance Name"); ?></td>		<td>'+jso.InstanceName+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Location"); ?></td>	<td>'+jso.Region+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Private IP Address"); ?></td>	<td>'+jso.PrivateIpAddress+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Public IP Address"); ?></td>		<td>'+jso.PublicIpAddress+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Security Groups"); ?></td>		<td>'+jso.SecurityGroup+'</td></tr>\
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
					 <tr><td width="200px"><?php echo _("Used Public IP"); ?></td>	<td><?php echo $_SESSION['SERVICE_SETTINGS']['elastic_address_id']; ?></td></tr>\
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
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'    	:'QueryFlavorInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'FLAVOR_ID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['flavor_id']; ?>'				 
			},
			success:function(jso)
			{
				$('#FlavorSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("ECS Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Instance Type Name"); ?></td>	<td>'+jso.InstanceTypeId+'</td></tr>\
					 <tr><td width="200px"><?php echo _("CPU"); ?></td>				<td>'+jso.CpuCoreCount+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Memory"); ?></td>			<td>'+jso.MemorySize+'GB'+'</td></tr>\
					 <tr><td width="200px"><?php echo _("GPU"); ?></td>				<td>'+jso.GPUAmount+'</td></tr>\
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
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'    	:'QueryNetworkInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'NETWORK_UUID' :'<?php echo $_SESSION['SERVICE_SETTINGS']['network_uuid']; ?>'				 
			},
			success:function(jso)
			{

				$('#NetworkSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Network Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Vpc Name"); ?></td>					<td>'+jso[0].VpcName+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Cidr Block"); ?></td>					<td>'+jso[0].CidrBlock+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Switch"); ?></td>						<td>'+<?php echo '"'.$_SESSION['SERVICE_SETTINGS']['switch_uuid'].'"'; ?>+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Status"); ?></td>						<td>'+jso[0].Status+'</td></tr>\
					');
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
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'    	:'QuerySecurityGroupInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'SGROUP_UUID' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['sgroup_uuid']; ?>'		
			},
			success:function(jso)
			{
				if( jso.length == 0 )
					return ;
				
				$('#SecurityGroupSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Security Group Information"); ?></th></tr>\
					 <tr><td width="200px"><?php echo _("Security Group Name"); ?></td>	<td>'+jso[0].SecurityGroupName+'</td></tr>\
					 <tr><td width="200px"><?php echo _("Create Time"); ?></td>	<td>'+jso[0].CreationTime+'</td></tr>\
					');
			},
			error: function(xhr)
			{
				
			}
		});	
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
			window.location.href = urlPrefix+"PlanInstanceAliyunConfigurations";
		})
		
		$("#SaveRecoverPlan").one("click" ,function(){
			
			var RECOVER_PLAN = {
				'ReplUUID'		 :'<?php echo $_SESSION['REPL_UUID']; ?>',
				'ServiceRegin'   :'<?php echo $_SESSION['SERV_REGN']; ?>',
				'CloudType'		 :'<?php echo $_SESSION['CLOUD_TYPE']; ?>',
				'VendorName'	 :'<?php echo $_SESSION['VENDOR_NAME']; ?>',
				'RecoverType'	 :'<?php echo $_SESSION['RECY_TYPE']; ?>',
				'ClusterUUID'	 :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				'HostType'		 :'<?php echo $_SESSION['REPL_OS_TYPE']; ?>',
				'SERVER_UUID'	 :'<?php echo $_SESSION['SERVER_UUID']; ?>',				
				'flavor_id'		 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['flavor_id']; ?>',
				'network_uuid'	 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['network_uuid']; ?>',
				'sgroup_uuid'	 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['sgroup_uuid']; ?>',
				'elastic_address_id':'<?php echo $_SESSION['SERVICE_SETTINGS']['elastic_address_id']; ?>',
				'private_address_id':'<?php echo $_SESSION['SERVICE_SETTINGS']['private_address_id']; ?>',
				'switch_uuid'		:'<?php echo $_SESSION['SERVICE_SETTINGS']['switch_uuid']; ?>',
				'rcvy_pre_script'  	:'<?php echo $_SESSION['SERVICE_SETTINGS']['rcvy_pre_script']; ?>',
				'rcvy_post_script' 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['rcvy_post_script']; ?>',
				'hostname_tag'	 	:'<?php echo $_SESSION['SERVICE_SETTINGS']['hostname_tag']; ?>',
				'datamode_instance' :'<?php echo $_SESSION['SERVICE_SETTINGS']['datamode_instance']; ?>',
				'volume_uuid'		:'<?php echo $_SESSION['VOLUME_UUID']; ?>',
				'disk_type' 		:'<?php echo $_SESSION['SERVICE_SETTINGS']['disk_type']; ?>'
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
					
				<table id="ReplicaSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="DataModeInstance">
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
					<button id='SaveRecoverPlan' 				class='btn btn-default pull-right btn-lg' disabled><?php echo _("Save"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>