<script>
	/* Define Portal Max Upload Size */
	MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;
	
	/* Begin to Automatic Exec */
	ListRecoveryScript();	
	SummaryPageRoute();
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#SelectInstanceConfig').prop('disabled', false);
		$('#SelectInstanceConfig').removeClass('btn-default').addClass('btn-primary');

		$('#BackToServiceList').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
	});	
	
	/* Page Route Control */
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
				
				/* OpenStack Query */
				ListInstanceFlavors();
				ListAvailableNetwork();				
				ListSecurityGroup();
				ListAvailablePublicIP();				
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
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'      :'ListInstanceFlavors',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#FLAVOR_LIST').append(new Option(value.flavorName+'  (Name:'+value.name+', CPU:'+value.cpuNum+'Core, Memory:'+value.memSize+'GB, Type:'+value.flavorType+')', value.id, true, false));
						$("#FLAVOR_LIST").val('s3.medium.2');
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
	
		
	/* List Subnet Networks */
	function ListAvailableNetwork(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    :'ListAvailableNetwork',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{					
					$.each(jso, function(key,value)
					{
						$('#SUBNET_LIST').append(new Option(value.name+' ('+value.cidr+')', value.resVlanId, true, false));						
					});
					$('.selectpicker').selectpicker('refresh');	

					/* Query Default Address */
					QuerySubnetAddress();
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
	
	
	/* Query Subnet Network Address */
	function QuerySubnetAddress(){
		
		$('#PRIVATE_ADDR').val('');
		$('#PRIVATE_ADDR').prop('disabled', true);
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'      :'QuerySubnetInformation',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'NETWORK_UUID':null
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					if (value.resVlanId == $("#SUBNET_LIST").val())
					{
						NETWORK_ID = value.vpcId;
						SUBNET_ID = value.resVlanId;
						CIDR_INFO = value.cidr;
					}
				});
				
				$('#PRIVATE_ADDR').prop('disabled', false);
				$('#PRIVATE_ADDR').attr("placeholder", CIDR_INFO);

				$('#SelectInstanceConfig').prop('disabled', false);
				$('#SelectInstanceConfig').removeClass('btn-default').addClass('btn-primary');

				$('#BackToServiceList').prop('disabled', false);
				$('#BackToMgmtRecoverWorkload').prop('disabled', false);
				
				ListDataModeInstance();
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
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'    :'ListSecurityGroup',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{					
					$.each(jso, function(key,value)
					{
						$('#SECURITY_GROUP').append(new Option(value.name, value.resSecurityGroupId, false, false));
						
						if (value.name == 'default')
						{
							$("#SECURITY_GROUP").val(value.resSecurityGroupId);
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
				
				$('.selectpicker').selectpicker('refresh');

				$('#MgmtRecoveryScript').removeClass('btn-default').addClass('btn-success');
				$('#MgmtRecoveryScript').prop('disabled', false);				
			},
			error: function(xhr)
			{
	
			}
		});
	}
	
	
	/* List Public IP Address */
	function ListAvailablePublicIP(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'      :'ListAvailablePublicIP',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				PUBLIC_ADDR_COUNT = false;
				if (jso != false)
				{
					$('#FLOATING_ADDR').append(new Option('<?php echo _('No Associated'); ?>', 'DynamicAssign', true, false));
					$.each(jso.publicips, function(key,value)
					{
						if (value.status == 'DOWN')
						{
							PUBLIC_ADDR_COUNT = true;
							$('#FLOATING_ADDR').append(new Option(value.public_ip_address, value.id, true, false));
						}
					});
				}
				
				if (PUBLIC_ADDR_COUNT == false)
				{
					$('#FLOATING_ADDR').append(new Option('<?php echo _('No available public IP'); ?>', 'DynamicAssign', true, false));
					$("#FLOATING_ADDR").prop("disabled", true);
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
		$('#SelectInstanceConfig').prop('disabled', true);
		$('#SelectInstanceConfig').removeClass('btn-primary').addClass('btn-default');

		$('#BackToServiceList').prop('disabled', true);
		$('#BackToMgmtRecoverWorkload').prop('disabled', true);	
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION':'ListDataModeInstances',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'FILTER_UUID':'<?php echo $_SESSION['SERV_UUID']; ?>',
				
			},
			beforeSend: function() {
				$('#DATAMODE_INSTANCE').empty();
			},
			success:function(jso)
			{
				HOST_TYPE = '<?php echo $_SESSION['HOST_TYPE']; ?>';
				
				InstanceInfo = JSON.stringify({'id':'NoAssociatedDataModeInstance', 'bootable':true});
				
				$('#DATAMODE_INSTANCE').append(new Option('<?php echo ('No Associated'); ?>', InstanceInfo, true, false));	
				if (jso != false)
				{
					$.each(jso.servers, function(key,value)
					{
						Bootable = value["os-extended-volumes:volumes_attached"].length == 0 ? false : true;
						
						InstanceInfo = JSON.stringify({'id':value.id, 'bootable':Bootable});
						
						$('#DATAMODE_INSTANCE').append(new Option(value.name, InstanceInfo, true, false));						
					});					
				}
				
				$('.selectpicker').selectpicker('refresh');
										
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
						title: 'Recovery Script',
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
	
	/* Set Boot CheckBox */
	function SetBootCheckBox(){
		
		DATAMOE_INSTANCE_INFO = JSON.parse($("#DATAMODE_INSTANCE").val());
		
		DATAMODE_ID = DATAMOE_INSTANCE_INFO.id;
		DATAMODE_BOOTABLE = DATAMOE_INSTANCE_INFO.bootable;
		
		if (DATAMODE_ID == 'NoAssociatedDataModeInstance')
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', false);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', false);
			
			$('#MgmtRecoveryScript').prop('disabled', false);			
			
			$('#FLAVOR_LIST').prop('disabled', false);
			$('#SECURITY_GROUP').prop('disabled', false);
			$('#HOSTNAME_TAG').prop('disabled', false);

			$('#DATAMODE_BOOT_FROM_INSTANCE').prop('checked', false);
			$('#DATAMODE_BOOT_FROM_INSTANCE').prop('disabled', true);

			$('#SUBNET_LIST').prop('disabled', false);
			$('#PRIVATE_ADDR').prop('disabled', false);
			$('#FLOATING_ADDR').prop('disabled', false);

			$('#ConfigureDataModeAgent').prop('disabled', true);
			$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
		}
		else
		{
			$('#RECOVERY_PRE_SCRIPT').prop('disabled', true);
			$('#RECOVERY_POST_SCRIPT').prop('disabled', true);
			
			$('#MgmtRecoveryScript').prop('disabled', true);
			
			$('#FLAVOR_LIST').prop('disabled', true);
			$('#SECURITY_GROUP').prop('disabled', true);
			$('#HOSTNAME_TAG').prop('disabled', true);
			
			$('#DATAMODE_BOOT_FROM_INSTANCE').prop('checked', false);
			$('#DATAMODE_BOOT_FROM_INSTANCE').prop('disabled', DATAMODE_BOOTABLE);
			
			if (DATAMODE_BOOTABLE == true)
			{
				$('#ConfigureDataModeAgent').prop('disabled', false);
				$('#ConfigureDataModeAgent').removeClass('btn-default').addClass('btn-warning');
			}
			else
			{
				$('#ConfigureDataModeAgent').prop('disabled', true);
				$('#ConfigureDataModeAgent').removeClass('btn-warning').addClass('btn-default');
			}
						
			$('#SUBNET_LIST').prop('disabled', true);
			$('#PRIVATE_ADDR').prop('disabled', true);
			$('#FLOATING_ADDR').prop('disabled', true);			
		}
		$('.selectpicker').selectpicker('refresh');
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
	
	
	/* Select Instance Config */
	function SelectInstanceConfig(){
		
		/* DOM SERVUCE SETTINGS */
		var SERVICE_SETTINGS = {
								'flavor_id':$("#FLAVOR_LIST").val(),
								'network_uuid':NETWORK_ID,
								'subnet_uuid':SUBNET_ID,
								'sgroup_uuid':$("#SECURITY_GROUP").val(),
								'elastic_address_id':$("#FLOATING_ADDR").val(),
								'private_address_id':PRIVATE_ADDR,
								'rcvy_pre_script':$("#RECOVERY_PRE_SCRIPT").val(),
								'rcvy_post_script':$("#RECOVERY_POST_SCRIPT").val(),
								'hostname_tag':$("#HOSTNAME_TAG").val(),
								'datamode_instance':JSON.parse($("#DATAMODE_INSTANCE").val()).id,
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
					window.location.href = 'RecoverCtyunSummary';
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
	
	/* Configure DataMode Agent */
	function ConfigureDataModeAgent(){
		$.ajax({
			url: '_include/_exec/mgmt_service.php',
			type:"POST",
			dataType:'TEXT',
			data:{
				 'ACTION'		:'ConfigureDataModeAgent',
				 'REPL_UUID'	:'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'DISK_FILTER'	:'<?php echo $_SESSION['SNAP_UUID']; ?>',
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
		
		$("#BackToMgmtRecoverWorkload").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToServiceList").click(function(){
			window.location.href = RouteBack;
		})
		
		$("#SelectInstanceConfig").click(function(){
			PrivateAddrCheck();
			//SelectInstanceConfig();
		})
	});
</script>
<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:18%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:22%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 3 - Select Snapshot"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 4 - Configure Instance"); ?></a></li>
					<li style='width:20%'><a>		 		<?php echo _("Step 5 - Recovery Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<label for='comment'><?php echo _("Recovery Script"); ?></label>
				<div class='form-group'>					
					<select id='RECOVERY_PRE_SCRIPT'  class='selectpicker' data-width='46%'></select>
					<select id='RECOVERY_POST_SCRIPT' class='selectpicker' data-width='46%'></select>
					<button id='MgmtRecoveryScript'   class='btn btn-default pull-right btn-md' disabled><?php echo _("Add Script"); ?></button>
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:46%"><?php echo _("Instance Flavors"); ?></label>
					<label for='comment' style="width:25%"><?php echo _("Instance Name"); ?></label>
					<label for='comment' style="width:21%"><?php echo _("Data Mode Host"); ?></label>
					
					<select id='FLAVOR_LIST' class='selectpicker' data-width='46%'></select>					
					<input type='text' id='HOSTNAME_TAG' class='form-control' value='<?php echo $_SESSION['HOST_NAME']; ?>' placeholder='' style="width:25%">
					<select id='DATAMODE_INSTANCE' class='selectpicker' data-width='19%' onchange="SetBootCheckBox();"></select>
					<button id='ConfigureDataModeAgent' class='btn btn-default btn-md' disabled><?php echo _("Agent"); ?></button>
					<label class="checkbox-inline"><input type="checkbox" id="DATAMODE_BOOT_FROM_INSTANCE" value="true" disabled><?php echo _("Boot"); ?></label>	
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:23%"><?php echo _("Subnet Network"); ?></label>
					<label for='comment' style="width:23%"><?php echo _("IP Address"); ?></label>
					<label for='comment' style="width:25%"><?php echo _("Floating IP"); ?></label>
					<label for='comment' style="width:21%"><?php echo _("Security Group"); ?></label>
					
					<select id='SUBNET_LIST' class='selectpicker' data-width='23%' onchange="QuerySubnetAddress();"></select>
					<input type='text' id='PRIVATE_ADDR' class='form-control' value='' placeholder='0.0.0.0/0' style="width:23%" disabled>
					<select id='FLOATING_ADDR' class='selectpicker' data-width='25%'></select>
					<select id='SECURITY_GROUP' class='selectpicker' data-width='23%' data-dropup-auto="false"></select>
				</div>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToServiceList' 	  		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectInstanceConfig' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>					
				</div>
			</div>
		</div>			
	</div> <!-- id: wrapper_block-->
</div>