<script>
	<!-- Begin to Automatic Exec -->
	ListInstalledInstances();
		
	/* List Installed Instance */
	function ListInstalledInstances(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'    	:'ListInstalledInstances',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				$('#LoadingTable').remove();
				if (jso != false)
				{
					count = 0;
					$.each(jso, function(Region,value)
					{
						$.each(value, function(inst_key,inst_value)
						{
							/* Define Instance ID */
							InstanceId = inst_value.InstanceId;
														
							/* Define Instance Name */
							Name = 'undefined';							
							if (typeof inst_value.Tags != 'undefined')
							{
								Tags = inst_value['Tags'];
								$.each(Tags, function(tags_key,tag_value)
								{
									if (tag_value.Key == 'Name')
									{
										Name = tag_value.Value;
									}
								});								
							}
					
							/* Define Instance Public Address */
							PublicIpAddress  = inst_value.PublicIpAddress;
							if( typeof PublicIpAddress === 'undefined' )
							{
								PublicIpAddress = 'None';
							}
							
							/* Define Instance Private Address */
							PrivateIpAddress = inst_value.PrivateIpAddress;
							InstanceType	 = inst_value.InstanceType;
							PowerState 		 = inst_value.State.Name;
							if (PowerState == 'running')
							{
								SetSelect = '';
							}
							else
							{
								SetSelect = 'disabled'
							}							
							
							/* Define Instance Availability Zone */
							AvailabilityZone = inst_value['Placement']['AvailabilityZone'];
							$('#InstanceTable > tbody').append
							(
								'<tr>\
									<td width="60px" class="TextCenter"><input type="radio" id="HOST_UUID" name="HOST_UUID" value="'+InstanceId+'|'+AvailabilityZone+'" '+SetSelect+'/></td>\
									<td width="260px"><div class="InstanceNameOverFlow">'+Name+'</div></td>\
									<td width="240px">'+PublicIpAddress+'</td>\
									<td width="240px">'+PrivateIpAddress+'</td>\
									<td width="130px">'+InstanceType+'</td>\
									<td width="130px">'+PowerState+'</td>\
									<td width="140px">'+AvailabilityZone+'</td>\
								</tr>'
							);
							count++;
						});						
					});
					
					if (count > 10)
					{
						$(document).ready(function()
						{
							$('#InstanceTable').DataTable(
							{
								paging		: true,
								Info		: false,
								bFilter		: true,
								lengthChange: false,
								pageLength	: 10,
								pagingType	: "simple_numbers",
								order: [],
								columnDefs: [ {
									targets	: 'TextCenter',
									orderable: false
								}],
								ordering	: true,
								language	: {
									search: '_INPUT_'
								}
							});
							
							$('div.dataTables_filter input').attr("placeholder",$.parseHTML("&#xF002; Search")[0].data);
							$('div.dataTables_filter input').addClass("form-control input-sm");
							$('div.dataTables_filter input').css("max-width", "180px");
							$('div.dataTables_filter input').css("padding-right", "80px");								
						});
					}
					
					$('#SelectAndNext').prop('disabled', false);	
					$('#SelectAndNext').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#InstanceTable > tbody').append(
						'<tr>\
							<td class="TextCenter">-</td>\
							<td colspan="7">No Server found on selected connection.</td>\
						</tr>');
				}
				
				$('#BackToLastPage').prop('disabled', false);	
				$('#CancelToMgmt').prop('disabled', false);
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
	/* Selected Instance */
	function SelectedInstance(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'SelectedInstance', 
				 'HOST_UUID' 	:$("#InstanceTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.code == true)
				{
					window.location.href = "VerifyAwsLinuxLauncherService";					
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
						}}],
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Submit Trigger */
	$(function(){
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtTransportConnection";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = "ServLinuxConnection";
		})
		
		$("#SelectAndNext").click(function(){
			SelectedInstance();
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-train fa-fw"></i>&nbsp;<?php echo _("Transport"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:25%'><a>				<?php echo _("Step 1 - Select Type"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 2 - Select Connection"); ?></a></li>
					<li style='width:25%' class='active'><a><?php echo _("Step 3 - Select Server"); ?></a></li>
					<li style='width:25%'><a>	 			<?php echo _("Step 4 - Verify Services"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<table id="InstanceTable">
					<thead>
					<tr>
						<th width="60px" class="TextCenter">#</th>
						<th width="260px"><?php echo _("Name"); ?></th>
						<th width="240px"><?php echo _("Public Address"); ?></th>
						<th width="240px"><?php echo _("Internal Address"); ?></th>
						<th width="130px"><?php echo _("Instance Type"); ?></th>
						<th width="130px"><?php echo _("Power Status"); ?></th>
						<th width="140px"><?php echo _("Availability Zone"); ?></th>
					</tr>
					</thead>
				
					<tbody>
						<tr id='LoadingTable'><td height='37px' colspan='7'><div class="DefaultLoading"></div></td></tr>					
					</tbody>
				</table>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToLastPage' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='CancelToMgmt' 		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectAndNext' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>					
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>
