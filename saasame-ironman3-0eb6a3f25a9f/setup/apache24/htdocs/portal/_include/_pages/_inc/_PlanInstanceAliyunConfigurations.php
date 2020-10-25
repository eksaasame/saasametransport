<?php

if (isset($_SESSION['EDIT_PLAN']))
{
	$_SESSION['CLUSTER_UUID'] = $_SESSION['EDIT_PLAN'] -> ClusterUUID;
	$_SESSION['REPL_OS_TYPE'] = $_SESSION['EDIT_PLAN'] -> HostType;
	$_SESSION['REPL_UUID'] = $_SESSION['EDIT_PLAN'] -> ReplUUID;
	$_SESSION['SERVER_UUID'] = $_SESSION['EDIT_PLAN'] -> SERVER_UUID;
	
	$SERVICE_REGION			  = $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$FLAVOR_ID    			  = $_SESSION['EDIT_PLAN'] -> flavor_id;
	$NETWORK_UUID 			  = $_SESSION['EDIT_PLAN'] -> network_uuid;
	$SGROUP_UUID  			  = $_SESSION['EDIT_PLAN'] -> sgroup_uuid;
	$SwitchId     			  = $_SESSION['EDIT_PLAN'] -> switch_uuid;
	$PublicIP     			  = $_SESSION['EDIT_PLAN'] -> elastic_address_id;
	$RCVY_PRE_SCRIPT		  = $_SESSION['EDIT_PLAN'] -> rcvy_pre_script;
	$RCVY_POST_SCRIPT		  = $_SESSION['EDIT_PLAN'] -> rcvy_post_script;
	$HOSTNAME_TAG 	 		  = $_SESSION['EDIT_PLAN'] -> hostname_tag;
}
else
{
	$SERVICE_REGION = $_SESSION['SERV_REGN'];
	$FLAVOR_ID = '';
	$NETWORK_UUID = '';
	$SGROUP_UUID = '';
	$SwitchId = '';
	$PublicIP = '';
	$RCVY_PRE_SCRIPT = 'xyzzy';
	$RCVY_POST_SCRIPT = 'xyzzy';
	$HOSTNAME_TAG = $_SESSION['HOST_NAME'];
}
?>

