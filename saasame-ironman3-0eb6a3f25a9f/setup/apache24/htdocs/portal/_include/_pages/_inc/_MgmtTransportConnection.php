<script>
	/* Begin to Exec Get List */
	UnsetSession();
	QueryAvailableService();
		
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
		
	/* Query Service List */
	function QueryAvailableService(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryAvailableService',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_TYPE' :'Launcher',
				 'SYST_TYPE' :''
			},
			success:function(jso)
			{
				$('.default').remove();
				
				if (jso.Code != false)
				{
					$('#AddNewService').prop('disabled', false);
					$('#AddNewService').removeClass('btn-default').addClass('btn-primary');
					
					if (jso != false)
					{
						$.each(jso, function(key,value)
						{
							if (this.SERV_LOCA == 'Cloud')
							{
								switch (this.HOST_TYPE) 
								{
									case 'OPENSTACK':
										if (this.SYST_TYPE == 'WINDOWS')
										{										
											if (this.VENDOR_NAME == 'OPENSTACK')
											{
												SERV_LOCATION = 'OpenStack';
											}
											else
											{											
												SERV_LOCATION = this.VENDOR_NAME;											
											}
										}
										else
										{
											SERV_LOCATION = 'Linux Launcher';
										}
									break;
									
									case 'AWS':
										SERV_LOCATION = 'AWS';
									break;
									
									case 'Azure':
										SERV_LOCATION = 'Azure';
									break;
									
									case 'Aliyun':
										SERV_LOCATION = 'Alibaba Cloud';
									break;
									
									case 'Ctyun':
										SERV_LOCATION = '天翼云';
									break;
									
									case 'Tencent':
										SERV_LOCATION = 'Tencent Cloud';
									break;
									
									default:
										SERV_LOCATION = this.HOST_TYPE;								
								}														
							}
							else if(this.SERV_LOCA == 'On Premise')
							{
								SERV_LOCATION = 'General Purpose';
							}
							else if(this.SERV_INFO.is_winpe == true)
							{
								SERV_LOCATION = 'Recovery Kit';
							}
							else
							{
								SERV_LOCATION = this.SERV_LOCA;
							}
							
							
							if(this.SERV_INFO.direct_mode == true)
							{
								SERV_MODE = '<i class="fa fa-indent fa-lg"></i>';
							}
							else
							{
								SERV_MODE = '<i class="fa fa-outdent fa-lg"></i>';
							}
							
							
							$('#TransportTable > tbody').append(
							'<tr>\
								<td width="60px" class="TextCenter"><input type="radio" id="SERV_UUID" name="SERV_UUID" value="'+this.SERV_UUID+'" /></td>\
								<td width="400px">'+this.HOST_NAME+'</td>\
								<td width="190px">'+SERV_LOCATION+'</td>\
								<td width="290px">'+this.SERV_ADDR+'</td>\
								<td width="60px" class="TextCenter"><div id="TransportInfo" data-repl="'+this.SERV_UUID+'" style="cursor:pointer;">'+SERV_MODE+'</div></td>\
							</tr>');
						});
						
						$("#TransportTable").on("click", "#SERV_UUID", function(e){
							OnClickSelectTransport();
						});	
						
						$("#TransportTable").on("click", "#TransportInfo", function(e){
							$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
							$('#TransportInfo').prop('disabled', true);
							var SERV_UUID = $(this).attr('data-repl');  //use  .attr() for accessing attibutes							
							QuerySelectTransportInfo(SERV_UUID);
						});						
					}
					else
					{
						$('#TransportTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Click New to add a Transport."); ?></td></tr>');
					}
				}
				else
				{
					$('#TransportTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* OnClick Select Transport */
	function OnClickSelectTransport()
	{
		$('#EditService').prop('disabled', false);
		$('#DeleteService').prop('disabled', false);
						
		$('#EditService').removeClass('btn-default').addClass('btn-primary');
		$('#DeleteService').removeClass('btn-default').addClass('btn-primary');	
	}
	
	/* Query Select Transport Server Information */
	function QuerySelectTransportInfo(SERV_UUID){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QuerySelectTransportInfo', 
				 'SERV_UUID' :SERV_UUID
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#TransportInfo').prop('disabled', false);
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: '<?php echo _("Transport Server Information"); ?>',
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
	
	
	/* Edit Transport Server */
	function EditTransportService()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SelectEditServiceByUUID', 
				 'SERV_UUID' :$("#TransportTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					switch(jso.ServInfo.RDIR_TYPE)
					{
						case 'OnPremise':
							window.location.href = "EditOnPremiseTransportServices"
						break;
						
						case 'RecoveryKit':
							window.location.href = "EditRecoveryKitServices"
						break;
						
						case 'OpenStack':
							if (jso.ServInfo.SYST_TYPE == 'WINDOWS')
							{
								window.location.href = "EditCloudTransportServices"
							}
							else if(jso.ServInfo.SYST_TYPE == 'LINUX')
							{
								window.location.href = "EditLinuxLauncherService"
							}
							else
							{
								window.location.href = "MgmtTransportConnection"
							}						
						break;
						
						case 'AWS':
							if (jso.ServInfo.SYST_TYPE == 'WINDOWS')
							{
								window.location.href = "EditAwsTransportServices"
							}
							else if(jso.ServInfo.SYST_TYPE == 'LINUX')
							{
								window.location.href = "EditAwsLinuxLauncherService"
							}
							else
							{
								window.location.href = "MgmtTransportConnection"
							}
						break;
						
						case 'Azure':
							window.location.href = "EditAzureTransportServices"
						break;
						
						case 'Aliyun':
							window.location.href = "EditAliyunTransportServices"
						break;
						
						case 'Ctyun':
							window.location.href = "EditCtyunTransportServices"
						break;
						case 'Tencent':
							window.location.href = "EditTencentTransportServices"
						break;
						case 'VMWare':
							window.location.href = "EditOnPremiseTransportServices"
						break;
					}	
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.ServInfo,
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
	
	/* Delete Transport Server */
	function DeleteTransporService()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'DeleteServiceByUUID', 
				 'SERV_UUID' :$("#TransportTable input[type='radio']:checked").val()
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
							window.location.href = "MgmtTransportConnection"
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
					});
				}
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
			$('#AddNewService').prop('disabled', true);
			$('#AddNewService').removeClass('btn-primary').addClass('btn-default');
			$('#EditService').prop('disabled', true);
			$('#EditService').removeClass('btn-primary').addClass('btn-default');
			$('#DeleteService').prop('disabled', true);
			$('#DeleteService').removeClass('btn-primary').addClass('btn-default');
		}
		else
		{
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			$('#AddNewService').prop('disabled', false);
			$('#AddNewService').removeClass('btn-default').addClass('btn-primary');
			$('#EditService').prop('disabled', false);
			$('#EditService').removeClass('btn-default').addClass('btn-primary');
			$('#DeleteService').prop('disabled', false);
			$('#DeleteService').removeClass('btn-default').addClass('btn-primary');
		}
	}
		
	/* Submit Trigger */
	$(function(){
		$("#AddNewService").click(function(){
			window.location.href = "SelectServRegion";
		})

		$("#EditService").click(function(){
			EditTransportService(); 
		})
		
		$("#DeleteService").click(function(){
			BootstrapDialog.confirm("<?php echo _("Do you want to delete the selected transport server connection?"); ?>", function(result){
				if (result == true)
				{
					LoadingOverLayAnimation(true);
					setTimeout(function(){DeleteTransporService()},333);
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
							<i class="fa fa-subway fa-fw"></i>&nbsp;<?php echo _("Transport"); ?>
							<button id='AddNewService' class='btn btn-default pull-right' disabled><?php echo _("New"); ?></button>
							<button id='EditService'   class='btn btn-default pull-right' disabled><?php echo _("Edit"); ?></button>							
							<button id='DeleteService' class='btn btn-default pull-right' disabled><?php echo _("Delete"); ?></button>
						</div>
					</div>										
				</div>
				
				<div id='form_block'>
					<table id="TransportTable">
						<thead>
						<tr>
							<th width='60px' class="TextCenter">#</th>
							<th width='400px'><?php echo _("Server"); ?></th>							
							<th width='190px'><?php echo _("Type"); ?></th>
							<th width='290px'><?php echo _("Address"); ?></th>
							<th width='60px' class="TextCenter"><?php echo _("Details"); ?></th>
						</tr>
						</thead>
						
						<tbody>
							<tr class='default'>
								<td colspan="5" class="padding_left_30"><?php echo _("Checking management connection..."); ?></td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
		</div> <!-- id: form_block-->		
	</div> <!-- id: wrapper_block-->
</div>
