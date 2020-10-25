<?php if (isset($_SESSION['HOST_UUID'])){$HOST_UUID = $_SESSION['HOST_UUID'];}else{$HOST_UUID = null;} ?>
<script>
	<!-- Begin to Exec Get List -->
	QueryAvailableService();
	
	/* Determine URL Segment */
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT == $SET_SEGMENT)
		{
			$('#TRANSPORT_SERV_INFO').prop('disabled', true);
			QueryAccessCredentialInformation();
		}
	}
	
	/* Query Access Credential Information */
	function QueryAccessCredentialInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo isset($_SESSION['CLUSTER_UUID'])?$_SESSION['CLUSTER_UUID']:"false"; ?>'	 
			},
			success:function(jso)
			{
				$('#INPUT_HOST_IP').val(jso.endpoint);
				$('#INPUT_HOST_USER').val(jso.name);
				$('#INPUT_HOST_PASS').val(jso.pass);

				$('.selectpicker').selectpicker('refresh');
				
				$('#BackToMgmt').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
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
						var HOST_TYPE = value['VENDOR_NAME'];
						var mapObj = {UnknownVendorType:"General Purpose",OPENSTACK:"OpenStack",OpenStack:"OpenStack",AzureBlob:"Azure",Azure:"Azure",Aliyun:"Alibaba Cloud",Ctyun:"天翼云"};
						HOST_TYPE = HOST_TYPE.replace(/UnknownVendorType|OPENSTACK|AzureBlob|Aliyun|Ctyun/gi, function(matched){return mapObj[matched];});	

						if( window.location.pathname.split('/').pop() == 'EditVMWareConnection' ){
							if( '<?php echo isset($_SESSION['CLUSTER_UUID'])?$_SESSION['CLUSTER_UUID']:null; ?>' == value['OPEN_UUID'] ){
								$('#TRANSPORT_SERV_INFO').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.OPEN_UUID+'|'+this.SERV_UUID+'|'+this.SERV_ADDR, true, true));
							}
						}
						else if( HOST_TYPE == "General Purpose" )
							$('#TRANSPORT_SERV_INFO').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.OPEN_UUID+'|'+this.SERV_UUID+'|'+this.SERV_ADDR, true, true));
						
					});
					$('.selectpicker').selectpicker('refresh');
					
					DetermineSegment('EditVMWareConnection');
					
					$('#TestPackerConn').prop('disabled', false);
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
				}		
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Test Service Connection */
	function TestServiceConnection(){
			SERVER_INFO = $("#TRANSPORT_SERV_INFO").val().split('|');
			OPEN_UUID = SERVER_INFO[0];
			SERV_UUID = SERVER_INFO[1];
			Transport_ip = SERVER_INFO[2];
			
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'	 :'TestEsxConnection',
				 'ESX_ADDR' :$("#INPUT_HOST_IP").val(),
				 'ESX_USER' :$("#INPUT_HOST_USER").val(),
				 'ESX_PASS' :$("#INPUT_HOST_PASS").val(),
				 'SERVER_ID':SERV_UUID,
				 'TRANSPORT_IP' :Transport_ip
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				
				if( jso.success !== true )
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.why,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#TestPackerConn').prop('disabled', false);
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
					
					return;
				}
				
				$('#INPUT_HOST_IP').prop('disabled', true);
				$('#INPUT_HOST_USER').prop('disabled', true);
				$('#INPUT_HOST_PASS').prop('disabled', true);
				$('#TRANSPORT_SERV_INFO').prop('disabled', true);

				$('#CheckAndSubmit').prop('disabled', false);
				$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
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
				 'ACTION'	 		:'UpdateHostInformation',
				 'ACCT_UUID' 		:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 		:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' 		:OPEN_UUID,
				 'SERV_UUID' 		:SERV_UUID,
				 'HOST_UUID' 		:'<?php echo $HOST_UUID; ?>',
				 'HOST_ADDR' 		:$("#INPUT_HOST_IP").val(),
				 'HOST_USER' 		:$("#INPUT_HOST_USER").val(),
				 'HOST_PASS' 		:$("#INPUT_HOST_PASS").val(),
				 'SERV_TYPE' 		:'Virtual Packer',
				 'SELECT_VM_HOST'	:SELECT_VM_HOST
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
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	    
	}
	
	/* Initialize New Connection */
	function InitializeNewConnection(){
		
		SERVER_INFO = $("#TRANSPORT_SERV_INFO").val().split('|');
		OPEN_UUID = SERVER_INFO[0];
		SERV_UUID = SERVER_INFO[1];
		Transport_ip = SERVER_INFO[2];
			
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'	 					:'InitializeNewConnection',
				 'ACCT_UUID' 					:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 					:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'INPUT_HOST_IP'				:$("#INPUT_HOST_IP").val(),
				 'INPUT_HOST_USER'				:$("#INPUT_HOST_USER").val(),
				 'INPUT_HOST_PASS' 				:$("#INPUT_HOST_PASS").val(),
				 'TRANSPORT_UUID' 			:SERV_UUID		 
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
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
							window.location.href = "MgmtCloudConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.type_danger,
						message: jso.Msg,						
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
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	    
	}
	
	/* Update Connection */
	function UpdateConnection(){
		
		SERVER_INFO = $("#TRANSPORT_SERV_INFO").val().split('|');
		OPEN_UUID = SERVER_INFO[0];
		SERV_UUID = SERVER_INFO[1];
		Transport_ip = SERVER_INFO[2];
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'	 					:'UpdateConnection',
				 'CLUSTER_UUID'					:'<?php echo isset($_SESSION['CLUSTER_UUID'])?$_SESSION['CLUSTER_UUID']:null; ?>',
				 'INPUT_HOST_IP'				:$("#INPUT_HOST_IP").val(),
				 'INPUT_HOST_USER'				:$("#INPUT_HOST_USER").val(),
				 'INPUT_HOST_PASS' 				:$("#INPUT_HOST_PASS").val(),
				 'TRANSPORT_UUID' 				:SERV_UUID		
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
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
							window.location.href = "MgmtCloudConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.type_danger,
						message: jso.Msg,						
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
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	
	}
	
	/*
		Submit Trigger
	*/
	$(function(){
		$("#BackToMgmt").click(function(){
			window.location.href = "SelectCloudRegistration";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtCloudConnection";
		})
		
		$("#TestPackerConn").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#TestPackerConn').prop('disabled', true);
			$('#TestPackerConn').removeClass('btn-primary').addClass('btn-default');
			TestServiceConnection();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if ($URL_SEGMENT == 'AddVMWareConnection')
			{				
				InitializeNewConnection();
			}
			else
			{
				UpdateConnection();
			}
		})		
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img src="data:image/png;base64,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">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			<div id='EDIT_HOST_INFO'></div>
			
			<label for='comment'><?php echo _("ESX IP / Name:"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_HOST_IP' class='form-control' value='' placeholder='Address'>	
			</div>
			
			<label for='comment'><?php echo _("ESX Username:"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_HOST_USER' class='form-control' value='' placeholder='Username'>	
			</div>
			
			<label for='comment'><?php echo _("ESX Password:"); ?></label>
			<div class='form-group has-primary'>				
				<div class="input-group"><input type='password' id='INPUT_HOST_PASS' class='form-control pwd' value='' placeholder='Password'><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>	
			</div>
			
			<label for='comment'><?php echo _("Transport Server:"); ?></label>
			<div class='form-group has-primary'>	
				<select id="TRANSPORT_SERV_INFO" class="selectpicker" data-width="100%"></select>
			</div>
		</div>
		
		<div id='title_block_wizard'>		
			<div class="btn-toolbar">
				<button id='BackToMgmt' 	 class='btn btn-default pull-left  btn-lg'><?php echo _("Back"); ?></button>
				<button id='CancelToMgmt' 	 class='btn btn-default pull-left  btn-lg'><?php echo _("Cancel"); ?></button>
				<button id='CheckAndSubmit' class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
				<button id='TestPackerConn'  class='btn btn-default pull-right btn-lg' disabled><?php echo _("Test Connection"); ?></button>
			</div>
		</div>
					
	</div> <!-- id: wrapper_block-->
</div>

<script>
	/* EYE PASSWORD */
	$(".reveal").mousedown(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'text'));
	})
	.mouseup(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'password'));
	})
	.mouseout(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'password'));
	});
</script>
