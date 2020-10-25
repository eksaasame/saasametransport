<?php

	$HOST_INFO = explode('|', $_SESSION['HOST_UUID']);
	$HOST_UUID = $HOST_INFO[0];
	$HOST_REGN = $HOST_INFO[1];
	
	if( isset( $_SESSION['RG'] ) )
	{ 
		$RG = $_SESSION['RG'];
	}
	else
	{
		$RG = null;
	}
	
	if (isset($_SESSION['SERV_UUID']))
	{
		$SERV_UUID = $_SESSION['SERV_UUID'];
	}
	else
	{
		$SERV_UUID = null;
	}	
?>
	
<script>

	var IsDisableUpdate = false;

	/* Determine URL Segment */	
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT == $SET_SEGMENT)
		{
			window.setTimeout(QueryTransportServerInfo(),500); // 0.5 seconds
		}
	}
	
	/* Check Running Connection */
	function CheckRunningConnection()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 : 'CheckRunningConnection',
				 'SERV_UUID' : '<?php echo $SERV_UUID; ?>'			 
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
					
					//$('#VerifyService').removeClass('btn-primary').addClass('btn-default');	
					$('#VerifyService').prop('disabled', false);
					
					IsDisableUpdate = true;
				}
			}
		})
	}
	
	/* Query Select Server Information*/
	function QueryTransportServerInfo()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 : 'QueryTransportServerInformation',
				 'SERV_UUID' : '<?php echo $SERV_UUID; ?>' 
			},
			success:function(jso)
			{
				SAVE_SERV_ADDR = jso.SERV_ADDR;
				SERV_ADDR_ARRAY = transport_addr.split(',');
				var INPUT_ADDR = [];
				jQuery.grep(SAVE_SERV_ADDR, function(el) {if (jQuery.inArray(el, SERV_ADDR_ARRAY) == -1) INPUT_ADDR.push(el);});
				
				if (INPUT_ADDR.length != 0)
				{
					$('#OtherIpAddress').val(INPUT_ADDR[0]);
				}
				
				$('#BackToLastPage').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
				
				CheckRunningConnection();
			}
		})
	}

	var transport_addr = '';
	
	/* Select Service */
	function SelectService()
	{
		return 'Scheduler,Carrier,Loader,Launcher';
	}

	<!-- Begin to Automatic Exec -->
	QuerySelectedHostInformation();

	/* List Selected Host Information */
	function QuerySelectedHostInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_NAME' 	:'<?php echo $HOST_UUID; ?>',
				 'HOST_REGN' 	:'<?php echo $HOST_REGN; ?>',
				 'HOST_RG'		:'<?php echo $RG; ?>',
				 'SERV_UUID'	:'<?php echo $SERV_UUID; ?>'
			},
			success:function(jso)
			{
				$('#LoadingTable').remove();
				if (jso != false)
				{
					managed = jso.managed;
					
					if( jso.managed == true )
						manageDisk = "Yes";
					else
						manageDisk = "No";
					
					$('#InstanceDetailTable > tbody').append(
						'<tr><td><?php echo _("Name"); ?></td><td>'+jso.name+'</td></tr>\
						 <tr><td><?php echo _("Public Address"); ?></td><td>'+jso.public_ip+'</td></tr>\
						 <tr><td><?php echo _("Internal Address"); ?></td><td>'+jso.private_ip+'</td></tr>\
						 <tr><td><?php echo _("Internet Address"); ?></td><td><input type="text" id="OtherIpAddress"></input></td></tr>\
						 <tr><td><?php echo _("Instance Type"); ?></td><td>'+jso.type+'</td></tr>\
						 <tr><td><?php echo _("Power Status"); ?></td><td>'+jso.enable+'</td></tr>\
						 <tr><td><?php echo _("Management Disk"); ?></td><td>'+manageDisk+'</td></tr>\
						 <tr><td><?php echo _("Resource Group"); ?></td><td>'+jso.resource_group+'</td></tr>\
						 <tr><td><?php echo _("Availability Zone"); ?></td><td>'+jso.location+'</td></tr>'
					);
					
					if (jso.public_ip == 'None')
					{
						transport_addr = jso.private_ip;
					}
					else
					{
						transport_addr = jso.private_ip+','+jso.public_ip;
					}
					/* Determine Edit Segment */
					DetermineSegment('EditAzureTransportServices');
					
					$('#VerifyService').prop('disabled', false);
					$('#VerifyService').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#InstanceDetailTable > tbody').append(
						'<tr><td class="TextCenter">-</td><td colspan="2"><?php echo _("No Host Information!"); ?></td></tr>'
					);
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
		/* Get List of Ip Address */
		if ($("#OtherIpAddress").val() != '')
		{
			transport_addr = $("#OtherIpAddress").val()+','+transport_addr;
		}
		
		$.ajax({
		type: 'POST',
		dataType:'JSON',
		url: '_include/_exec/mgmt_service.php',
		data:{
			 'ACTION'	 :'TestServicesConnection',
			 'SERV_ADDR' :transport_addr,
			 'SELT_SERV' :service,
			 'MGMT_ADDR' :document.domain
		},
		success:function(jso)
		{
			if (jso.Code != false)
			{
				transport_addr = transport_addr.split(',');
				transport_addr.splice($.inArray(jso.Msg.prefer_address, transport_addr),1);
				transport_addr.unshift(jso.Msg.prefer_address);
				transport_addr = transport_addr.toString();
				
				webdav_path = jso.Msg.path+'\\imagex';
				VerifyFolderConnection();
			}
			else
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#VerifyService').prop('disabled', false);
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
	
	/* Verify Folder Connection */
	function VerifyFolderConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'VerifyFolderConnection',
				 'SERV_ADDR' :transport_addr,
				 'CONN_DEST' :webdav_path
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');				
				if (jso == true)
				{
					$('#OtherIpAddress').prop('disabled', true);
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifyService').prop('disabled', false);
					
					BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					type: BootstrapDialog.TYPE_DANGER,
					message: jso,						
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}}],
					onhide: function(dialogRef){
							window.location.href = "VerifyAwsTransportServices"
						}
					});
					
					if( !IsDisableUpdate )
						$('#CheckAndSubmit').prop('disabled', true);
				}
			},
			error: function(xhr)
			{
				
			}
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
				 'HOST_UUID' :'<?php echo $HOST_UUID.'|'.$HOST_REGN; ?>'+'|Azure',
				 'SERV_ADDR' :transport_addr,
				 'SYST_TYPE' :'WINDOWS',
				 'SELT_SERV' :service,
				 'MGMT_ADDR' :document.domain,
				 'CONN_DEST' :webdav_path,
				 'MGMT_DISK' :managed
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
				 'SERV_ADDR' :transport_addr,
				 'MGMT_ADDR' :document.domain
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
		$("#BackToLastPage").click(function(){
			window.location.href = "SelectAzureTransportInstance";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtTransportConnection";
		})
		
		$("#VerifyService").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#VerifyService').prop('disabled', true);
			VerifyServiceConnection(SelectService());
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#CheckAndSubmit').prop('disabled', true);
			
			setPublicIpStatic();

			if ($URL_SEGMENT == 'VerifyAzureTransportServices')
			{		
				InitializeNewServices( SelectService() );
			}
			else
			{
				UpdateTrnasportServices();
			}
		})
	});
	
	function setPublicIpStatic() {		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'	 	:'setPublicIpStatic',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'HOST_NAME' 	:'<?php echo $HOST_UUID; ?>',
				 'HOST_REGN' 	:'<?php echo $HOST_REGN; ?>',
				 'HOST_RG'		:'<?php echo $RG; ?>',
				 'SERV_UUID'	:'<?php echo $SERV_UUID; ?>'
			},
			success:function(jso)
			{
				
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
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
				<table id="InstanceDetailTable">
					<thead>
					<tr>
						<th width="150px"><?php echo _("Name"); ?></th>
						<th><?php echo _("Description"); ?></th>
					</tr>
					</thead>
				
					<tbody>
						<tr id='LoadingTable'><td height='37px' colspan='2'><div class="DefaultLoading"></div></td></tr>	
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