<?php include "_include/_pages/_inc/dataMode_com.php"?>
<script>

	$(document).ready(function(){

		if( "<?php echo isset( $_SESSION["RECY_TYPE"] )?$_SESSION["RECY_TYPE"]:"false" ?>" == "RECOVERY_PM" ){
			$('#diskType').selectpicker('hide');
			$('#l_diskType').hide();
		}
	});
	
	obj = new dataMode(); 
	obj.ListDataModeInstances( "mgmt_aliyun.php" );
	/* Define Portal Max Upload Size */
	MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;
	
	var FLAVOR_ID 		 = '<?php echo $FLAVOR_ID; ?>';
	var NETWORK_UUID 	 = '<?php echo $NETWORK_UUID; ?>';
	var SGROUP_UUID 	 = '<?php echo $SGROUP_UUID; ?>';
	var SwitchId 		 = '<?php echo $SwitchId; ?>';
	var PublicIP 		 = '<?php echo $PublicIP; ?>';
	var RCVY_PRE_SCRIPT  = '<?php echo $RCVY_PRE_SCRIPT; ?>';
	var RCVY_POST_SCRIPT = '<?php echo $RCVY_POST_SCRIPT; ?>';
	
	DetermineSegment('EditPlanInstanceAliyunConfigurations');
	
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			BackToLastPage  = 'EditPlanSelectRecoverType';
			NextPage		= 'EditPlanRecoverAliyunSummary';
		}
		else
		{
			BackToLastPage 	= 'PlanSelectRecoverType';
			NextPage		= 'PlanRecoverAliyunSummary';
		}
	}
	
	/* Begin to Automatic Exec */
	ListRecoveryScript();
	ListInstanceFlavors();
	ListAvailableNetwork('<?php echo $SERVICE_REGION; ?>');
	//ListSecurityGroup();
	
	var vpc = null;
	var sg = null;
	var switchs = null;
	var zone = '<?php echo $SERVICE_REGION; ?>';
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#NextPage').prop('disabled', false);
		$('#NextPage').removeClass('btn-default').addClass('btn-primary');

		$('#BackToLastPage').prop('disabled', false);
		$('#CancelToMgmt').prop('disabled', false);
		obj.SetBootCheckBox();
	});
	
	/* List Instance Flavors */
	function ListInstanceFlavors(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Aliyun.php',
			data:{
				 'ACTION':'ListInstanceFlavors',
				 'SERV_REGN' :'<?php echo $SERVICE_REGION; ?>',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#FLAVOR_LIST').append(new Option(value.InstanceTypeId+"( CPU: "+value.CpuCoreCount+"Core ,Memory: "+value.MemorySize+"G )", value.InstanceTypeId, true, false));
						
						if (value == 'ecs.n1.small')
						{
							$("#FLAVOR_LIST").val(value);
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
		
		zone = SELECT_ZONE;
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Aliyun.php',
			data:{
				 'ACTION'      :'ListAvailableNetwork',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE' :SELECT_ZONE
			},
			success:function(jso)
			{
				if (jso != false)
				{
					vpc = jso;
					$('#NETWORK_LIST').attr('disabled', false);
					$('#NETWORK_LIST')[0].options.length = 0;
					$('#SWITCH').attr('disabled', false);
					$('#SWITCH')[0].options.length = 0;
					$.each(jso, function(key,value)
					{
						var display = value.VpcId+"( "+value.VpcName+" )";
						var data = value.VpcId;
						
						if( key == "switch_detail" )
							switchs = value;
						else
							$('#NETWORK_LIST').append(new Option(display, data, true, false));	
											
					});
					$.each(jso[0].VSwitchIds.VSwitchId, function(key,value) {
						
						$.each(switchs, function(skey,svalue) {
						if( svalue.VSwitchId == value && svalue.ZoneId == zone)
							$('#SWITCH').append(new Option(value, value, true, false));
						})
					})

					ListSecurityGroup();	
					
					if (NETWORK_UUID != '')
					{
						$("#NETWORK_LIST").val( NETWORK_UUID );
						$("#SWITCH").val( SwitchId );
						$("#PUBLIC_IP").val( PublicIP );
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
			url: '_include/_exec/mgmt_Aliyun.php',
			data:{
				 'ACTION'    :'ListSecurityGroup',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SERVER_ZONE' :'<?php echo $SERVICE_REGION; ?>',
			},
			success:function(jso)
			{
				if (jso != false)
				{		
					sg = jso;
					
					var select_vpc = $("#NETWORK_LIST").val();
					
					$.each(jso, function(key,value)
					{
						if( select_vpc == value.VpcId)
							$('#SECURITY_GROUP').append(new Option(value.SecurityGroupName, value.SecurityGroupId, true, false));
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
	
	
	/* Post Select Service UUID To Session */
	function SelectAliyunInstanceConfig(){
		if( !checkSwitchAndSGEmpty() )
			return false;
		
		$.each(switchs, function(key,value)
		{
			if( $("#SWITCH").val() == value.VSwitchId && zone == value.ZoneId )
			{
				cidr = value.CidrBlock;
			}
								
		});
		
		if( !checkIpInSubnet( cidr ) )
			return false;
		
		/* DOM SERVUCE SETTINGS */
		var SERVICE_SETTINGS = {
								'flavor_id':$("#FLAVOR_LIST").val(),
								'network_uuid':$("#NETWORK_LIST").val(),
								'sgroup_uuid':$("#SECURITY_GROUP").val(),
								'switch_uuid':$("#SWITCH").val(),
								'elastic_address_id':$("#PUBLIC_IP").val(),
								'private_address_id':$("#SPECIFIED_PRIVATE_IP").val(),
								'rcvy_pre_script':$("#RECOVERY_PRE_SCRIPT").val(),
								'rcvy_post_script':$("#RECOVERY_POST_SCRIPT").val(),
								'hostname_tag':$("#HOSTNAME_TAG").val(),
								'datamode_instance': $("#DATAMODE_INSTANCE").val(),
								'disk_type' :$("#diskType").val()
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
	
	function checkIpInSubnet( cidr ){
		
		if( $("#SPECIFIED_PRIVATE_IP").val() === "" ){
			return true;
		}
		
		ret = inSubNet( $("#SPECIFIED_PRIVATE_IP").val(), cidr );
		
		if( ret === false ){
			
			BootstrapDialog.show({
				title: '<?php echo _("Service Message"); ?>',
				message: $("#SPECIFIED_PRIVATE_IP").val()+" is not in range of "+cidr,
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
	
	function checkSwitchAndSGEmpty( ){
		
		if( $("#SECURITY_GROUP").val() !== null || $("#SWITCH").val() !== null){
			return true;
		}
			
		BootstrapDialog.show({
			title: '<?php echo _("Service Message"); ?>',
			message: "<?php echo _("Security Group or Switch cannot be empty."); ?>",
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
	
	function changeVPC( ){
		
		var select_vpc = $("#NETWORK_LIST").val();

		$('#SWITCH').empty();
		$('#SECURITY_GROUP').empty();
		
		$.each(switchs, function(key,value)
		{
			if( select_vpc == value.VpcId && zone == value.ZoneId )
			{
				$('#SWITCH').append(new Option(value.VSwitchId, value.VSwitchId, true, false));	
			}
								
		});
		
		$.each(sg, function(key,value)
		{
			if( select_vpc == value.VpcId )
			{
				$('#SECURITY_GROUP').append(new Option(value.SecurityGroupName, value.SecurityGroupId, true, false));	
			}
								
		});
		
		$('.selectpicker').selectpicker('refresh');
								
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
	
	/* Submit Trigger */
	$(function(){
		$("#MgmtRecoveryScript").click(function(){
			MgmtRecoveryScript();
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = 'PlanSelectRecoverType';
		})
		
		$("#NextPage").click(function(){
			SelectAliyunInstanceConfig();
		})
		
		$("#ConfigureDataModeAgent").click(function(){
			obj.ConfigureDataModeAgent();
		})

		$('#DATAMODE_INSTANCE').on('change',function(){
			obj.SetBootCheckBox();
		})
		
	});
</script>
<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-clone fa-fw"></i>&nbsp;Recovery Plan
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
					<select id='RECOVERY_POST_SCRIPT' class='selectpicker' data-width='46%'></select>
					<button id='MgmtRecoveryScript'   class='btn btn-default pull-right btn-md' disabled><?php echo _("Add Script"); ?></button>
				</div>
				
				<div class='form-group'>
					<label for='comment' style="width:46%"><?php echo _("Instance Type"); ?></label>
					<label for='comment' style="width:46%"><?php echo _("Data Mode Host"); ?></label>
					<select id='FLAVOR_LIST' class='selectpicker' data-width='46%'></select>
					<select id='DATAMODE_INSTANCE' class='selectpicker' data-width='46%' ></select>
					<button id='ConfigureDataModeAgent' class='btn btn-default pull-right btn-md' style="width:7%" disabled><?php echo _("Agent"); ?></button>
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("VPC"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Security Group"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Instance Name"); ?></label>
					<select onchange="changeVPC()" id='NETWORK_LIST' class='selectpicker' data-width='33.3%' disabled></select>
					<select id='SECURITY_GROUP' class='selectpicker' data-width='33%' data-dropup-auto="false"></select>
					<input type='text' id='HOSTNAME_TAG' class='form-control' value='<?php echo $HOSTNAME_TAG; ?>' placeholder='' style="width:33%">
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("Switch"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Specified Private IP"); ?>:</label>					
					<label for='comment' style="width:33%"><?php echo _("Public IP"); ?></label>
					
					<select id='SWITCH' class='selectpicker' data-width='33.3%' disabled></select>
					<input type='text' id='SPECIFIED_PRIVATE_IP' style="width:33%" class='form-control' value='' placeholder='Private IP'>
					<select id='PUBLIC_IP' class='selectpicker' data-width='33%' data-dropup-auto="false" >
					<option selected="selected">Yes</option>
					<option>No</option>
					</select>
				</div>	

				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%" id='l_diskType' ><?php echo _("Disk Type"); ?></label>
					<label for='comment' style="width:33%"></label>					
					<label for='comment' style="width:33%"></label>
					
					<select id='diskType' class='selectpicker' data-width='33%' data-dropup-auto="false" >
						<option value="cloud_efficiency" selected="selected">HDD</option>
						<option value="cloud_ssd">SSD</option>
					</select>
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