<script>

function ShowErrorDialog( Msg ){
	BootstrapDialog.show({
		title: '<?php echo _("Service Message"); ?>',
		message: Msg,
		draggable: true,
		closable: false,
		buttons:[{
			label: '<?php echo _("Close"); ?>',
			action: function(dialogRef){
			dialogRef.close();
			}
		}]
	});
}

	function SummaryPageRoute(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SummaryPageRoute',
				 'SERV_REGN' :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'RECY_TYPE' :'<?php echo $_SESSION['RECY_TYPE']; ?>',
				 'CLOUD_TYPE' :'<?php echo $_SESSION['CLOUD_TYPE']; ?>'
			},
			success:function(jso)
			{
				RouteBack = jso.Back;
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	function GetResourceGroup(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    :'QueryTransportInformation',
				 'VM_REGN' :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'VM_NAME':'<?php echo $_SESSION['SERV_UUID']; ?>',
				 'SERV_UUID':'<?php echo $_SESSION['SERVER_UUID']; ?>'
			},
			success:function(jso)
			{
				resource_group = jso.Resource_group;
				ListInstanceFlavors();
				ListAvailableNetwork('<?php echo $_SESSION['SERV_REGN']; ?>');
				ListSecurityGroup();
				QueryServerInformation();
				
				if( typeof publicIp !== 'undefined' && publicIp != '' )
					$("#PUBLIC_IP").val( publicIp );
				
				if( typeof privateIp !== 'undefined' && privateIp != '' )
					$("#SPECIFIED_PRIVATE_IP").val( privateIp );
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* List Instance Flavors */
	function ListInstanceFlavors(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION':'ListInstanceFlavors',
				 'SERV_REGN' :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#FLAVOR_LIST').append(new Option(value.name+' (CPU:'+value.numberOfCores+'Core, Memory:'+value.memoryInMB+'MB)', value.name, true, false));
						
						if (value.name == 'Standard_A2')
						{
							$("#FLAVOR_LIST").val(value.name);
						}
						
						if ( FLAVOR_ID != '' )
						{
							$("#FLAVOR_LIST").val( FLAVOR_ID );
						}
						
					});
					$('.selectpicker').selectpicker('refresh');
				}
				else
				{
				
				}
			},
			error: function(xhr)
			{
			
			}
		});
	}
	
	/* List Internal Network */
	function ListAvailableNetwork(SELECT_ZONE){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'      :'ListAvailableNetwork',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE' :SELECT_ZONE
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$('#NETWORK_LIST').attr('disabled', false);
					$('#NETWORK_LIST')[0].options.length = 0;
					$.each(jso, function(key,value)
					{
						var netInfo = value.id.split("/");
						var display = netInfo[8]+'|'+netInfo[10];
						var data = netInfo[8]+'|'+netInfo[10]+'|'+netInfo[4];
						
						if( netInfo[4].toLowerCase() == resource_group.toLowerCase() )
							$('#NETWORK_LIST').append(new Option(display, JSON.stringify( value ), true, false));											
					});
					
					if (NETWORK_UUID != '')
					{
						$("#NETWORK_LIST").val( NETWORK_UUID );
					}
					
					$('.selectpicker').selectpicker('refresh');			
				}				
				
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	function QueryServerInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryServerInformation',
				 'SERV_UUID' : <?php echo '"'.$_SESSION['SERVER_UUID'].'"'; ?>
			},
			success:function(jso)
			{
				is_azure_mgmt_disk = jso.SERV_INFO.is_azure_mgmt_disk;
				ListAvailabilitySet('<?php echo $_SESSION['SERV_REGN']; ?>');
				
				if( !is_azure_mgmt_disk ){
					$('#Disk_Type').prop('disabled', true);
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
		
	/* List Internal Network */
	function ListAvailabilitySet(SELECT_ZONE){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'      :'ListAvailabilitySet',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'RESOURCE_GROUP': resource_group ,
				 'SELECT_ZONE' :SELECT_ZONE
			},
			success:function( object )
			{
				$('#Availability_Set').append(new Option( 'no use' , false, true, false));
				
				if (object != false)
				{
					$.each( object.value , function(key,value)
					{
						if( is_azure_mgmt_disk == false && value.sku.name == "Classic"){
							$('#Availability_Set').append(new Option( value.name , value.id, true, false));	
						}
						else if( is_azure_mgmt_disk == true && value.sku.name == "Aligned" ){
							$('#Availability_Set').append(new Option( value.name , value.id, true, false));	
						}
															
					});
					
					if ( availabilitySet != '' )
					{
						var t = availabilitySet.split("/");
						$("#Availability_Set").val( availabilitySet );
					}
						
					$('.selectpicker').selectpicker('refresh');			
				}				
				
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	/* List Security Group */
	function ListSecurityGroup(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    :'ListSecurityGroup',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SERVER_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>',
			},
			success:function(jso)
			{
				if (jso != false)
				{					
					$.each(jso, function(key,value)
					{
						var sgInfo = value.id.split("/");
						if( sgInfo[4].toLowerCase() == resource_group.toLowerCase() )
							$('#SECURITY_GROUP').append(new Option(value.name, value.id, true, false));
					});
					
					if (SGROUP_UUID != '')
					{
						$("#SECURITY_GROUP").val(SGROUP_UUID);
					}
					
					$('.selectpicker').selectpicker('refresh');	
				}
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	function checkInstanceSupportSSD(){
		
		var vmType = $("#FLAVOR_LIST").val();
		vmType = vmType.replace("Standard_", "");
		vmType = vmType.replace("Basic_", "");
		
		if( vmType.toLowerCase().search("s") == -1 && $("#Disk_Type").val() == "SSD"){
			BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				message: $("#FLAVOR_LIST").val()+' dose not support SSD.',
				draggable: true,
				closable: false,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}]
			});
			
			return false;
		}
		
		return true;
	}
	
	function checkIpInSubnet( cidr ){
		
		if( $("#SPECIFIED_PRIVATE_IP").val() === "" ){
			return true;
		}
		
		ret = inSubNet( $("#SPECIFIED_PRIVATE_IP").val(), cidr );
		
		if( ret === false ){
			
			BootstrapDialog.show({
				title: '<?php echo ("Service Message"); ?>',
				message: $("#SPECIFIED_PRIVATE_IP").val()+" <?php echo _("is not in range of"); ?> "+cidr,
				draggable: true,
				closable: false,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}]
			});
		}
		
		return inSubNet( $("#SPECIFIED_PRIVATE_IP").val(), cidr );
	}
	
	function validateAddress( string ){

		if( /^[0-9]*$|[^a-zA-Z0-9\-\/]|[-]$/.test( string ) ) 
			return false;
		
		if( string.length > 60 || string.length < 2 )
			return false;
		
		return true;     
	}

	function getInstanceName( ){

		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    		:'ListInstancesInResourceGroup',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'ServerId'			:'<?php echo $_SESSION['SERVER_UUID']; ?>'
			},
			success:function( response )
			{
				InstancesInfo = response;
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	function checkInstanceName( Name ){
		
		for( i = 0; i < InstancesInfo.value.length; i++ ){
			
			var temp = InstancesInfo.value[i].id.split("/");
			if( temp[ temp.length -1] == Name )
				return false;
		}
		
		return true;
	}
	
	/* Post Select Service UUID To Session */
	function SelectAzureInstanceConfig(){
		
		var string = $("#HOSTNAME_TAG").val();
		
		if( !validateAddress( $("#HOSTNAME_TAG").val() ) ){
			ShowErrorDialog( "<?php echo  _('Instance name is not vaild.'); ?>" );
			return false;
		}

		var info = JSON.parse( $("#NETWORK_LIST").val() );
		
		if( !checkIpInSubnet( info.properties.addressPrefix ) )
			return false;
		
		if( !checkInstanceSupportSSD() )
			return false;
		
		if( !checkInstanceName( $("#HOSTNAME_TAG").val() ) ){
			ShowErrorDialog( "<?php echo  _('The instance name has be used.'); ?>" );
			return false;
		}

		var netInfo = info.id.split("/");
		var network = netInfo[8]+'|'+netInfo[10]+'|'+netInfo[4];
		
		/* DOM SERVUCE SETTINGS */
		var SERVICE_SETTINGS = {
								'flavor_id':$("#FLAVOR_LIST").val(),
								'network_uuid':network,
								'sgroup_uuid':$("#SECURITY_GROUP").val(),
								'elastic_address_id':$("#PUBLIC_IP").val(),
								'private_address_id':$("#SPECIFIED_PRIVATE_IP").val(),
								'rcvy_pre_script':$("#RECOVERY_PRE_SCRIPT").val(),
								'rcvy_post_script':$("#RECOVERY_POST_SCRIPT").val(),
								'disk_type':$("#Disk_Type").val(),
								'availability_set':$("#Availability_Set").val(),
								'hostname_tag':$("#HOSTNAME_TAG").val(),
								'datamode_instance': $("#DATAMODE_INSTANCE").val(),
								'datamode_power':$('#DATAMODE_POWER:checkbox:checked').val()
							  };
							  
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    		:'ServiceConfiguration',
				 'SERVICE_SETTINGS'	:SERVICE_SETTINGS
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = NextPage;
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	function checkPrivateIp(){
		
		var info = JSON.parse( $("#NETWORK_LIST").val() );
		
		var netInfo = info.id.split("/");
		var network = netInfo[8]+'|'+netInfo[10]+'|'+netInfo[4];
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'    		:'CheckPrivateIp',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'NETWORK_UUID'		:network,
				 'PRIVATE_IP' 		:$("#SPECIFIED_PRIVATE_IP").val()
			},
			success:function(jso)
			{
				if (jso.available == true)
					SelectAzureInstanceConfig();
				else{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: '<?php echo _("cannot use this private ip."); ?>',
						draggable: true,
						closable: false,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}]
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* List Recovery Script */
	function ListRecoveryScript(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'ListRecoveryScript',
				 'FILTER_TYPE':'<?php echo $_SESSION['REPL_OS_TYPE']; ?>'
			},
			success:function(jso)
			{
				$("#RECOVERY_PRE_SCRIPT").empty();
				$("#RECOVERY_POST_SCRIPT").empty();
				
				$('#RECOVERY_PRE_SCRIPT').append(new Option('- - - <?php echo _('Select Pre Script'); ?> - - -', '', false, false));
				$('#RECOVERY_POST_SCRIPT').append(new Option('- - - <?php echo _('Select Post Script'); ?> - - -', '', false, false));
							
				$.each(jso, function(key,value)
				{
					$('#RECOVERY_PRE_SCRIPT').append(new Option(key, key, false, false));						
					$('#RECOVERY_POST_SCRIPT').append(new Option(key, key, false, false));
				});
				
				$("#RECOVERY_PRE_SCRIPT").val(RCVY_PRE_SCRIPT);
				$("#RECOVERY_POST_SCRIPT").val(RCVY_POST_SCRIPT);
				
				$('.selectpicker').selectpicker('refresh');

				$('#MgmtRecoveryScript').removeClass('btn-default').addClass('btn-success');
				$('#MgmtRecoveryScript').prop('disabled', false);				
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	/* Query Recovery Script */
	function MgmtRecoveryScript(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'MgmtRecoveryScript',
			},
			success:function(recovery_script)
			{
				$('#MgmtRecoveryScript').prop('disabled', true);
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _("Recovery Script"); ?>',
						cssClass: 'recovery-script-dialog',
						message: recovery_script,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: false,
						onhidden: function(dialogRef){
							$('#MgmtRecoveryScript').prop('disabled', false);
						},
						buttons:
						[
							{
								label: '<?php echo _("Browse..."); ?>',
								id: 'BrowseFileBtn',
								cssClass: 'btn-success pull-left'								
							},							
							{
								label: '<?php echo _("Upload"); ?>',
								id: 'FileUploadBtn',
								cssClass: 'btn-warning pull-left',
								action: function(dialogRef)
								{
									FileUpload();									
								}
							},
							{
								label: '<?php echo _("Delete"); ?>',
								id: 'FileDeleteBtn',
								cssClass: 'btn-danger pull-left',
								action: function(dialogRef)
								{
									SELECT_FILES = [];
									if ($.fn.dataTable.isDataTable == true)
									{
										var RowCollection = $("#RecoveryScriptTable").dataTable().$("input[name=select_file]:checked", {"page": "all"});
									}
									else
									{
										var RowCollection = $("input[name=select_file]:checked");
									}
									
									RowCollection.each(function(){SELECT_FILES.push($(this).val());});
									DeleteFiles(SELECT_FILES);																						
								}
							},
							{
								label: '<?php echo _("Close"); ?>',
								id: 'CloseBtn',
								cssClass: 'btn-primary',
								action: function(dialogRef)
								{
									dialogRef.close();
								}
							},							
						]				
					});
				}, 0);
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	function DeleteFiles(SELECT_FILES)
	{
		$.ajax({
			url: '_include/_exec/mgmt_service.php',
			type:"POST",
			data:{
				 'ACTION'	   :'DeleteFiles',
				 'SELECT_FILES':SELECT_FILES
			},			
			beforeSend:function(){
				BtnOptions(true);
			},   
			success:function(data)
			{
				BtnOptions(false);
				ReloadMgmtRecoveryScript();
				ListRecoveryScript();
			}
		});
	}
	
	/* Button Options */
	function BtnOptions(DisableOption)
	{
		if (DisableOption == true)
		{
			$("#LoadingOverLay").addClass("TransparentOverlay SpinnerLoading");
			$('#BrowseFileBtn').removeClass('btn-success').addClass('btn-default');
			$('#BrowseFileBtn').prop('disabled', true);
			$('#FileUploadBtn').removeClass('btn-warning').addClass('btn-default');
			$('#FileUploadBtn').prop('disabled', true);
			$('#FileDeleteBtn').removeClass('btn-danger').addClass('btn-default');
			$('#FileDeleteBtn').prop('disabled', true);
			$('#CloseBtn').removeClass('btn-primary').addClass('btn-default');
			$('#CloseBtn').prop('disabled', true);	
		}
		else
		{
			$('#LoadingOverLay').removeClass('TransparentOverlay SpinnerLoading');
			$('#BrowseFileBtn').removeClass('btn-default').addClass('btn-success');
			$('#BrowseFileBtn').prop('disabled', false);						
			$('#CloseBtn').removeClass('btn-default').addClass('btn-primary');
			$('#CloseBtn').prop('disabled', false);			
			$("#BrowseFileBtn").text('<?php echo _("Browse..."); ?>').button("refresh");
		}
	}
	
	/* Reload Mgmt Recovery Script */
	function ReloadMgmtRecoveryScript(){		
		$("#RecoveryScriptTable").empty();
		$("#RecoveryScriptDiv").empty();
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'ReloadMgmtRecoveryScript',
			},
			beforeSend:function(){
				
			},
			success:function(data)
			{
				var recovery_script = $($.parseHTML(data)).filter("#RecoveryScriptTable");	
				$('#RecoveryScriptDiv').append(recovery_script);
				
				recovery_script_count = $('#RecoveryScriptTable tbody tr').length;
				if (recovery_script_count > 10)
				{
					$("#RecoveryScriptTable").DataTable({
						paging:true,
						ordering:false,
						searching:false,
						bLengthChange:false,
						pageLength:10,
						pagingType:"simple_numbers"						
					});
				}
				else
				{
					if ($.fn.DataTable.isDataTable('#RecoveryScriptTable')) 
					{
						$("#RecoveryScriptTable").DataTable({ordering:false,destroy:true}).destroy();
					}
				}
				
				if ($('#RecoveryScriptTable td input:checkbox:first').val() == '')
				{
					$('#FileDeleteBtn').removeClass('btn-danger').addClass('btn-default');
					$('#FileDeleteBtn').prop('disabled', true);
					$('#checkAll').prop('disabled', true);
				}
				else
				{
					$('#FileDeleteBtn').removeClass('btn-default').addClass('btn-danger');
					$('#FileDeleteBtn').prop('disabled', false);
					$('#checkAll').prop('disabled', false);
				}
				
				$('#FileUploadBtn').removeClass('btn-warning').addClass('btn-default');
				$('#FileUploadBtn').prop('disabled', true);
				
				$("#checkAll").click(function() {$("#RecoveryScriptDiv input:checkbox:enabled").prop("checked", this.checked);});
				$('#checkAll').prop('checked', false);
			}
		});
	}
	
	
	/* File Upload */
	function FileUpload()
	{
		var file = document.getElementById('UploadFile').files[0];
		
		if (file.size < MaxUploadSize)
		{		
			var form_data = new FormData();
			form_data.append("ACTION",'FileUpload');
			form_data.append("UploadFile", file);
		
			$.ajax({
				url: '_include/_exec/mgmt_service.php',
				type:"POST",
				data:form_data,
				contentType: false,
				cache: false,
				processData: false,
				beforeSend:function(){
					BtnOptions(true);	
				},
				success:function(data)
				{
					var obj = jQuery.parseJSON(data);
					BtnOptions(false);
					ReloadMgmtRecoveryScript();
					ListRecoveryScript();
					if (obj.status == false)
					{
						BootstrapDialog.show({
							title: '<?php echo _("File Upload Message"); ?>',
							message: obj.reason,
							type: BootstrapDialog.TYPE_PRIMARY,
							draggable: true,
							buttons:[{
								label: '<?php echo _("Close"); ?>',
								action: function(dialogRef){
								dialogRef.close();
								}
							}],					
						});
					}
				}			
			});
		}
		else
		{
			BtnOptions(false);
			ReloadMgmtRecoveryScript();
			ListRecoveryScript();
			BootstrapDialog.show({
				title: '<?php echo _("File Upload Message"); ?>',
				message: '<?php echo _("Max file upload size is ").ini_get("upload_max_filesize"); ?>B.',
				type: BootstrapDialog.TYPE_PRIMARY,
				draggable: true,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}],					
			});
		}
	}
	
	/* Submit Trigger */
	$(function(){
		$("#MgmtRecoveryScript").click(function(){
			MgmtRecoveryScript();
		})
		
		$("#BackToMgmtRecoverWorkload").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToServiceList").click(function(){
			window.location.href = RouteBack;
		})
		
		$("#SelectAzureInstanceConfig").click(function(){
			
			if( $("#SPECIFIED_PRIVATE_IP").val() == "" )
				SelectAzureInstanceConfig();
			else
				checkPrivateIp();
			
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = 'PlanSelectRecoverType';
		})
		
		$("#NextPage").click(function(){
			
			if( $("#SPECIFIED_PRIVATE_IP").val() == "" )
				SelectAzureInstanceConfig();
			else
				checkPrivateIp();
		})
		
		$("#ConfigureDataModeAgent").click(function(){
			ConfigureDataModeAgent();
		})
	});

	/* List Data Mode Instance */
	function ListDataModeInstances(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION':'ListDataModeInstances',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'FILTER_UUID':'<?php echo $_SESSION['SERVER_UUID']; ?>'
			},
			beforeSend: function() {
				$('#DATAMODE_INSTANCE').empty();
			},
			success:function(jso)
			{
				HOST_TYPE = '<?php echo isset($_SESSION['HOST_TYPE'])?$_SESSION['HOST_TYPE']:null; ?>';
				
				$('#DATAMODE_INSTANCE').append(new Option('<?php echo _('No Associated'); ?>', 'NoAssociatedDataModeInstance', true, false));
				
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#DATAMODE_INSTANCE').append(new Option(value.name, value.id, true, false));						
					});
					$('.selectpicker').selectpicker('refresh');	
				}
				
				$('#SelectInstanceConfig').prop('disabled', false);
				$('#SelectInstanceConfig').removeClass('btn-default').addClass('btn-primary');

				$('#BackToServiceList').prop('disabled', false);
				$('#BackToMgmtRecoverWorkload').prop('disabled', false);
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Set Boot CheckBox */
	function SetBootCheckBox(){
		if ($("#DATAMODE_INSTANCE").val() == 'NoAssociatedDataModeInstance')
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', false);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', false);
			
			$('#MgmtRecoveryScript').prop('disabled', false);
			$('#MgmtRecoveryScript').removeClass('btn-default').addClass('btn-success');
			
			$('#ConfigureDataModeAgent').prop('disabled', true);
			$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
				
			$('#FLAVOR_LIST').prop('disabled', false);
			$('#SECURITY_GROUP').prop('disabled', false);
			$('#HOSTNAME_TAG').prop('disabled', false);
			
			$('#SUBNET_LIST').prop('disabled', false);
			$('#PRIVATE_ADDR').prop('disabled', false);
			$('#FLOATING_ADDR').prop('disabled', false);

			$('#Availability_Set').prop('disabled', false);
			$('#Disk_Type').prop('disabled', false);
			$('#PUBLIC_IP').prop('disabled', false);
			$('#NETWORK_LIST').prop('disabled', false);
			$('#SPECIFIED_PRIVATE_IP').prop('disabled', false);		

			$('#ConfigureDataModeAgent').prop('disabled', true);
			$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
			
			$('#DATAMODE_POWER').bootstrapToggle('enable');
			$('#DATAMODE_POWER').bootstrapToggle('off');
			$("#DATAMODE_POWER").bootstrapToggle('disable');
		}
		else
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', true);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', true);
			
			$('#MgmtRecoveryScript').prop('disabled', true);
			$('#MgmtRecoveryScript').removeClass('btn-success').addClass('btn-default');
			
			$('#ConfigureDataModeAgent').prop('disabled', false);
			$('#ConfigureDataModeAgent').removeClass('btn-default').addClass('btn-warning');
			
			$('#FLAVOR_LIST').prop('disabled', true);
			$('#SECURITY_GROUP').prop('disabled', true);
			$('#HOSTNAME_TAG').prop('disabled', true);
			
			$('#SUBNET_LIST').prop('disabled', true);
			$('#PRIVATE_ADDR').prop('disabled', true);
			$('#FLOATING_ADDR').prop('disabled', true);	

			$('#Availability_Set').prop('disabled', true);
			$('#Disk_Type').prop('disabled', true);
			$('#PUBLIC_IP').prop('disabled', true);
			$('#NETWORK_LIST').prop('disabled', true);
			$('#SPECIFIED_PRIVATE_IP').prop('disabled', true);	
			
			if (HOST_TYPE == 'Physical')
			{
				$('#ConfigureDataModeAgent').prop('disabled', true);
				$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
				
			}
			
			$("#DATAMODE_POWER").bootstrapToggle('enable');
			$('#DATAMODE_POWER').bootstrapToggle('on');
		}
		$('.selectpicker').selectpicker('refresh');
	}
	
	/* Configure DataMode Agent */
	function ConfigureDataModeAgent(){
		
		$.ajax({
			url: '_include/_exec/mgmt_service.php',
			type:"POST",
			dataType:'TEXT',
			data:{
				 'ACTION'		:'ConfigureDataModeAgent',
				 'REPL_UUID'	:'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'DISK_FILTER'	:'<?php echo isset($_SESSION['SNAP_UUID'])?$_SESSION['SNAP_UUID']:$_SESSION['VOLUME_UUID']; ?>',
				 'RECOVERY_MODE':'<?php echo $_SESSION['RECY_TYPE']; ?>'
			},   
			success:function(data)
			{
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _('Data Mode'); ?>',
						cssClass: 'partition-agent-dialog',
						message: data,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: true,
						onhidden: function(dialogRef){							
							$('#ConfigureDataModeAgent').removeClass("btn-default").addClass("btn-warning")
							$('#ConfigureDataModeAgent').prop('disabled', false);
						},
						onshown: function(dialogRef){
							$('#ConfigureDataModeAgent').removeClass("btn-warning").addClass("btn-default")
							$('#ConfigureDataModeAgent').prop('disabled', true);
						},
						buttons:
						[
							{
								id: 'AgentCfgBtn',
								label: '<?php echo _('Download'); ?>',
								cssClass: 'btn-default',
								action: function(dialogRef)
								{
									id = 0;
									var PartitionArray = new Array();
									$("#MappingTable").find("tbody > tr").each(function(){
										PartitionInfo = {guid:$("#guid"+id).text(),
														 is_system_disk:$("#is_system_disk"+id).text(),
														 partition_number:$("#partition_number"+id).text(),
														 partition_offset:$("#partition_offset"+id).text(),
														 partition_size:$("#partition_size"+id).text(), 
														 partition_style:$("#partition_style"+id).text(),
														 signature:$("#signature"+id).text(),
														 path:$("#set_letter"+id).val()};
										
										PartitionArray[id] = PartitionInfo;
										id++;
									});
									
									PartitionArray = {partitions:PartitionArray}
									//console.log(PartitionArray);
									PARTITION_REF = JSON.stringify(PartitionArray);
									GenerateDataModeAgentPackage();
									dialogRef.close();
									
								}
							}
						]				
					});
				}, 0);
			}
		});
	}
</script>