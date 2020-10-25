<?php 
	if (isset($_SESSION['CLUSTER_UUID']))
	{
		$HOST_INFO = explode('|', $_SESSION['HOST_UUID']);
		$INFO_COUNT = count($HOST_INFO);
		$CLUSTER_UUID = $_SESSION['CLUSTER_UUID'];
		$HOST_UUID = $HOST_INFO[0];
		$HOST_REGN = $HOST_INFO[1];
		$RG = $_SESSION['RG'];
	}
	else
	{
		$CLUSTER_UUID = null;
		$HOST_UUID = $_SESSION['HOST_UUID'];
		$HOST_REGN = null;
		$RG = null;
	}
?>

<script>
	<!-- Begin to Automatic Exec -->
	QueryAvailableService();
	QueryHostInfo();

	/* Query Select Server Information*/
	function QueryHostInfo()
	{
		url_segment = window.location.pathname.split('/').pop();
		open_uuid = '<?php echo $CLUSTER_UUID; ?>';
		host_uuid = '<?php echo $HOST_UUID; ?>';
		host_regn = '<?php echo $HOST_REGN; ?>';
		host_rg   = '<?php echo $RG; ?>';
	
		if (open_uuid != '')
		{
			cluster_uuid = open_uuid;
			QuerySelectedHostInformation();
		}
		else
		{
			$.ajax({
				type: 'POST',
				dataType:'JSON',
				url: '_include/_exec/mgmt_service.php',
				data:{
					 'ACTION'	 : 'QueryHostInfo',
					 'HOST_UUID' : host_uuid	,
					 'RG'		 : host_rg
				},
				success:function(jso)
				{
					$("#BackToLastPage").remove();
					$("#CheckAndSubmit").text("Update");
					
					cluster_uuid = jso.HOST_INFO.reg_cloud_uuid;
					host_uuid 	 = jso.HOST_INFO.instance_id;
					host_regn 	 = jso.HOST_INFO.availability_zone;
					QuerySelectedHostInformation();
				}
			})
		}
	}
	var vm_name = '';
	var AvailabilityZone = '';
	/* List Selected Host Information */
	function QuerySelectedHostInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_tencent.php',
			data:{
				 'ACTION'    	:'QuerySelectedHostInformation',
				 'CLUSTER_UUID' :cluster_uuid,
				 'HOST_ID' 		:host_uuid,
				 'HOST_REGN' 	:host_regn,
				 'HOST_RG' 		:host_rg
			},
			success:function(jso)
			{
				$('#LoadingTable').remove();
				if (jso != false)
				{
					vm_name = jso.name;
					AvailabilityZone = jso.Zone;
					$('#InstanceDetailTable > tbody').append(
						'<tr><td>Name</td><td>'+jso.InstanceName+'</td></tr>\
						 <tr><td>Public Address</td><td>'+jso.PublicIpAddress+'</td></tr>\
						 <tr><td>Internal Address</td><td>'+jso.PrivateIpAddress+'</td></tr>\
						 <tr><td>Instance Type</td><td>'+jso.InstanceType+'</td></tr>\
						 <tr><td>Power Status</td><td>'+jso.Status+'</td></tr>\
						 <tr><td>Availability Zone</td><td>'+jso.Zone+'</td></tr>'
					);
					
					if (jso.public_ip == 'None')
					{
						transport_addr = jso.PrivateIpAddress;
					}
					else
					{
						transport_addr = jso.PrivateIpAddress+','+jso.PublicIpAddress;
					}
				}
				else
				{
					$('#InstanceDetailTable > tbody').append(
						'<tr><td class="TextCenter">-</td><td colspan="2">No Host Information!</td></tr>'
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
		
	/* Query Service List */
	function QueryAvailableService(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryAvailableService',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_TYPE' :'Carrier',
				 'SYST_TYPE' :''
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						/* SELECT HOME SERVER ONLY Tencent TYPE */
						if (this.HOST_TYPE == 'Tencent')
						{						
							$('#TRANSPORT_SERV_INFO').append(new Option(this.HOST_NAME+' - '+this.SERV_ADDR[0], this.SERV_UUID+'|'+this.SERV_ADDR[this.SERV_ADDR.length-1], true, true));
							$('#TestPackerConn').prop('disabled', false);
							$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
						}
					});
					$('.selectpicker').selectpicker('refresh');
				}				
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Test Service Connection */
	function TestServiceConnection(){
		
			SERV_INFO		= $("#TRANSPORT_SERV_INFO").val().split('|');
			SERV_UUID		= SERV_INFO[0];
			SERV_NIC_ADDR 	= SERV_INFO[1];
		
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'TestServiceConnection',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_UUID' :SERV_UUID,
				 'HOST_ADDR' :transport_addr,
				 'HOST_USER' :'',
				 'HOST_PASS' :'',
				 'HOST_UUID' :'<?php echo $HOST_UUID; ?>',
				 'SERV_TYPE' :'Physical Packer'
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code != false)
				{
					OS_TYPE = jso.OS_Type;
					
					$('#TRANSPORT_SERV_INFO').prop('disabled', true);
					
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
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
							}
						}],
					});
					$('#TestPackerConn').prop('disabled', false);
					$('#CheckAndSubmit').prop('disabled', true);					
         		}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');        
			}
		});
	}

	/* Initialize New Service */
	function InitializeNewService(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'InitializeNewService',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' :'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' :'',
				 'SERV_UUID' :SERV_UUID,
				 'SERV_ADDR' :SERV_NIC_ADDR,
				 'HOST_ADDR' :transport_addr,
				 'HOST_USER' :vm_name+'|'+AvailabilityZone,		/* Only Use this value for Cloud Select Client */
				 'HOST_PASS' :cluster_uuid, 						/* Only Use this value for Cloud Select Client */
				 'SYST_TYPE' :'',									/* Only Use this value for Cloud Select Client */
				 'SERV_TYPE' :'Physical Packer',
				 'OS_TYPE'	 	:OS_TYPE
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
				$('#CheckAndSubmit').prop('disabled', true);
	
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtHostConnection";
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
					$('#TestPackerConn').prop('disabled', false);          
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);        
			}
		});	    
	}
	
	
	/* Update Host Information */
	function UpdateHostInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'UpdateHostInformation',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' :'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' :'',
				 'SERV_UUID' :SERV_UUID,
				 'SERV_ADDR' :SERV_NIC_ADDR,
				 'HOST_UUID' :'<?php echo $_SESSION['HOST_UUID']; ?>',
				 'HOST_ADDR' :transport_addr,
				 'HOST_USER' :vm_name+'|'+AvailabilityZone,			/* Only Use For Azure */
				 'HOST_PASS' :cluster_uuid,								/* Only Use For Azure */
				 'SERV_TYPE' :'Physical Packer'
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
				$('#CheckAndSubmit').prop('disabled', true);
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtHostConnection";
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
					$('#TestPackerConn').prop('disabled', false);
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', false);
        	}
		});	    
	}
	
	
	/* Submit Trigger */
	$(function(){
		$("#BackToLastPage").click(function(){
			window.location.href = "SelectTencentHostTransportInstance";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtTransportConnection";
		})
		
		$("#TestPackerConn").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#TestPackerConn').prop('disabled', true);
			TestServiceConnection();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if (url_segment == 'VerifyTencentHostTransportType')
			{
				InitializeNewService();
			}
			else
			{
				UpdateHostInformation();
			}
		})
	});

</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-server fa-fw"></i>&nbsp;<?php echo _("Host"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:25%'><a>				<?php echo _("Step 1 - Select Type"); ?> </a></li>
					<li style='width:25%'><a>				<?php echo _("Step 2 - Select Connection"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 3 - Select Server"); ?></a></li>
					<li style='width:25%' class='active'><a><?php echo _("Step 4 - Verify Host"); ?></a></li>
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
				<table>
					<tr>
						<th><label for='comment'><?php echo _("Home Transport Server:"); ?></label></th>
					</tr>
					<tr>
						<td>
						<select id="TRANSPORT_SERV_INFO" class="selectpicker" data-width="100%"></select>
						</td>
					</tr>
				</table>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToLastPage' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='CancelToMgmt' 		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='CheckAndSubmit'		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
					<button id='TestPackerConn'		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Test Connection"); ?></button>									
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>
