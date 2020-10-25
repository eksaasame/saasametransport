<?php if (isset($_SESSION['HOST_UUID'])){$HOST_UUID = $_SESSION['HOST_UUID'];}else{$HOST_UUID = null;} ?>
<script>
	<!-- Begin to Exec Get List -->
	QueryAvailableService();
	
	/* Determine URL Segment */
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT != $SET_SEGMENT)
		{
			submit_label = '<?php echo _("Submit"); ?>';
		}
		else
		{
			QueryHostInfo();
			submit_label = "<?php echo _("Update and Submit"); ?>";
		}
	}

	/* Query Select Server Information*/
	function QueryHostInfo()
	{
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
				/* DISPLAY EDIT HOST INFO */
				$('#EDIT_HOST_INFO').append("<label for='comment'><?php echo _("Host Name"); ?>:</label><div class='form-group has-primary'><input type='text' class='form-control' value='"+jso.HOST_NAME+"' disabled></div>");
					
				/*GET REG ESX INFORMATION */
				$('#INPUT_HOST_IP').val(jso.HOST_SERV.SERV_MISC.ADDR);
				$('#INPUT_HOST_USER').val(jso.HOST_SERV.SERV_MISC.USER);
				$('#INPUT_HOST_PASS').val(jso.HOST_SERV.SERV_MISC.PASS);
				
				/*GET REG SELECT SERVER*/
				$SELECT_SERVER = jso.HOST_SERV.OPEN_UUID+'|'+jso.HOST_SERV.OPEN_HOST;
				
				$("#TRANSPORT_SERV_INFO").val($SELECT_SERVER);
				$('.selectpicker').selectpicker('refresh');
				
				$("#BackToMgmt").remove();		
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

						$('#TRANSPORT_SERV_INFO').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.OPEN_UUID+'|'+this.SERV_UUID, true, true));
					});
					$('.selectpicker').selectpicker('refresh');
					
					DetermineSegment('EditHostVirtual');
					
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
			
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'TestServiceConnection',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_UUID' :SERV_UUID,
				 'HOST_ADDR' :$("#INPUT_HOST_IP").val(),
				 'HOST_USER' :$("#INPUT_HOST_USER").val(),
				 'HOST_PASS' :$("#INPUT_HOST_PASS").val(),
				 'HOST_UUID' :'<?php echo $HOST_UUID; ?>',
				 'SERV_TYPE' :'Virtual Packer'
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code != false)
				{
					$('#INPUT_HOST_IP').prop('disabled', true);
					$('#INPUT_HOST_USER').prop('disabled', true);
					$('#INPUT_HOST_PASS').prop('disabled', true);
					$('#TRANSPORT_SERV_INFO').prop('disabled', true);

					$('#ListVirtualHost').prop('disabled', false);
					$('#ListVirtualHost').removeClass('btn-default').addClass('btn-primary');
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
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			}
		});
	}

	/* Query Virtual Host List */
	function QueryVirtualHostList(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'QueryVirtualHostList',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_UUID' :SERV_UUID,
				 'HOST_ADDR' :$("#INPUT_HOST_IP").val(),
				 'HOST_USER' :$("#INPUT_HOST_USER").val(),
				 'HOST_PASS' :$("#INPUT_HOST_PASS").val(),
				 'SELECT_VM' :'<?php echo $HOST_UUID; ?>'
			},
			success:function(host_list)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _("ESX Virtual Machine List"); ?>',
						cssClass: 'vmhost-dialog',
						message: host_list,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: true,
						buttons:
						[
							{
								label: submit_label,
								cssClass: 'btn-primary',
								action: function(dialogRef)
								{
									$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
									$('#ListVirtualHost').prop('disabled', true);
									$('#ListVirtualHost').removeClass('btn-primary').addClass('btn-default');
								
									SELECT_VM_HOST = [];									
									var RowCollection = $('#VmHostTable').dataTable().$("input[name='select_vm']:checked", {"page": "all"});
									RowCollection.each(function(){SELECT_VM_HOST.push($(this).val());});
																	
									//console.log(SELECT_VM_HOST); /* For Debug */
									dialogRef.close();
									if ($URL_SEGMENT == 'VerifyHostVirtual')
									{
										InitializeNewService();
									}
									else
									{
										UpdateHostInformation();
									}									
								}
							}							
						]				
					});
				}, 0);
			},
			error: function(xhr)
			{
				
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
				 'ACTION'	 		:'InitializeNewService',
				 'ACCT_UUID' 		:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 		:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'OPEN_UUID' 		:OPEN_UUID,
				 'HOST_UUID' 		:SERV_UUID,
				 'SERV_UUID' 		:SERV_UUID,
				 'HOST_ADDR' 		:$("#INPUT_HOST_IP").val(),
				 'HOST_USER' 		:$("#INPUT_HOST_USER").val(),
				 'HOST_PASS' 		:$("#INPUT_HOST_PASS").val(),
				 'SYST_TYPE' 		:'ESX',
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
							window.location.href = "MgmtHostConnection"
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
							}
						}],
					});
					$('#TestPackerConn').removeClass('btn-default').addClass('btn-primary');
					$('#TestPackerConn').prop('disabled', false);
					$('#ListVirtualHost').prop('disabled', true);
					$('#ListVirtualHost').removeClass('btn-primary').addClass('btn-default');
				}		
			},
			error: function(xhr)
			{
			
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
				$('#ListVirtualHost').prop('disabled', true);
        	}
		});	    
	}
	
	/*
		Submit Trigger
	*/
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
		
		$("#ListVirtualHost").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			QueryVirtualHostList();
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
          <li style='width:25%'><a>				  <?php echo _("Step 1 - Select Type"); ?> </a></li>
          <li style='width:25%'><a>				  <?php echo _("Step 2 - Select Connection"); ?></a></li>
          <li style='width:25%'><a>				  <?php echo _("Step 3 - Select Server"); ?></a></li>
          <li style='width:25%' class='active'><a><?php echo _("Step 4 - Verify Host"); ?></a></li>
        </ul>
      </div>
			
			<div id='form_block_wizard'>
				<div id='EDIT_HOST_INFO'></div>
				<label for='comment' style='width:32%;'><?php echo _("ESX IP / Name:"); ?></label>
				<label for='comment' style='width:32%;'><?php echo _("ESX Username:"); ?></label>
				<label for='comment' style='width:31.5%;'><?php echo _("ESX Password:"); ?></label>
				<div class='form-group form-inline'>
					<input type='text' id='INPUT_HOST_IP' class='form-control' value='' placeholder='Address' style='width:32%;'>	
					<input type='text' id='INPUT_HOST_USER' class='form-control' value='' placeholder='Username' style='width:32%;'>	
					<div class="input-group"><input type='password' id='INPUT_HOST_PASS' class='form-control pwd' value='' placeholder='Password' style='width:415px'><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>
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
					<button id='ListVirtualHost' class='btn btn-default pull-right btn-lg' disabled><?php echo _("Select Host"); ?></button>	
					<button id='TestPackerConn'  class='btn btn-default pull-right btn-lg' disabled><?php echo _("Test Connection"); ?></button>
				</div>
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
