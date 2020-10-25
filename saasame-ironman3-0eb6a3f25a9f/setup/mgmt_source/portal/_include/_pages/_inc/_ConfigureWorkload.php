<script>
$( document ).ready(function() {
	
	QueryTransportInformation();
		
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
				$('#REPL_DISK_MGMT').removeClass('DefaultLoading');
				$('#BackToReplicaService').prop('disabled', false);
				$('#BackToMgmtPrepareWorkload').prop('disabled', false);
	
				if (jso != false)
				{
					$('#REPL_DISK_MGMT').append("<label for='comment'><?php echo _("Disk Selection:"); ?></label><br>");
					
					type_of_packer = jso.HOST_TYPE;
					
					DISK_SELECT = '';
					CLOUD_DISK_SELECT = '';
					DISK_COUNT = 0;
					auto_map_disk = [];

					$.each(jso.HOST_DISK, function(key,value)
					{
						if (type_of_packer == 'Physical' && option_level == "USER")
						{
							if (jso.HOST_INFO.disk_infos[key]['is_boot'] == true)
							{
								skip_disabled = ' disabled';
							}
							else
							{
								skip_disabled = '';
							}
						}
						else
						{
							skip_disabled = '';
							$('#PRE_SNAPSHOT_SCRIPT').prop('disabled', true).attr("placeholder", "<?php echo _("Not applicable for Virtual packer"); ?>").blur();
							$('#POST_SNAPSHOT_SCRIPT').prop('disabled', true).attr("placeholder", "<?php echo _("Not applicable for Virtual packer"); ?>").blur();
						}
						
						REPL_DISK_SIZE = value['DISK_SIZE'] / 1024 / 1024 / 1024;
					
						if (VENDOR_NAME == 'RecoverKit' || VENDOR_NAME == 'Ctyun' || VENDOR_NAME == 'AdvancedOnPremise')
						{
							$('#REPL_DISK_MGMT').append("<?php echo _('Disk'); ?> "+DISK_COUNT+": <select id='Disk"+key+"' name='HostDisk' class='selectpicker' data-width='70%' "+skip_disabled+" title='disk'></select>");
							$('#Disk'+key+'').append(new Option(value['DISK_NAME']+' ('+REPL_DISK_SIZE+'GB)', 'KeepThisDisk', true, true));
							$('#Disk'+key+'').append(new Option('<?php echo _("-- Do not replicate this disk --"); ?>', value['DISK_UUID'], true, false));

							$('#REPL_DISK_MGMT').append("<select id='CloudDisk"+key+"' name='CloudDisk' class='selectpicker CloudDisk' data-width='26%' title='disk'></select><br><br>");
							
							// CLOUD DISK MESSAGE
							CLOUD_DISK_MSG = VENDOR_NAME == 'RecoverKit' ? '<?php echo _("-- Please select a mapping disk --"); ?>' : '<?php echo _("-- Create this disk on demand --"); ?>';
							$('#CloudDisk'+key+'').append(new Option(CLOUD_DISK_MSG, 'CreateOnDemand', true, true));
							
							//SET UP DISK IN USE
							disk_in_use = [false];
							selected_disk_info = {};
							
							$.each(CLOUD_DISK, function(cd_key,cd_value)
							{
								if (VENDOR_NAME == 'RecoverKit')
								{
									DISK_ADDR = (cd_value['serial_number'] != "")?cd_value['serial_number']:jQuery.parseJSON(cd_value['uri']).address;
									SCSI_ADDR = jQuery.parseJSON(cd_value['uri']).address;
									DISK_SIZE = cd_value['size'] / 1024 / 1024 / 1024;
									UNIQUE_ID = cd_value['unique_id'];
							
									$('#CloudDisk'+key+'').append(new Option(SCSI_ADDR+' ('+DISK_SIZE+'GB)', DISK_ADDR, true, false));
									
									selected_disk_info[DISK_ADDR] = DISK_SIZE;
									
									/* Push Status To Array */
									disk_in_use.push(false);
								}								
								else if (VENDOR_NAME == 'AdvancedOnPremise')
								{
									if (cd_value['is_boot'] == false)
									{
										DISK_ADDR = (cd_value['serial_number'] != "")?cd_value['serial_number']:jQuery.parseJSON(cd_value['uri']).address;
										SCSI_ADDR = jQuery.parseJSON(cd_value['uri']).address;
										DISK_SIZE = cd_value['size'] / 1024 / 1024 / 1024;
										UNIQUE_ID = cd_value['unique_id'];
							
										$('#CloudDisk'+key+'').append(new Option(SCSI_ADDR+' ('+DISK_SIZE+'GB)', DISK_ADDR, true, false));
							
										/* Disabled is-use Disk */								
										if (cd_value['in_use'] == true)
										{
											$("option[value='"+DISK_ADDR+"']").attr("disabled", true);
										}
										
										selected_disk_info[DISK_ADDR] = DISK_SIZE;
										/* Push in-use Status To Array */
										disk_in_use.push(cd_value['in_use']);
									}
								}
								else
								{
									if (cd_value['status'] != 'in-use')
									{
										$('#CloudDisk'+key+'').append(new Option(cd_value['name']+' ('+cd_value['size']+'GB)', cd_value['id'], true, false));
										
										/* Push Status To Array */
										disk_in_use.push(false);
									}
								}
							});
								
							/* Automatic Map Select Same Size Pair Disks */
							var selected_map_disk = jQuery.unique($('[name="CloudDisk"] option:selected').map(function(){return this.value}).get());
							selected_disk_map = selected_map_disk.filter(function(itm, i, selected_map_disk) {return i == selected_map_disk.indexOf(itm);});
							
							var disabled_map_disk= jQuery.unique($('[name="CloudDisk"] option:disabled').map(function(){return this.value}).get());
							disabled_disk_map = disabled_map_disk.filter(function(itm, i, disabled_map_disk) {return i == disabled_map_disk.indexOf(itm);});
						
							$.each(selected_disk_info, function(map_addr,map_size)
							{
								if((jQuery.inArray(map_addr, selected_disk_map) !== 1) && (jQuery.inArray(map_addr, disabled_disk_map) !== 0) && REPL_DISK_SIZE == map_size)
								{
									$('#CloudDisk'+key+'').val(map_addr);
									return false;
								}
							});
								
							CLOUD_DISK_SELECT = CLOUD_DISK_SELECT + '#CloudDisk'+key+',';
												
							/* OnChange Create on Demand selection */
							$('#Disk'+key+'').change(function() {
								if ($(this).val() == 'KeepThisDisk')
								{
									$('#CloudDisk'+key+'').prop('disabled', false);
								}
								else
								{
									$('#CloudDisk'+key+'').prop('disabled', true);
									$('#CloudDisk'+key+'').val('CreateOnDemand');
								}
								$('.selectpicker').selectpicker('refresh');
							});
						
						}
						else
						{						
							$('#REPL_DISK_MGMT').append("<?php echo _('Disk'); ?> "+DISK_COUNT+": <select id='Disk"+key+"' class='selectpicker' data-width='96%' "+skip_disabled+" title='disk'></select><br><br>");
							$('#Disk'+key+'').append(new Option(value['DISK_NAME']+' ('+REPL_DISK_SIZE+'GB)', 'KeepThisDisk', true, true));
							$('#Disk'+key+'').append(new Option('<?php echo _("-- Do not replicate this disk --"); ?>', value['DISK_UUID'], true, false));
						}
						DISK_SELECT = DISK_SELECT + '#Disk'+key+',';
						DISK_COUNT++;
					});
					
					$('.selectpicker').selectpicker('refresh');

					/* FOR SELECTED CLOUD DISK */
					if (VENDOR_NAME == 'RecoverKit' || VENDOR_NAME == 'Ctyun' || VENDOR_NAME == 'AdvancedOnPremise')
					{
						/* Loop For all Cloud Disk Values */
						var cloud_disk = [];
						$("#CloudDisk0 option").each(function(){
							cloud_disk.push($(this).val());
						});
						
						/* Filter out empty array item */
						cloud_disk = cloud_disk.filter(function(v){return v!==''});
					
						//Disabled Selected Option In The Same Name Group
						$('[name="CloudDisk"] option:selected').each(function() {
							$('[name="CloudDisk"]').find('option[value="'+$(this).val()+'"]').attr('disabled', true);
						});
						
						$('[name="CloudDisk"]').find('option:selected').attr('disabled', false);
						$('[name="CloudDisk"]').find("option[value=CreateOnDemand]").attr('disabled', false);				
						$('.selectpicker').selectpicker('refresh');
				
						//Begin Disk Selection On Change
						$('[name="HostDisk"], [name="CloudDisk"]').change(function() {
							select_value = $(this).val();							
								
							//List all seleted values
							var selected_disk = [];
							$.each($('[name="CloudDisk"] option:selected'), function(){
								selected_disk.push($(this).val());	
							});
							
							//Filter Out in-use disk
							$.each(cloud_disk, function(cloud_disk_index,cloud_disk_value){
								$('[name="CloudDisk"]').not(this).find('option[value="'+cloud_disk_value+'"]').attr('disabled', disk_in_use[cloud_disk_index]);
							});
							
							//Disabled selected value in the same options groups
							$.each(selected_disk, function(selected_disk_index,selected_disk_value){
								if (selected_disk_value != 'CreateOnDemand'){
									$('[name="CloudDisk"]').find('option[value="'+selected_disk_value+'"]').attr('disabled', true);
								}
							});
							
							//Enable selected option to be selected
							$('[name="CloudDisk"]').find('option:selected').attr('disabled', false);
							
							$('.selectpicker').selectpicker('refresh');
						});
						CLOUD_DISK_SELECT = CLOUD_DISK_SELECT.slice(0,-1);
					}
		
					DISK_SELECT = DISK_SELECT.slice(0,-1);
					
					$('#ConfigureSchedule').prop('disabled', false);
					$('#ConfigureSchedule').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
				
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
		
	/* Query Select Transport Information*/
	function QueryTransportInformation()
	{
		option_level = '<?php echo $_SESSION['optionlevel']; ?>';
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'QueryTransportServerInformation',
				 'SERV_UUID' :'<?php echo $_SESSION['LAUN_UUID']; ?>',
				 'ON_THE_FLY':(option_level == 'USER')?false:true
			},
			success:function(jso)
			{
				CLOUD_UUID = jso.OPEN_UUID;
				VENDOR_NAME = jso.VENDOR_NAME;
				CLOUD_DISK_NUM = 999;
				
				if (VENDOR_NAME == 'Ctyun')
				{
					DescribeVolumes();
				}
				else if (jso.SERV_INFO.is_winpe == true)
				{
					VENDOR_NAME = 'RecoverKit';
					CLOUD_DISK = jso.SERV_INFO.disk_infos;
					CLOUD_DISK_NUM = CLOUD_DISK.length;
					QueryHostInformation();
				}
				else if (VENDOR_NAME == 'UnknownVendorType' && option_level != 'USER')
				{
					VENDOR_NAME = 'AdvancedOnPremise';
					CLOUD_DISK = jso.SERV_INFO.disk_infos;
					CLOUD_DISK_NUM = CLOUD_DISK.length - 1;
					QueryHostInformation();
				}
				else
				{
					QueryHostInformation();
				}					
			}
		});
	}
	
	/* Describe CTyun Volumes*/
	function DescribeVolumes()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'	  :'DescribeVolumes',
				 'CLOUD_UUID' :CLOUD_UUID	 
			},
			success:function(jso)
			{
				CLOUD_DISK = jso;
				QueryHostInformation();
			}
		});
		
		
	}
	
	/* Workload Configuration */
	function WorkloadConfiguration(){
		/* Format Select Disk UUID */
		DISK_UUID = '';
		KEEP_COUNT = 0;
		$(DISK_SELECT).each(function(){
			DISK_UUID = DISK_UUID + $(this).val()+',';
			if ($(this).val() == 'KeepThisDisk')
			{
				KEEP_COUNT++;
			}
		});
		DISK_UUID = DISK_UUID.slice(0,-1);
	
		/* Format Cloud Disk Mapping UUID */
		CLOUD_DISK_UUID = '';
		DEMAND_COUNT = 0;
		if (VENDOR_NAME == 'RecoverKit' || VENDOR_NAME == 'Ctyun' || VENDOR_NAME == 'AdvancedOnPremise')
		{			
			$(CLOUD_DISK_SELECT).each(function(){
				CLOUD_DISK_UUID = CLOUD_DISK_UUID + $(this).val()+',';		
				if ($(this).val() != 'CreateOnDemand')
				{
					DEMAND_COUNT++;
				}			
			});
			CLOUD_DISK_UUID = CLOUD_DISK_UUID.slice(0,-1);
		}
		
		//console.log(DEMAND_COUNT);
		//console.log(KEEP_COUNT);
		//console.log(CLOUD_DISK_NUM);
		
		/* Select at least one disk check logic */
		if (KEEP_COUNT == 0)
		{
			BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				message: '<?php echo _("Need at least one replication disk."); ?>',
				type: BootstrapDialog.TYPE_PRIMARY,
				draggable: true,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}],
			});
			return;		
		}

		/* For Cloud Disk Mapping Need to */
		if (KEEP_COUNT > CLOUD_DISK_NUM && DEMAND_COUNT != 0)
		{ 
			BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				message: '<?php echo _("Recovery Kit has fewer disks."); ?>',
				type: BootstrapDialog.TYPE_PRIMARY,
				draggable: true,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}],
			});
			return;
		}
		
		/* RecoverKit needs to map all the cloud disk(s) */
		if (VENDOR_NAME == 'RecoverKit' && DEMAND_COUNT != KEEP_COUNT)
		{
			BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				message: '<?php echo _("Please map all selected replica disks to Recovery Kit."); ?>',
				type: BootstrapDialog.TYPE_PRIMARY,
				draggable: true,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}],
			});
			return;
		}
		
		/* RecoverKit needs to map all the cloud disk(s) */
		if (VENDOR_NAME == 'AdvancedOnPremise' && (DEMAND_COUNT != 0 && DEMAND_COUNT != KEEP_COUNT))
		{
			BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				message: '<?php echo _("Please map all selected replica disks to Transport Server."); ?>',
				type: BootstrapDialog.TYPE_PRIMARY,
				draggable: true,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}],
			});
			return;
		}
				
		/* Save Workload Disk Configuration To Session */	
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    			:'WorkloadConfiguration',
				 'SKIP_DISK' 			:DISK_UUID,
				 'CLOUD_MAP_DISK'		:CLOUD_DISK_UUID,
				 'PRE_SNAPSHOT_SCRIPT' 	:$("#PRE_SNAPSHOT_SCRIPT").val(),
				 'POST_SNAPSHOT_SCRIPT' :$("#POST_SNAPSHOT_SCRIPT").val()
			},
			success:function(jso)
			{
				window.location.href = "ConfigureSchedule";
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	
	/* Submit Trigger */
	$(function(){
		$("#BackToMgmtPrepareWorkload").click(function(){
			window.location.href = "MgmtPrepareWorkload";
		})
		
		$("#BackToReplicaService").click(function(){
			window.location.href = "SelectTargetTransportServer";
		})
		
		$("#ConfigureSchedule").click(function(){			
			WorkloadConfiguration();
		})
	});
});
</script>
<style type="text/css">
.pre_script_row{
	width:50%;
	float:left;
}

