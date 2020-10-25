<script>
	/* Query Host List */
	function QueryAvailableHost(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryAvailableHost',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_TYPE' :''
			},
			success:function(jso)
			{
				if (jso != false)
				{
					option_level = '<?php echo $_SESSION['optionlevel']; ?>';
				
					count = Object.keys(jso).length;
					$.each(jso, function(key,value)
					{
						CHECK_RADIO = '';
						PAIR_COLOR = '';
						
						if (this.PROTECTED == true && option_level != 'DEBUG')
						{
							CHECK_RADIO = 'disabled';
						}
						
						if ((this.PROTECTED_NUM == 1 && this.INIT_CARRIER == false) || this.PROTECTED_NUM >= 2)
						{
							CHECK_RADIO = 'disabled';
						}

						if (this.HOST_ADDR == 'None')
						{
							HOST_ADDR = '<font color="#2F4F4F">'+this.SERV_MISC.ADDR+'</font>';
						}
						else
						{
							HOST_ADDR = this.HOST_ADDR;
						}
						
						/* Host Type */
						HOST_TYPE = this.HOST_TYPE;
						if (this.SERV_MISC.reg_cloud_uuid != null && this.SERV_MISC.reg_cloud_uuid.length == 36)
						{
							TYPE_HOST = 'Cloud';
							HOST_TYPE = 'Cloud';
						}
						else if (this.HOST_TYPE == 'Physical')
						{
							TYPE_HOST = '<div id="capitalize">'+this.OS_TYPE+'</div>';
						}
						else if (this.HOST_TYPE == 'Offline')
						{
							TYPE_HOST = HOST_TYPE;
						}
						else
						{
							TYPE_HOST = 'VMware';
						}
						
						if (this.SERV_UUID == "00000000-0000-0000-0000-000000000000")
						{
							CHECK_RADIO = 'disabled';
							PAIR_COLOR = 'style="color:#ff4000";';
						}
												
						$('#HostTable > tbody').append('<tr '+PAIR_COLOR+'>\
							<td width="60px" class="TextCenter"><input type="radio" id="HOST_UUID" name="HOST_UUID" value="'+this.HOST_UUID+'|'+this.OS_TYPE+'|'+HOST_TYPE+'|'+this.PRIORITY_ADDR+'" '+CHECK_RADIO+'/></td>\
							<td width="384px"><div class="ReplicaHostOverFlow">'+this.REAL_NAME+'</div></td>\
							<td width="170px">'+HOST_ADDR+'</td>\
							<td width="170px"><div id="capitalize">'+this.OS_TYPE+'</div></td>\
							<td width="170px">'+TYPE_HOST+'</td>\
							<td width="170px">'+Math.round((this.DISK_SIZE)/1024/1024/1024)+'GB</td>\
							<td width="60px" class="TextCenter"><div id="HostInfo" data-repl="'+this.HOST_UUID+'" style="cursor:pointer;"><i class="fa fa-sliders fa-lg"></i></div></td>\
						</tr>');
					});	

					$("#HostTable").on("click", "#HostInfo", function(e){
						$('#LoadingOverLay').addClass('GrayOverlay GearLoading');	
						$('#HostInfo').prop('disabled', true);
						var HOST_UUID = $(this).attr('data-repl');  //use  .attr() for accessing attibutes
						QuerySelectHostInfo(HOST_UUID);
					});

					if (count > 10)
					{
						$(document).ready(function()
						{
							$('#HostTable').DataTable(
							{
								paging		 : true,
								Info		 : false,
								bFilter		 : true,
								lengthChange : false,
								pageLength	 : 10,
								pagingType	 : "simple_numbers",
								order: [],
								columnDefs   : [ {
									targets: 'TextCenter',
									orderable: false										
								}],
								ordering	: true,
								aoColumns:[
												null,
												null,
												null,
												null,
												null,
												{"sType":"file-size"},
												null
								],
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
					
					 
					
					$('#ReplicaSelectService').prop('disabled', false);
					$('#ReplicaSelectService').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#HostTable > tbody').append('<tr><td colspan="8" class="padding_left_30"><?php echo _("Please add a host."); ?></td></tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Select Host Information */
	function QuerySelectHostInfo(HOST_UUID){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QuerySelectHostInfo', 
				 'HOST_UUID' :HOST_UUID
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#HostInfo').prop('disabled', false);
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: "<?php echo _("Host Information"); ?>",
					cssClass: 'workload-dialog',
					message: jso,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);				
			},
			error: function(xhr)
			{
				
			}
		});
	}	
	
	/* Post Select Host UUID To Session */
	function ReplicaSelectHostUUID(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'ReplicaSelectHostUUID', 
				 'HOST_INFO' :$("#HostTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = "SelectTargetTransportServer";
				}
				else
				{
					BootstrapDialog.show({
						title: "<?php echo _("Service Message"); ?>",
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],						
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	<!-- Begin to Exec Get List -->
	QueryAvailableHost();
		
	/* Submit Trigger */
	$(function(){
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtPrepareWorkload";
		})
		
		$("#BackToMgmtPrepareWorkload").click(function(){
			window.location.href = "MgmtPrepareWorkload";
		})
		
		$("#ReplicaSelectService").click(function(){
			ReplicaSelectHostUUID();			
		})
	});
</script>

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
					<li style='width:20%' class='active'><a><?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 2 - Select Target"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 3 - Configure Host"); ?></a></li>
					<li style='width:20%'><a>	 			<?php echo _("Step 4 - Configure Replication"); ?></a></li>
					<li style='width:20%'><a>		 		<?php echo _("Step 5 - Replication Summary"); ?></a></li>
				</ul>
			</div>	

			<div id='form_block_wizard'>
				<table id="HostTable" style='min-width:1190px;'>
					<thead>
					<tr>
						<th width="60px" class="TextCenter">#</th>
						<th width="384px"><?php echo _("Host Name"); ?></th>						
						<th width="170px"><?php echo _("Host IP"); ?></th>
						<th width="170px"><?php echo _("OS Type"); ?></th>
						<th width="170px"><?php echo _("Packer Type"); ?></th>
						<th width="170px"><?php echo _("Total Disk Size"); ?></th>						
						<th width="60px" class="TextCenter"><?php echo _("Details"); ?></th>							
					</tr>
					</thead>
				
					<tbody></tbody>
				</table>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToMgmtPrepareWorkload' 	class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>	
					<button id='CancelToMgmt' 				class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>
					<button id='ReplicaSelectService' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>									
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block_wizard-->
</div>
