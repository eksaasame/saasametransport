<?php
if (isset($_SESSION['EDIT_PLAN']))
{
	$REPL_UUID		  = $_SESSION['EDIT_PLAN'] -> ReplUUID;
	
	$_SESSION['CLUSTER_UUID'] 	= $_SESSION['EDIT_PLAN'] -> ClusterUUID;
	$_SESSION['SERV_REGN']	  	= $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$_SESSION['REPL_OS_TYPE'] 	= $_SESSION['EDIT_PLAN'] -> OSType;
	$_SESSION['HOST_TYPE']	  	= $_SESSION['EDIT_PLAN'] -> HostType;
	
	$FLAVOR_ID    				= $_SESSION['EDIT_PLAN'] -> flavor_id;
	$NETWORK_UUID 				= $_SESSION['EDIT_PLAN'] -> network_uuid;
	$SGROUP_UUID  	 			= $_SESSION['EDIT_PLAN'] -> sgroup_uuid;
	$RCVY_PRE_SCRIPT 			= $_SESSION['EDIT_PLAN'] -> rcvy_pre_script;
	$RCVY_POST_SCRIPT			= $_SESSION['EDIT_PLAN'] -> rcvy_post_script;
	$ELASTIC_ADDRESS 			= $_SESSION['EDIT_PLAN'] -> elastic_address_id;
	$PRIVATE_ADDRESS			= $_SESSION['EDIT_PLAN'] -> private_address_id;
	$HOSTNAME_TAG 	 			= $_SESSION['EDIT_PLAN'] -> hostname_tag;
	$TRANSPORT_UUID	 			= $_SESSION['EDIT_PLAN'] -> TransportUUID;
	$DATAMODE_INSTANCE 			= $_SESSION['EDIT_PLAN'] -> datamode_instance;
	$BOOTABLE_CHECKED			= $_SESSION['EDIT_PLAN'] -> is_datamode_boot;
}
else
{
	$REPL_UUID		  	= $_SESSION['REPL_UUID'];
	
	$FLAVOR_ID 		  	= 'xyzzy';
	$NETWORK_UUID 	  	= 'xyzzy';
	$SGROUP_UUID 	  	= 'xyzzy';
	$RCVY_PRE_SCRIPT  	= 'xyzzy';
	$RCVY_POST_SCRIPT 	= 'xyzzy';
	$ELASTIC_ADDRESS 	= 'xyzzy';
	$PRIVATE_ADDRESS	= 'xyzzy';
	$HOSTNAME_TAG	  	= $_SESSION['HOST_NAME'];
	$TRANSPORT_UUID	  	= $_SESSION['SERV_UUID'];
	$DATAMODE_INSTANCE	= 'NoAssociatedDataModeInstance';
	$BOOTABLE_CHECKED	= false;
}
?>