.post_script_row{
	width:50%;
	padding-left:18px;
	float:left;
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
						<li style='width:20%' class='active'><a><?php echo _("Step 3 - Configure Host"); ?></a></li>
						<li style='width:20%'><a>				<?php echo _("Step 4 - Configure Replication"); ?></a></li>
						<li style='width:20%'><a>		 		<?php echo _("Step 5 - Replication Summary"); ?></a></li>
					</ul>
				</div>
		
				<div id='form_block_wizard'>
				
					<div id='REPL_DISK_MGMT' class='form-group form-inline DefaultLoading'>&nbsp;</div>
					
					<div class="pre_script_row">					
						<label for='comment'><?php echo _("Pre Snapshot Script:"); ?></label>
						<textarea class='form-control' rows="4" cols="50" style="overflow:hidden; resize:none;" id="PRE_SNAPSHOT_SCRIPT" placeholder="<?php echo _("Input Command Here"); ?>"></textarea>
					</div>
					
					<div class="post_script_row">
						<label for='comment'><?php echo _("Post Snapshot Script:"); ?></label>
						<textarea class='form-control' rows="4" cols="50" style="overflow:hidden; resize:none;" id="POST_SNAPSHOT_SCRIPT" placeholder="<?php echo _("Input Command Here"); ?>"></textarea>
					</div>
				</div>
				
				<div id='title_block_wizard'>			
					<div class='btn-toolbar'>
						<button id='BackToReplicaService' 		class='btn btn-default pull-left  btn-lg' disabled><?php echo _("Back"); ?></button>
						<button id='BackToMgmtPrepareWorkload' 	class='btn btn-default pull-left  btn-lg' disabled><?php echo _("Cancel"); ?></button>						
						<button id='ConfigureSchedule' 			class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>						
					</div>
				</div>
			</div> <!-- id: page -->
	</div> <!-- id: wrapper_block-->
</div>
