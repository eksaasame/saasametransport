<?php if (isset($_SESSION['HOST_UUID'])){$HOST_UUID = $_SESSION['HOST_UUID'];}else{$HOST_UUID = null;} ?>
<script>
	<!-- Begin to Exec Get List -->
	PACKER_DIRECT_MODE = true;
	PRIORITY_ADDR = null;
	QueryAvailableService();
	
	/* Determine URL Segment */
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT == $SET_SEGMENT)
		{
			QueryHostInfo();
		}
	}

	/* Query Select Server Information*/
	function QueryHostInfo(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'QueryHostInfo',
				 'HOST_UUID' :'<?php echo $HOST_UUID; ?>'	 
			},
			success:function(jso)
			{
				/*GET REG HOST IP*/
				$('#INPUT_HOST_IP').val(jso.HOST_ADDR);
				
				/* QUERY DIRECT MODE STATUS */
				PACKER_DIRECT_MODE = jso.HOST_INFO.direct_mode;
			
				if (PACKER_DIRECT_MODE != true)
				{
					$('#INPUT_HOST_IP').prop('disabled', true);
				}
			
				/* Query Packer UUID */
				PACKER_UUID = jso.HOST_INFO.machine_id;

				/* Query Pair Transport Server */
				if (jso.HOST_SERV != false)
				{
					SELECT_SERVER = jso.HOST_SERV.SERV_UUID;
					$("#TRANSPORT_SERV_INFO").val(SELECT_SERVER);
					$('.selectpicker').selectpicker('refresh');
				}
				
				PRIORITY_ADDR = jso.HOST_INFO.priority_addr;
				$('#PRIORITY_SERV_ADDR').append(new Option(PRIORITY_ADDR,PRIORITY_ADDR, true, true));
				$("#PRIORITY_SERV_ADDR").val(PRIORITY_ADDR);
				$('.selectpicker').selectpicker('refresh');				
				
				$("#BackToMgmt").remove();
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
						var HOST_TYPE = (value['VENDOR_NAME'] == 'AzureBlob' && value['SERV_INFO']['is_promote'] == true)?value['HOST_TYPE']:value['VENDOR_NAME'];
						var mapObj = {UnknownVendorType:"General Purpose",OPENSTACK:"OpenStack",OpenStack:"OpenStack",AzureBlob:"Azure",Azure:"Azure",Aliyun:"Alibaba Cloud",Ctyun:"天翼云"};
						HOST_TYPE = HOST_TYPE.replace(/UnknownVendorType|OPENSTACK|AzureBlob|Aliyun|Ctyun/gi, function(matched){return mapObj[matched];});
						
						$('#TRANSPORT_SERV_INFO').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.SERV_UUID, true, true));
					});
					$('.selectpicker').selectpicker('refresh');
					DetermineSegment('EditHostPhysical');

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
			SERV_UUID = $("#TRANSPORT_SERV_INFO").val();
	
			if (PACKER_DIRECT_MODE == true)
			{
				PACK_NIC_ADDR = $("#INPUT_HOST_IP").val();
			}
			else
			{
				PACK_NIC_ADDR = PACKER_UUID;
			}		
				
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'TestServiceConnection',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_UUID' :SERV_UUID,
				 'HOST_ADDR' :PACK_NIC_ADDR,
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
					
					/* Carrir Priority Address */
					$('#PRIORITY_SERV_ADDR').empty();
					$.each(jso.Server_Addr, function(key,value)
					{
						$('#PRIORITY_SERV_ADDR').append(new Option(value,value, true, true));
					});
					
					$("#PRIORITY_SERV_ADDR").val(PRIORITY_ADDR);					
					$('#PRIORITY_SERV_ADDR').prop('disabled', false);
					$('.selectpicker').selectpicker('refresh');

					$('#INPUT_HOST_IP').prop('disabled', true);
					$('#TRANSPORT_SERV_INFO').prop('disabled', true);
					
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _('Service Message'); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.Msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
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
				 'ACTION'	 	:'InitializeNewService',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 	:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' 	:'',
				 'SERV_UUID' 	:SERV_UUID,
				 'HOST_ADDR' 	:PACK_NIC_ADDR,
				 'HOST_USER' 	:'',
				 'HOST_PASS' 	:'',
				 'SYST_TYPE' 	:'',
				 'SERV_TYPE' 	:'Physical Packer',
				 'OS_TYPE'	 	:OS_TYPE,
				 'PRIORITY_ADDR':$("#PRIORITY_SERV_ADDR").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');							
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _('Service Message'); ?>',
						message: jso.Msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
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
						title: '<?php echo _('Service Message'); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					
					$('#INPUT_HOST_IP').prop('disabled', false);
					$('#TRANSPORT_SERV_INFO').prop('disabled', false);
					
					$('#TestPackerConn').prop('disabled', false);
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
					
					$('#CheckAndSubmit').prop('disabled', true);
					$('#CheckAndSubmit').removeClass('btn-primary').addClass('btn-default');
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
				 'ACTION'	 	:'UpdateHostInformation',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 	:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' 	:'',				 
				 'SERV_UUID' 	:SERV_UUID,
				 'HOST_UUID' 	:'<?php echo $HOST_UUID; ?>',
				 'HOST_ADDR' 	:PACK_NIC_ADDR,
				 'HOST_USER' 	:'',
				 'HOST_PASS' 	:'',
				 'SERV_TYPE' 	:'Physical Packer',
				 'OS_TYPE'	 	:OS_TYPE,
				 'PRIORITY_ADDR':$("#PRIORITY_SERV_ADDR").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _('Service Message'); ?>',
						message: jso.Msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
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
						title: '<?php echo _('Service Message'); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,
						buttons:[{
							label: '<?php echo _('Close'); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#INPUT_HOST_IP').prop('disabled', false);
					$('#TestPackerConn').prop('disabled', false);
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
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
		$("#BackToMgmt").click(function(){
			window.location.href = "SelectHostType";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtHostConnection";
		})
		
		$("#TestPackerConn").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#TestPackerConn').prop('disabled', true);
			$('#TestPackerConn').removeClass('btn-primary').addClass('btn-default');
			TestServiceConnection();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#CheckAndSubmit').prop('disabled', true);
			if ($URL_SEGMENT == 'VerifyHostPhysical')
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
          <li style='width:25%'><a>					<?php echo _("Step 1 - Select Type"); ?> </a></li>
          <li style='width:25%'><a>					<?php echo _("Step 2 - Select Connection"); ?></a></li>
          <li style='width:25%'><a>					<?php echo _("Step 3 - Select Server"); ?></a></li>
          <li style='width:25%' class='active'><a>	<?php echo _("Step 4 - Verify Host"); ?></a></li>
        </ul>
      </div>
			
			<div id='form_block_wizard'>
				<label for='comment'><?php echo _("FQDN / Host Name / IP Address:"); ?></label>
				<div class='form-group has-primary'>			
					<input type='text' id='INPUT_HOST_IP' class='form-control' value='' placeholder='Address'>	
				</div>
				
				<label for='comment'><?php echo _("Transport Server:"); ?></label>
				<label for='comment' style='position: absolute; right:43px; width: 200px;'><?php echo _("Preferred Address:"); ?></label>
				<div class='form-group has-primary'>				
					<select id="TRANSPORT_SERV_INFO" class="selectpicker" data-width="80%"></select>
					<select id="PRIORITY_SERV_ADDR" class="selectpicker" data-width="19.5%" disabled></select>
				</div>
			</div>

			<div id='title_block_wizard'>
				<div class="btn-toolbar">
					<button id='BackToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Back"); ?></button>
					<button id='CancelToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Cancel"); ?></button>
					<button id='CheckAndSubmit' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
					<button id='TestPackerConn' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Test Connection"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>