<script>
$( document ).ready(function() {
	/* Define Portal Max Upload Size */
	MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;
	
	/* Begin to Automatic Exec */
	ListRecoveryScript();
	ListInstanceFlavors();
	ListAvailableNetwork();
	ListElasticAddresses();
	ListDataModeInstance();
		
	DetermineSegment('EditPlanInstanceAwsConfigurations');
	/* Determine URL Segment */
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			BackToLastPage  = 'EditPlanSelectListEbsSnapshot';
			NextPage		= 'EditPlanRecoverAwsSummary';
		}
		else
		{
			BackToLastPage 	= 'PlanSelectListEbsSnapshot';
			NextPage		= 'PlanRecoverAwsSummary';
		}
	}
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#NextPage').prop('disabled', false);
		$('#NextPage').removeClass('btn-default').addClass('btn-primary');

		$('#BackToLastPage').prop('disabled', false);
		$('#CancelToMgmt').prop('disabled', false);
	});
	
	FLAVOR_ID 		 = '<?php echo $FLAVOR_ID; ?>';
	NETWORK_UUID 	 = '<?php echo $NETWORK_UUID; ?>';
	SGROUP_UUID 	 = '<?php echo $SGROUP_UUID; ?>';
	ELASTIC_ADDRESS  = '<?php echo $ELASTIC_ADDRESS; ?>';
	PRIVATE_ADDRESS  = '<?php echo $PRIVATE_ADDRESS; ?>';
	RCVY_PRE_SCRIPT  = '<?php echo $RCVY_PRE_SCRIPT; ?>';
	RCVY_POST_SCRIPT = '<?php echo $RCVY_POST_SCRIPT; ?>';
	DATAMODE_INSTANCE= '<?php echo $DATAMODE_INSTANCE; ?>';
	BOOTABLE_CHECKED = '<?php echo $BOOTABLE_CHECKED; ?>';
		
	/* List Instance Flavors */
	function ListInstanceFlavors(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION':'ListInstanceFlavors'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#FLAVOR_LIST').append(new Option(key, key, true, false));
						
						if (value.Name == 't2.medium')
						{
							$("#FLAVOR_LIST").val(value.Name);
						}
					});
					
					if (FLAVOR_ID != 'xyzzy')
					{
						$("#FLAVOR_LIST").val(FLAVOR_ID);
					}
					
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
	function ListAvailableNetwork(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'      :'ListAvailableNetwork',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$('#NETWORK_LIST').attr('disabled', false);

					$.each(jso, function(key,value)
					{
						if (NETWORK_UUID != 'xyzzy')
						{
							if (NETWORK_UUID == value.SubnetId)
							{
								Network_Id = value.SubnetId;
								Default_Vpc_Id = value.VpcId;
							}
						}
						if (PRIVATE_ADDRESS != 'xyzzy')
						{
							$('#PRIVATE_ADDR').val(PRIVATE_ADDRESS);
						}
						
						select_network = NETWORK_UUID == value.SubnetId ? true : false;
						
						CIDR_INFO = NETWORK_UUID == value.SubnetId ? value.CidrBlock : false;
						
						$('#NETWORK_LIST').append(new Option(value.CidrBlock, value.SubnetId+'|'+value.VpcId+'|'+value.CidrBlock, true, select_network));											
					});
					
					if (NETWORK_UUID == 'xyzzy')
					{
						Default_Vpc_Id = jso[0].VpcId;
						Network_Id = jso[0].SubnetId;
						CIDR_INFO = jso[0].CidrBlock;
						
						$('#PRIVATE_ADDR').val('');
						$('#PRIVATE_ADDR').attr("placeholder", CIDR_INFO);
					}
					
					ListSecurityGroup();
					DescribeNetworkInterfaces();
					
					$('#NETWORK_LIST').on('change', function() {
						NETWORK_VALUE = $(this).val().split("|");
						Default_Vpc_Id = NETWORK_VALUE[1];
						Network_Id = NETWORK_VALUE[0];
						CIDR_INFO = NETWORK_VALUE[2];
						ListSecurityGroup();
						DescribeNetworkInterfaces();
						
						$('#PRIVATE_ADDR').val('');
						$('#PRIVATE_ADDR').attr("placeholder", CIDR_INFO);
					});
					$('.selectpicker').selectpicker('refresh');			
				}				
				
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	/* Describe Network Interfaces */
	function DescribeNetworkInterfaces(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'    :'DescribeNetworkInterfaces',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SERVER_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'VPC_UUID'    :Default_Vpc_Id
			},
			success:function(jso)
			{
				if (jso != false)
				{
					PrivateIpAddress = new Array();
					$.each(jso, function(key,value)
					{
						PrivateIpAddress.push(value.PrivateIpAddress);
					});
				}
			},
			error: function(xhr)
			{
	
			}
		});		
	}
	
	/* Private Address Check */
	function PrivateAddrCheck(){
		if ($("#PRIVATE_ADDR").val() == '')
		{
			PRIVATE_ADDR = 'DynamicAssign';
			SelectInstanceConfig();
		}
		else
		{
			PRIVATE_ADDR = $("#PRIVATE_ADDR").val();
			
			if (inSubNet(PRIVATE_ADDR,CIDR_INFO) == false)
			{
				BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					message: '<?php echo _("Invalid ip address."); ?>',
					draggable: true,
					closable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
					
					},
				});
			}
			else if (jQuery.inArray(PRIVATE_ADDR, PrivateIpAddress) !== -1)
			{
				BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					message: '<?php echo _("Input IP address has been used."); ?>',
					draggable: true,
					closable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
					
					},
				});
			}
			else
			{
				SelectInstanceConfig();
			}
		}			
	}	
	
	/* List Security Group */
	function ListSecurityGroup(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'    :'ListSecurityGroup',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SERVER_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'VPC_UUID'    :Default_Vpc_Id
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$('#SECURITY_GROUP').empty();
					$.each(jso, function(key,value)
					{
						$('#SECURITY_GROUP').append(new Option(value.GroupName, value.GroupId, true, false));
					});
					
					if (SGROUP_UUID != 'xyzzy')
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
	
	
	/* List Elastic Addresses */
	function ListElasticAddresses(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'    :'ListElasticAddresses',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>',
			},
			success:function(jso)
			{
				$('#ELASTIC_ADDRESS').append(new Option('<?php echo _('Dynamic Assign'); ?>', 'DynamicAssign', true, false));
				$.each(jso, function(key,value)
				{
					if(value.InstanceId == undefined)
					{
						$('#ELASTIC_ADDRESS').append(new Option(value.PublicIp, value.AllocationId, true, false));
					}
				});
					
				if (ELASTIC_ADDRESS != 'xyzzy')
				{
					$("#ELASTIC_ADDRESS").val(ELASTIC_ADDRESS);
				}
				
				$('.selectpicker').selectpicker('refresh');	
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	/* List Data Mode Instance */
	function ListDataModeInstance(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION':'ListRegionInstances',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'TRANSPORT_ID':'<?php echo $TRANSPORT_UUID; ?>'
			},
			beforeSend: function() {
				$('#DATAMODE_INSTANCE').empty();
			},
			success:function(jso)
			{
				HOST_TYPE = '<?php echo $_SESSION['HOST_TYPE']; ?>';
				
				InstanceInfo = JSON.stringify({'id':'NoAssociatedDataModeInstance', 'bootable':false, 'status':80});
				
				$('#DATAMODE_INSTANCE').append(new Option('<?php echo _('No Associated'); ?>', InstanceInfo, true, false));	
				
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						InstanceName = value.InstanceId;
						Status = value.State.Code;
						
						if (typeof value.Tags != 'undefined')
						{
							$.each(value.Tags, function(tag_key, tag_value)
							{
								if (tag_value.Key == 'Name' && tag_value.Value != '')
								{
									InstanceName = tag_value.Value;
									return false;
								}
							});
						}
						
						data_instance_select = DATAMODE_INSTANCE == value.InstanceId ? true : false;

						Bootable = value.BlockDeviceMappings.length == 0 ? true : false;

						InstanceInfo = JSON.stringify({'id':value.InstanceId, 'bootable':Bootable, 'status':Status});
						
						$('#DATAMODE_INSTANCE').append(new Option(InstanceName, InstanceInfo, true, data_instance_select));						
					});					
					SetBootCheckBox();
				}
				
				$('.selectpicker').selectpicker('refresh');
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Post Select Service UUID To Session */
	function SelectInstanceConfig(){
		/* DOM SERVUCE SETTINGS */
		var SERVICE_SETTINGS = {
								'flavor_id':$("#FLAVOR_LIST").val(),
								'network_uuid':Network_Id,
								'sgroup_uuid':$("#SECURITY_GROUP").val(),
								'elastic_address_id':$("#ELASTIC_ADDRESS").val(),
								'private_address_id':PRIVATE_ADDR,
								'rcvy_pre_script':$("#RECOVERY_PRE_SCRIPT").val(),
								'rcvy_post_script':$("#RECOVERY_POST_SCRIPT").val(),
								'hostname_tag':$("#HOSTNAME_TAG").val(),
								'datamode_instance':JSON.parse($("#DATAMODE_INSTANCE").val()).id,
								'datamode_power':$('#DATAMODE_POWER:checkbox:checked').val(),
								'is_datamode_boot':$('#DATAMODE_BOOT_FROM_INSTANCE:checkbox:checked').val()	
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
	
	/* Delete Files */
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
	
	/* Configure DataMode Agent */
	function ConfigureDataModeAgent(){
		$.ajax({
			url: '_include/_exec/mgmt_service.php',
			type:"POST",
			dataType:'TEXT',
			data:{
				 'ACTION'		:'ConfigureDataModeAgent',
				 'REPL_UUID'	:'<?php echo $REPL_UUID; ?>',
				 'DISK_FILTER'	:'<?php echo $_SESSION['VOLUME_UUID']; ?>',
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
														 is_system_disk:Boolean($("#is_system_disk"+id).text()),
														 partition_number:parseInt($("#partition_number"+id).text()),
														 partition_offset:parseInt($("#partition_offset"+id).text()),
														 partition_size:parseInt($("#partition_size"+id).text()), 
														 partition_style:parseInt($("#partition_style"+id).text()),
														 signature:parseInt($("#signature"+id).text()),
														 path:$("#set_letter"+id).val()};
										
										PartitionArray[id] = PartitionInfo;
										id++;
									});
									
									PartitionArray = {partitions:PartitionArray}
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

	/* Generate DataMode Agent Package */
	function GenerateDataModeAgentPackage(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    		:'GenerateDataModeAgentPackage',
				 'AGENT_PATH'		:'<?php echo str_replace('\\','/',str_replace('index.php','_include/_inc/_agent/',$_SERVER['SCRIPT_FILENAME'])); ?>',
				 'PARTITION_REF'	:PARTITION_REF,
				 'OS_TYPE'			:'<?php echo $_SESSION['REPL_OS_TYPE']; ?>'
			},
			success:function(agent)
			{
				window.location.href = './_include/_inc/_agent/'+agent;	
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Submit Trigger */
	$(function(){
		$("#MgmtRecoveryScript").click(function(){
			MgmtRecoveryScript();
		})
		
		$("#ConfigureDataModeAgent").click(function(){
			ConfigureDataModeAgent();
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverPlan";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = BackToLastPage;
		})
				
		$("#NextPage").click(function(){
			PrivateAddrCheck();
			//SelectInstanceConfig();
		})
	});
});

	/* Set Boot CheckBox */
	function SetBootCheckBox(){
		DATAMOE_INSTANCE_INFO = JSON.parse($("#DATAMODE_INSTANCE").val());
		
		DATAMODE_ID = DATAMOE_INSTANCE_INFO.id;
		DATAMODE_STATUS = DATAMOE_INSTANCE_INFO.status;
		DATAMODE_BOOTABLE = DATAMOE_INSTANCE_INFO.bootable;

		if (DATAMODE_ID == 'NoAssociatedDataModeInstance')
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', false);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', false);
			
			$('#MgmtRecoveryScript').prop('disabled', false);			
			
			$('#FLAVOR_LIST').prop('disabled', false);
			$('#SECURITY_GROUP').prop('disabled', false);
			$('#HOSTNAME_TAG').prop('disabled', false);

			$('#NETWORK_LIST').prop('disabled', false);
			$('#PRIVATE_ADDR').prop('disabled', false);
			$('#ELASTIC_ADDRESS').prop('disabled', false);
			
			$('#ConfigureDataModeAgent').prop('disabled', true);
			$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
			
			$('#DATAMODE_POWER').bootstrapToggle('destroy');
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('destroy');
			
			$('#DATAMODE_POWER').bootstrapToggle({
				on: '<?php echo _('N/A'); ?>',
				off: '<?php echo _('N/A'); ?>',
				width: 70
			});
			$('#DATAMODE_POWER').bootstrapToggle('enable');
			$('#DATAMODE_POWER').bootstrapToggle('off');
						
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle({
				on: '<?php echo _('N/A'); ?>',
				off: '<?php echo _('N/A'); ?>',
				width: 70
			});
			
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('enable');
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('off');
			
			$("#DATAMODE_POWER").bootstrapToggle('disable');
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('disable');
		}
		else
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', true);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', true);
			
			$('#MgmtRecoveryScript').prop('disabled', true);
			
			$('#FLAVOR_LIST').prop('disabled', true);
			$('#SECURITY_GROUP').prop('disabled', true);
			$('#HOSTNAME_TAG').prop('disabled', true);
			
			$('#NETWORK_LIST').prop('disabled', true);
			$('#PRIVATE_ADDR').prop('disabled', true);
			$('#ELASTIC_ADDRESS').prop('disabled', true);

			if (HOST_TYPE == 'Physical')
			{
				$('#ConfigureDataModeAgent').prop('disabled', false);
				$('#ConfigureDataModeAgent').removeClass('btn-default').addClass('btn-warning');
			}
			else
			{
				$('#ConfigureDataModeAgent').prop('disabled', true);
				$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
			}			
			
			$('#DATAMODE_POWER').bootstrapToggle('destroy');
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('destroy');
			
			$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle({
				on: '<?php echo _('Yes'); ?>',
				off: '<?php echo _('No'); ?>',
				offstyle: 'info',
				width: 70
			});
		
			$('#DATAMODE_POWER').bootstrapToggle({
				on: '<?php echo _('On'); ?>',
				off: '<?php echo _('Off'); ?>',
				width: 70
			});
			
			if (DATAMODE_BOOTABLE == true)
			{
				$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('enable');
			}
			else
			{
				$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('enable');
				$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('off');
				$('#DATAMODE_BOOT_FROM_INSTANCE').bootstrapToggle('disable');
			}			
			
			if (DATAMODE_STATUS == 80)
			{
				$("#DATAMODE_POWER").bootstrapToggle('enable');
				$('#DATAMODE_POWER').bootstrapToggle('off');
			}
			else
			{
				$("#DATAMODE_POWER").bootstrapToggle('enable');
				$('#DATAMODE_POWER').bootstrapToggle('on');
				$("#DATAMODE_POWER").bootstrapToggle('disable');
			}
		}
		$('.selectpicker').selectpicker('refresh');
	}
	
	/* Power Button OnChange */
	function SetPowerOnChange()
	{
		if ($('#DATAMODE_BOOT_FROM_INSTANCE:checkbox:checked').val() == "true")
		{
			$("#DATAMODE_POWER").bootstrapToggle('enable');
			$('#DATAMODE_POWER').bootstrapToggle('on');
			$("#DATAMODE_POWER").bootstrapToggle('disable');
		}
		else
		{
			if (DATAMODE_STATUS == 80)
			{
				$("#DATAMODE_POWER").bootstrapToggle('enable');
			}
			else
			{
				$("#DATAMODE_POWER").bootstrapToggle('disable');
			}
		}
	}
