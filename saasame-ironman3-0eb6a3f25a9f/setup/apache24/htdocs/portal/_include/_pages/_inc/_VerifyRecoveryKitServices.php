<?php if (isset($_SESSION['SERV_UUID'])){$SERV_UUID = $_SESSION['SERV_UUID'];}else{$SERV_UUID = null;} ?>
<script>
	/* Determine URL Segment */
	TRANSPORT_DIRECT_MODE = true;
	DetermineSegment('EditRecoveryKitServices');
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT == $SET_SEGMENT)
		{
			QueryTransportServerInfo();
		}
	}

	/* Query Select Server Information*/
	function QueryTransportServerInfo()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'QueryTransportServerInformation',
				 'SERV_UUID' :'<?php echo $SERV_UUID; ?>'	 
			},
			success:function(jso)
			{
				/* QUERY DIRECT MODE STATUS */
				TRANSPORT_DIRECT_MODE = jso.SERV_INFO.direct_mode;
				
				if (TRANSPORT_DIRECT_MODE == false)
				{
					$('#INPUT_SERV_IP').prop('disabled', true);
				}		
				
				$('#INPUT_SERV_IP').val(jso.SERV_ADDR);
				
				$("#BackToLastPage").remove();
				$("#CheckAndSubmit").text("Update");
			}
		})
	}
	
	/* Select Service */
	function SelectService()
	{
		return 'Loader,Launcher';
	}

	/* Set array unique value */
	function unique(array)
	{
		return array.filter(function(el, index, arr)
		{
			return index === arr.indexOf(el);
		});
	}
	
	/* Verify Service Connection */
	function VerifyServiceConnection(service)
	{
		if (TRANSPORT_DIRECT_MODE == true)
		{
			transport_addr = $('#INPUT_SERV_IP').val();
		}
		else
		{
			transport_addr = '<?php echo $SERV_UUID; ?>';
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
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			if (jso.Code != false)
			{
				transport_addr = transport_addr.split(',');
				transport_addr.splice($.inArray(jso.Msg.prefer_address, transport_addr),1);
				transport_addr.unshift(jso.Msg.prefer_address);
				transport_addr = transport_addr.toString();
				
				$('#INPUT_SERV_IP').prop('disabled', true);
				$('#CheckAndSubmit').prop('disabled', false);
				$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
			}
			else
			{
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
				$('#VerifyService').removeClass('btn-default').addClass('btn-primary');
				$('#VerifyService').prop('disabled', false);	
				$('#CheckAndSubmit').prop('disabled', true);		
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
				 'OPEN_UUID' :'ONPREMISE-00000-LOCK-00000-PREMISEON',
				 'HOST_UUID' :'ONPREMISE-00000-LOCK-00000-PREMISEON',
				 'SERV_ADDR' :transport_addr,
				 'SYST_TYPE' :'WINDOWS',
				 'SELT_SERV' :service,
				 'MGMT_ADDR' :document.domain,
				 'CONN_DEST' :'NA'
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
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtTransportConnection";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = "SelectServRegion";
		})
			
		$("#VerifyService").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#VerifyService').removeClass('btn-primary').addClass('btn-default');
			$('#VerifyService').prop('disabled', true);
			VerifyServiceConnection(SelectService());
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#CheckAndSubmit').prop('disabled', true);
			if ($URL_SEGMENT == 'VerifyRecoveryKitServices')
			{		
				InitializeNewServices(SelectService());
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
					<label for='comment'><?php echo _("FQDN / Host Name / IP address:"); ?></label>
					<div class='form-group'>				
						<input type='text' id='INPUT_SERV_IP' class='form-control' value='' placeholder='Input Address'>	
					</div>
				</div>
				
				<div id='title_block_wizard'>
					<div class='btn-toolbar'>
						<button id='BackToLastPage' 	class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>
						<button id='CancelToMgmt' 		class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>
						<button id='CheckAndSubmit'		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
						<button id='VerifyService' 		class='btn btn-primary pull-right btn-lg'><?php echo _("Verify Service"); ?></button>									
					</div>
				</div>
			</div>
	</div> <!-- id: wrapper_block-->
</div>