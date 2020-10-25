<?php if (isset($_SESSION['SERV_UUID'])){$SERV_UUID = $_SESSION['SERV_UUID'];}else{$SERV_UUID = null;} ?>
<script>
	<!-- Begin to Automatic Exec -->
	QuerySelectedHostInformation();
	
	/* Check Running Connection */
	function CheckRunningConnection()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'CheckRunningConnection',
				 'SERV_UUID' :'<?php echo $SERV_UUID; ?>'	 
			},
			success:function(jso)
			{
				if (jso.code == false)
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
					
					$('#VerifyService').removeClass('btn-primary').addClass('btn-default');	
					$('#VerifyService').prop('disabled', true);
				}
			}
		})
	}	
	
	/* List Selected Host Information */
	function QuerySelectedHostInformation()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_UUID' 	:'<?php echo $_SESSION['HOST_UUID']; ?>'
			},
			success:function(jso)
			{
				$('#LoadingTable').remove();
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						fixed_addr    = '-';
						floating_addr = '-';
						$.each(value.addresses[Object.keys(value.addresses)[0]], function(addr_key,addr_value)
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
						
						if (floating_addr == '-')
						{
							transport_addr = fixed_addr;
						}
						else
						{
							transport_addr = fixed_addr+','+floating_addr;
						}
						
						$('#LauncherDetailTable > tbody').append(
							'<tr><td>Name</td><td>'+value.name+'</td></tr>\
							 <tr><td>Cluster Name</td><td>'+value['OS-EXT-SRV-ATTR:host']+'</td></tr>\
							 <tr><td>Fixed IP</td><td>'+fixed_addr+'</td></tr>\
							 <tr><td>Floating IP</td><td>'+floating_addr+'</td></tr>\
							 <tr><td>Security Groups</td><td>'+value.security_groups[0]['name']+'</td></tr>\
							 <tr><td>Launched At</td><td>'+value['OS-SRV-USG:launched_at']+'</td></tr>\
							 <tr><td>Update Date</td><td>'+value.updated+'</td></tr>\
							 <tr><td>Status</td><td>'+value.status+'</td></tr>'
						);
					});
					
					$('#VerifyService').prop('disabled', false);						
					$('#VerifyService').removeClass('btn-default').addClass('btn-primary');	
				}
				else
				{
					$('#LauncherDetailTable > tbody').append(
						'<tr><td class="TextCenter">-</td><td colspan="2">No Host Information!</td></tr>'
					);
				}
				
				/* Update Submit Text */
				$URL_SEGMENT = window.location.pathname.split('/').pop();
				if ($URL_SEGMENT == 'EditLinuxLauncherService')
				{
					$("#BackToLastPage").remove();
					$("#CheckAndSubmit").text("Update");
					
					CheckRunningConnection();
				}

				$('#BackToLastPage').prop('disabled', false);	
				$('#CancelToMgmt').prop('disabled', false);
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Verify Service Connection */
	function VerifyServiceConnection(service)
	{
		$.ajax({
		type: 'POST',
		dataType: 'JSON',
		url: '_include/_exec/mgmt_service.php',
		data: {
			 'ACTION'	: 'TestServicesConnection',
			 'SERV_ADDR': transport_addr,
			 'SELT_SERV': service,
			 'MGMT_ADDR' :document.domain
		},
		success: function(jso)
		{
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			if (jso.Code != false)
			{
				if (jso.Msg.id == '{6FC9C4B6-B61E-4745-A6AA-1D5F0A2DA08B}')
				{
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifyService').prop('disabled', false);
					$('#VerifyService').removeClass('btn-default').addClass('btn-primary');	
					BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					type: BootstrapDialog.TYPE_DANGER,
					message: 'Failed to verify Linux launcher service',	
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}}],
					});	
				}
			}
			else
			{
				$('#VerifyService').prop('disabled', false);
				$('#VerifyService').removeClass('btn-default').addClass('btn-primary');				
				BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				type: BootstrapDialog.TYPE_DANGER,
				message: jso.Msg,						
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
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
		},
		});
	}
	
	/* Initialize New Service */
	function InitializeNewServices(service){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'InitializeNewServices',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' :'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_UUID' :'<?php echo $_SESSION['HOST_UUID']; ?>',
				 'SERV_ADDR' :transport_addr,
				 'SYST_TYPE' :'LINUX',
				 'SELT_SERV' :service,
				 'MGMT_ADDR' :document.domain,
				 'CONN_DEST' :'NA'
			},
			success: function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef)
							{
								dialogRef.close();
							}}],
						onhide: function(dialogRef){
							window.location.href = "MgmtTransportConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					type: BootstrapDialog.TYPE_DANGER,
					message: jso.Msg,						
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}}],
					});
					$('#INPUT_SERV_IP').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-primary').addClass('btn-default');
					$('#VerifyService').removeClass('btn-default').addClass('btn-primary');
					$('#VerifyService').prop('disabled', false);
				}
			},
			error: function(xhr)
			{
				
			}
		});	    
	}
	
	/* Update Transport Services */
	function UpdateTrnasportServices(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'UpdateTrnasportServices',
				 'SERV_UUID' :'<?php echo $SERV_UUID; ?>',
				 'SERV_ADDR' :transport_addr
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef)
							{
								dialogRef.close();
							}}],
						onhide: function(dialogRef){
							window.location.href = "MgmtTransportConnection"
						},
					});					
				}
				else
				{
					BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					type: BootstrapDialog.TYPE_DANGER,
					message: jso.Msg,						
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}}],
					});
					$('#VerifyService').removeClass('btn-default').addClass('btn-primary');
					$('#VerifyService').prop('disabled', false);				
				}
			},
			error: function(xhr)
			{
				
			}
		});			
	}
	
	/* Submit Trigger */
	$(function(){
		$('#CheckAndSubmit').prop('disabled', true);

		$("#BackToLastPage").click(function(){
			window.location.href = "SelectLinuxLauncher";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtTransportConnection";
		})
		
		$("#VerifyService").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#VerifyService').removeClass('btn-primary').addClass('btn-default');
			$('#VerifyService').prop('disabled', true);
			VerifyServiceConnection('Launcher');
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#CheckAndSubmit').prop('disabled', true);
			if ($URL_SEGMENT == 'VerifyLinuxLauncherService')
			{	
				InitializeNewServices('Launcher');
			}
			else
			{
				UpdateTrnasportServices();
			}
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
					<li style='width:25%'><a>				<?php echo _("Step 3 - Select Server"); ?></a></li>
					<li style='width:25%' class='active'><a><?php echo _("Step 4 - Verify Services"); ?></a></li>
				</ul>
			</div>

			<div id='form_block_wizard'>
				<table id="LauncherDetailTable">
					<thead>
					<tr>
						<th width="150px"><?php echo _("Name"); ?></th>
						<th><?php echo _("Description"); ?></th>
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
					<button id='CheckAndSubmit'		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
					<button id='VerifyService' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Verify Service"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>
