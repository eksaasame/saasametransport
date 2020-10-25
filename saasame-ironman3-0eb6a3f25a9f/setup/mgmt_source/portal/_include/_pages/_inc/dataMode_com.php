<script>
class dataMode{
	constructor( ){
	}
	
	ListDataModeInstances( url ){
		

		this.instance = '<?php echo isset($_SESSION["EDIT_PLAN"]->datamode_instance)?$_SESSION["EDIT_PLAN"]->datamode_instance:"" ?>';
		
		var parent = this;
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/'+url,
			data:{
				 'ACTION':'ListDataModeInstances',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'FILTER_UUID':'<?php echo $_SESSION['SERVER_UUID']; ?>'
			},
			success:function(jso)
			{
				parent.HOST_TYPE = '<?php echo isset($_SESSION['HOST_TYPE'])?$_SESSION['HOST_TYPE']:null; ?>';
				
				$('#DATAMODE_INSTANCE').append(new Option('<?php echo _('No Associated'); ?>', 'NoAssociatedDataModeInstance', true, false));	
				
				if (jso != false)
				{				
					$.each(jso, function(key,value)
					{
						$('#DATAMODE_INSTANCE').append(new Option(value.InstanceName, value.InstanceId, true, false));						
					});						
				}
				
				$('.selectpicker').selectpicker('refresh');
				
				$('#DATAMODE_INSTANCE').val(parent.instance);
				
				$('#BackToServiceList').prop('disabled', false);
				$('#BackToMgmtRecoverWorkload').prop('disabled', false);
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	SetBootCheckBox(){
		if ($("#DATAMODE_INSTANCE").val() == 'NoAssociatedDataModeInstance')
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', false);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', false);
			
			$('#MgmtRecoveryScript').prop('disabled', false);
			$('#MgmtRecoveryScript').removeClass('btn-default').addClass('btn-success');
			
			$('#ConfigureDataModeAgent').prop('disabled', true);
			$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
				
			$('#FLAVOR_LIST').prop('disabled', false);
			$('#SECURITY_GROUP').prop('disabled', false);
			$('#HOSTNAME_TAG').prop('disabled', false);
			
			$('#SUBNET_LIST').prop('disabled', false);
			$('#PRIVATE_ADDR').prop('disabled', false);
			$('#FLOATING_ADDR').prop('disabled', false);

			$('#Availability_Set').prop('disabled', false);
			$('#Disk_Type').prop('disabled', false);
			$('#PUBLIC_IP').prop('disabled', false);
			$('#NETWORK_LIST').prop('disabled', false);
			$('#SPECIFIED_PRIVATE_IP').prop('disabled', false);		
			$('#SWITCH').prop('disabled', false);		

			$('#ConfigureDataModeAgent').prop('disabled', true);
			$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
			
			$('#DATAMODE_POWER').bootstrapToggle('enable');
			$('#DATAMODE_POWER').bootstrapToggle('off');
			$("#DATAMODE_POWER").bootstrapToggle('disable');
		}
		else
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', true);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', true);
			
			$('#MgmtRecoveryScript').prop('disabled', true);
			$('#MgmtRecoveryScript').removeClass('btn-success').addClass('btn-default');
			
			$('#ConfigureDataModeAgent').prop('disabled', false);
			$('#ConfigureDataModeAgent').removeClass('btn-default').addClass('btn-warning');
			
			$('#FLAVOR_LIST').prop('disabled', true);
			$('#SECURITY_GROUP').prop('disabled', true);
			$('#HOSTNAME_TAG').prop('disabled', true);
			
			$('#SUBNET_LIST').prop('disabled', true);
			$('#PRIVATE_ADDR').prop('disabled', true);
			$('#FLOATING_ADDR').prop('disabled', true);	

			$('#Availability_Set').prop('disabled', true);
			$('#Disk_Type').prop('disabled', true);
			$('#PUBLIC_IP').prop('disabled', true);
			$('#NETWORK_LIST').prop('disabled', true);
			$('#SPECIFIED_PRIVATE_IP').prop('disabled', true);	
			$('#SWITCH').prop('disabled', true);
			
			if (this.HOST_TYPE == 'Physical')
			{
				$('#ConfigureDataModeAgent').prop('disabled', true);
				$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
			}
			
			$("#DATAMODE_POWER").bootstrapToggle('enable');
			$('#DATAMODE_POWER').bootstrapToggle('on');
		}
		$('.selectpicker').selectpicker('refresh');
	}
	
	ConfigureDataModeAgent(){

		$.ajax({
			url: '_include/_exec/mgmt_service.php',
			type:"POST",
			dataType:'TEXT',
			data:{
				 'ACTION'		:'ConfigureDataModeAgent',
				 'REPL_UUID'	:'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'DISK_FILTER'	:'<?php echo isset($_SESSION['SNAP_UUID'])?$_SESSION['SNAP_UUID']:$_SESSION['VOLUME_UUID']; ?>',
				 'RECOVERY_MODE':'<?php echo $_SESSION['RECY_TYPE']; ?>'
			},   
			success:function(data)
			{
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _('Data Mode'); ?>',
						cssClass: 'partition-agent-dialog',
						message: data,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: true,
						onhidden: function(dialogRef){							
							$('#ConfigureDataModeAgent').removeClass("btn-default").addClass("btn-warning")
							$('#ConfigureDataModeAgent').prop('disabled', false);
						},
						onshown: function(dialogRef){
							$('#ConfigureDataModeAgent').removeClass("btn-warning").addClass("btn-default")
							$('#ConfigureDataModeAgent').prop('disabled', true);
						},
						buttons:
						[
							{
								id: 'AgentCfgBtn',
								label: '<?php echo _('Download'); ?>',
								cssClass: 'btn-default',
								action: function(dialogRef)
								{
									id = 0;
									var PartitionArray = new Array();
									$("#MappingTable").find("tbody > tr").each(function(){
										PartitionInfo = {guid:$("#guid"+id).text(),
														 is_system_disk:$("#is_system_disk"+id).text(),
														 partition_number:$("#partition_number"+id).text(),
														 partition_offset:$("#partition_offset"+id).text(),
														 partition_size:$("#partition_size"+id).text(), 
														 partition_style:$("#partition_style"+id).text(),
														 signature:$("#signature"+id).text(),
														 path:$("#set_letter"+id).val()};
										
										PartitionArray[id] = PartitionInfo;
										id++;
									});
									
									PartitionArray = {partitions:PartitionArray}
									//console.log(PartitionArray);
									PARTITION_REF = JSON.stringify(PartitionArray);
									GenerateDataModeAgentPackage();
									dialogRef.close();
									
								}
							}
						]				
					});
				}, 0);
			}
		});
	}
}

</script>