</script>
<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-clone fa-fw"></i>&nbsp;<?php echo _("Recovery Plan"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:25%'><a>					<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:25%'><a>					<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>				
					<li style='width:25%' class='active'><a>	<?php echo _("Step 3 - Configure Instance"); ?></a></li>
					<li style='width:25%'><a>		 			<?php echo _("Step 4 - Recovery Plan Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<label for='comment'><?php echo _("Recovery Script"); ?></label>
				<div class='form-group'>					
					<select id='RECOVERY_PRE_SCRIPT'  class='selectpicker' data-width='46%'></select>
					<select id='RECOVERY_POST_SCRIPT' class='selectpicker' data-width='46.3%'></select>
					<button id='MgmtRecoveryScript'   class='btn btn-default pull-right btn-md' disabled style='width:90px;'><?php echo _("Add Script"); ?></button>
				</div>
				
				<div class='form-group form-inline'>
					<label for='comment' style="width:33%; border-style:solid; border-width:0px;"><?php echo _("Instance Type"); ?></label>
					<label for='comment' style="width:33%; border-style:solid; border-width:0px;"><?php echo _("Instance Name"); ?></label>
					<label for='comment' style="width:22%; border-style:solid; border-width:0px;"><?php echo _("Data Mode Host"); ?></label>	
					<label for='comment' style="width:5.3%; border-style:solid; border-width:0px;"><?php echo _("Convert"); ?></label>
					<label for='comment' style="width:5.3%; border-style:solid; border-width:0px;"><?php echo _("Power"); ?></label>				
					<select id='FLAVOR_LIST' class='selectpicker' data-width='33%'></select>
					<input type='text' id='HOSTNAME_TAG' class='form-control' value='<?php echo $_SESSION['HOST_NAME']; ?>' placeholder='' style="width:33%">
					<select id='DATAMODE_INSTANCE' class='selectpicker' data-width='16.5%' onchange="SetBootCheckBox();"></select>
					<button id='ConfigureDataModeAgent' class='btn btn-default btn-md' disabled style='width:65px;'><?php echo _("Agent"); ?></button>
					<input id="DATAMODE_BOOT_FROM_INSTANCE" data-toggle="toggle" data-width="70" data-style="slow" value="true" type="checkbox" disabled data-on="<?php echo _('N/A'); ?>" data-off="<?php echo _('N/A'); ?>" onchange="SetPowerOnChange();">
					<input id="DATAMODE_POWER" data-toggle="toggle" data-width="70" data-style="slow" type="checkbox" disabled data-on="<?php echo _('N/A'); ?>" data-off="<?php echo _('N/A'); ?>">
				</div>
				
				<div class='form-group form-inline'>
					<label for='comment' style="width:16.5%"><?php echo _("Internal Network"); ?></label>
					<label for='comment' style="width:16%"><?php echo _("IP Address"); ?></label>
					<label for='comment' style="width:16.5%"><?php echo _("Security Group"); ?></label>
					<label for='comment' style="width:50%"><?php echo _("Elastic Address"); ?></label>
					<select id='NETWORK_LIST' class='selectpicker' data-width='16.3%' disabled></select>
					<input type='text' id='PRIVATE_ADDR' class='form-control' value='' placeholder='0.0.0.0/0' style="width:16.2%" disabled>
					<select id='SECURITY_GROUP' class='selectpicker' data-width='16.3%' data-dropup-auto="false"></select>
					<select id='ELASTIC_ADDRESS' class='selectpicker' data-width='16.2%' data-dropup-auto="false"></select>
				</div>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToLastPage' class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='CancelToMgmt' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='NextPage' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>						
				</div>
			</div>
		</div>			
	</div> <!-- id: wrapper_block-->
</div>