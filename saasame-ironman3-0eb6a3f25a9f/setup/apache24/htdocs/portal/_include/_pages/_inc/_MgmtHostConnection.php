<script>
	/* Begin to Exec Get List */
	UnsetSession();
	QueryAvailableHost();
	
	/* Session Unset */
	function UnsetSession(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'UnsetSession'
			},			
		});
	}
	
	
	/*	Query Host List */
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
				$('.default').remove();
				
				if (jso.Code != false)
				{
					$('#NewHost').prop('disabled', false);
					$('#NewHost').removeClass('btn-default').addClass('btn-primary');
					
					if (jso != false)
					{
						count = Object.keys(jso).length;
						
						$.each(jso, function(key,value)
						{
							/* Host Address */
							if (this.HOST_ADDR == 'None')
							{
								HOST_ADDR = '<font color="#2F4F4F">'+this.SERV_MISC.ADDR+'</font>';
							}
							else
							{
								HOST_ADDR = this.HOST_ADDR;
							}

							/* Host Type */
							if (this.SERV_MISC.reg_cloud_uuid != null && this.SERV_MISC.reg_cloud_uuid.length == 36)
							{
								HOST_TYPE = 'Cloud';
							}
							else if (this.HOST_TYPE == 'Physical')
							{
								HOST_TYPE = '<div id="capitalize">'+this.OS_TYPE+'</div>';
							}
							else if (this.HOST_TYPE == 'Offline')
							{
								HOST_TYPE = '<div id="capitalize">'+this.HOST_TYPE+'</div>';
							}
							else
							{
								HOST_TYPE = 'VMware';
							}
							
							/* Direct Icon */
							if (this.IS_DIRECT == true)
							{
								PACKER_MODE = '<i class="fa fa-indent fa-lg"></i>';
							}
							else
							{
								PACKER_MODE = '<i class="fa fa-outdent fa-lg"></i>';
							}
				
							if (this.SERV_UUID == '00000000-0000-0000-0000-000000000000')
							{
								PAIR_COLOR = 'style="color:#ff4000";';
							}
							else
							{
								PAIR_COLOR = '';
							}	
							
							$('#HostTable > tbody').append('<tr '+PAIR_COLOR+'>\
							<td width="60px" class="TextCenter"><input type="radio" id="HOST_UUID" name="HOST_UUID" value="'+this.HOST_UUID+'" /></td>\
							<td width="265px"><div class="HostListOverFlow">'+this.HOST_NAME+'</div></td>\
							<td width="145px">'+HOST_ADDR+'</td>\
							<td width="160px"><div id="capitalize">'+this.OS_TYPE+'</div></td>\
							<td width="160px">'+HOST_TYPE+'</td>\
							<td width="150px">'+Math.round((this.DISK_SIZE)/1024/1024/1024)+'GB</td>\
							<td width="60px" class="TextCenter"><div id="HostInfo" data-repl="'+this.HOST_UUID+'" style="cursor:pointer;">'+PACKER_MODE+'</div></td>\
							</tr>');
					
													
						});
						
						$("#HostTable").on("click", "#HOST_UUID", function(e){
							OnClickSelectHost();
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
									aoColumns	:[
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
					}
					else
					{
						$('#HostTable > tbody').append('<tr><td colspan="8" class="padding_left_30"><?php echo _("Click New to add a host."); ?></td></tr>');					
					}
				}
				else
				{
					$('#HostTable > tbody').append('<tr><td colspan="8" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* OnClick Select Host */
	function OnClickSelectHost()
	{
		$('#EditHost').prop('disabled', false);
		$('#DeleteHost').prop('disabled', false);
		$('#RescanHost').prop('disabled', false);
			
		$('#EditHost').removeClass('btn-default').addClass('btn-primary');
		$('#DeleteHost').removeClass('btn-default').addClass('btn-primary');
		$('#RescanHost').removeClass('btn-default').addClass('btn-primary');
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
	
	/* Select Edit Host */
	function SelectEditHost(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SelectEditHost', 
				 'HOST_UUID' :$("#HostTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					switch(jso.Response)
					{
						case 'Physical':
							window.location.href = "EditHostPhysical";
						break;
						
						case 'Virtual':
							window.location.href = "EditHostVirtual";
						break;
						
						case 'Offline':
							window.location.href = "EditHostOfflineClone";
						break;
						
						case 'AWS':
							window.location.href = "EditAwsHostTransportType";
						break;
						
						case 'OpenStack':
							window.location.href = "EditHostTransportType";
						break;
					}
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Response,
						type: BootstrapDialog.type_info,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtHostConnection"
						},
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
		
	/* Delete Select Host */
	function DeletePackerHost(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'DeletePackerHost',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'HOST_UUID' :$("#HostTable input[type='radio']:checked").val()
			},
			
			success:function(jso)
			{
				LoadingOverLayAnimation(false);
				
				if (jso.Code == true)
				{				
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.type_info,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtHostConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtHostConnection"
						},
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/*	Reflush Packer	*/
	function ReflushPacker(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'ReflushPacker',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'HOST_UUID' :$("#HostTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				LoadingOverLayAnimation(false);
				window.location.href = "MgmtHostConnection";
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/*	Loading Over Lay Animation	*/
	function LoadingOverLayAnimation($TYPE)
	{
		if ($TYPE == true)
		{
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			$('#RescanHost').prop('disabled', true);
			$('#RescanHost').removeClass('btn-primary').addClass('btn-default');
			$('#DeleteHost').prop('disabled', true);
			$('#DeleteHost').removeClass('btn-primary').addClass('btn-default');
			$('#EditHost').prop('disabled', true);
			$('#EditHost').removeClass('btn-primary').addClass('btn-default');
			$('#NewHost').prop('disabled', true);
			$('#NewHost').removeClass('btn-primary').addClass('btn-default');
		}
		else
		{
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			$('#RescanHost').prop('disabled', false);
			$('#RescanHost').removeClass('btn-default').addClass('btn-primary');
			$('#DeleteHost').prop('disabled', false);
			$('#DeleteHost').removeClass('btn-default').addClass('btn-primary');
			$('#EditHost').prop('disabled', false);
			$('#EditHost').removeClass('btn-default').addClass('btn-primary');
			$('#NewHost').prop('disabled', false);
			$('#NewHost').removeClass('btn-default').addClass('btn-primary');
		}
	}

	/* Submit Trigger */
	$(function(){
		$("#NewHost").click(function(){
			window.location.href = "SelectHostType";
		})
		
		$("#DeleteHost").click(function(){
			BootstrapDialog.confirm("<?php echo _("Do you want to delete the selected host connection?"); ?>", function(result)
			{
				if (result == true)
				{
					setTimeout(function(){DeletePackerHost()},333);
				}
			});			
		})
		
		$("#EditHost").click(function(){
			SelectEditHost(); 
		})		
		
		$("#RescanHost").click(function(){
			BootstrapDialog.confirm("<?php echo _("Do you want to rescan the selected host connection?"); ?>", function(result)
			{
				if (result == true)
				{
					LoadingOverLayAnimation(true);
					setTimeout(function(){ReflushPacker()},333);					
				}
			});	
		})		
	});
</script>

<div id='container'>
	<div id='wrapper_block'>	
		<div id='form_block'>
			<div class="page">
				<div id='title_block'>
					<div id="title_h1">
						<div class="btn-toolbar">
							<i class="fa fa-server fa-fw"></i><?php echo _("Host"); ?>
							<button id='NewHost' 		class='btn btn-default pull-right' disabled><?php echo _("New"); ?></button>
							<button id='EditHost'   	class='btn btn-default pull-right' disabled><?php echo _("Edit"); ?></button>							
							<button id='DeleteHost' 	class='btn btn-default pull-right' disabled><?php echo _("Delete"); ?></button>
							<button id='RescanHost' 	class='btn btn-default pull-right' disabled><?php echo _("Rescan"); ?></button>
						</div>
					</div>										
				</div>
				
				<div id='form_block'>
					<table id="HostTable" style="min-width:1000px;">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="265px"><?php echo _("Host Name"); ?></th>	
							<th width="145px"><?php echo _("Host IP"); ?></th>
							<th width="160px"><?php echo _("OS Type"); ?></th>
							<th width="160px"><?php echo _("Packer Type"); ?></th>
							<th width="150px"><?php echo _("Total Disk Size"); ?></th>
							<th width="60px" class="TextCenter"><?php echo _("Details"); ?></th>
						</tr>
						</thead>				
						
						<tbody id="HostTableTbody">
							<tr class="default">
								<td colspan="8" class="padding_left_30"><?php echo _("Checking management connection..."); ?></td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
		</div> <!-- id: form_block-->		
	</div> <!-- id: wrapper_block-->
</div>