<?php
if (isset($_SESSION['EDIT_PLAN']))
{
	$_SESSION['CLOUD_TYPE']   = $_SESSION['EDIT_PLAN'] -> CloudType;
	$_SESSION['CLUSTER_UUID'] = $_SESSION['EDIT_PLAN'] -> ClusterUUID;
	$_SESSION['SERV_REGN']	  = $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$_SESSION['REPL_OS_TYPE'] = $_SESSION['EDIT_PLAN'] -> HostType;
	
	$FLAVOR_ID    		= $_SESSION['EDIT_PLAN'] -> flavor_id;
	$NETWORK_UUID 		= $_SESSION['EDIT_PLAN'] -> network_uuid;
	$SGROUP_UUID  		= $_SESSION['EDIT_PLAN'] -> sgroup_uuid;
	$RCVY_PRE_SCRIPT 	= $_SESSION['EDIT_PLAN'] -> rcvy_pre_script;
	$RCVY_POST_SCRIPT	= $_SESSION['EDIT_PLAN'] -> rcvy_post_script;
	$publicIp			= $_SESSION['EDIT_PLAN'] -> elastic_address_id;
	$privateIp			= $_SESSION['EDIT_PLAN'] -> private_address_id;
	$diskType			= isset( $_SESSION['EDIT_PLAN'] -> disk_type )?$_SESSION['EDIT_PLAN'] -> disk_type:null;
	$HOSTNAME_TAG 		= $_SESSION['EDIT_PLAN'] -> hostname_tag;
}
else
{
	$FLAVOR_ID 			= '';
	$NETWORK_UUID 		= '';
	$SGROUP_UUID 		= '';
	$RCVY_PRE_SCRIPT 	= '';
	$RCVY_POST_SCRIPT 	= '';
	$publicIp 			= '';
	$privateIp 			= '';
	$diskType			= '';
	$HOSTNAME_TAG 		= $_SESSION['HOST_NAME'];
}
?>

<script>
	/* Define Portal Max Upload Size */
	MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;

	var FLAVOR_ID 		= '<?php echo $FLAVOR_ID; ?>';
	var NETWORK_UUID 	= '<?php echo $NETWORK_UUID; ?>';
	var SGROUP_UUID 	= '<?php echo $SGROUP_UUID; ?>';
	var RCVY_PRE_SCRIPT  = '<?php echo $RCVY_PRE_SCRIPT; ?>';
	var RCVY_POST_SCRIPT = '<?php echo $RCVY_POST_SCRIPT; ?>';
	
	var privateIp 		= '<?php echo $privateIp; ?>';
	var publicIp 		= '<?php echo $publicIp; ?>';
	
	var diskType 		= '<?php echo $diskType; ?>';

	$(document).ready( function() {
		
		$("#PUBLIC_IP").val( publicIp );
		
		if( privateIp != "DynamicAssign" )
			$("#SPECIFIED_PRIVATE_IP").val( privateIp );
		
		$("#Disk_Type").val( diskType );
	});
	
	/* Begin to Automatic Exec */
	ListRecoveryScript();
	ListInstanceFlavors();
	ListAvailableNetwork('<?php echo $_SESSION['SERV_REGN']; ?>');
	//ListSecurityGroup();
	
	var vpc = null;
	var sg = null;
	var subnet = null;
	var zone = '<?php echo $_SESSION['SERV_REGN']; ?>';
	/* Enable Buttons When All Ajax Request Stop*/
	
	$(document).one("ajaxStop", function() {
		$('#SelectTencentInstanceConfig').prop('disabled', false);
		$('#SelectTencentInstanceConfig').removeClass('btn-default').addClass('btn-primary');

		$('#BackToServiceList').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
	});
	
	DetermineSegment('EditPlanInstanceTencentConfigurations');
	/* Determine URL Segment */
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			RouteBack  = 'EditPlanSelectRecoverType';
			NextPage		= 'EditPlanRecoverTencentSummary';
		}
		else
		{
			RouteBack 	= 'PlanSelectRecoverType';
			NextPage		= 'PlanRecoverTencentSummary';
		}
	}
	
	/* List Instance Flavors */
	function ListInstanceFlavors(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Tencent.php',
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
						$('#FLAVOR_LIST').append(new Option(value.InstanceType, value.InstanceType, true, false));
						
						if (value.InstanceType == 'S2.SMALL2')
						{
							$("#FLAVOR_LIST").val(value.InstanceType);
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
			url: '_include/_exec/mgmt_Tencent.php',
			data:{
				 'ACTION'      :'ListAvailableNetwork',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SELECT_ZONE' :SELECT_ZONE
			},
			success:function(jso)
			{
				if (jso != false)
				{
					var vpc_map = [];
					vpc = jso;
					var default_vpc = null;
					$('#NETWORK_LIST').attr('disabled', false);
					$('#NETWORK_LIST')[0].options.length = 0;
					$('#SUBNET').attr('disabled', false);
					$('#SUBNET')[0].options.length = 0;
					
					subnet = jso["subnets"];
					
					$.each(jso, function(key,value)
					{
						
						var display = value.unVpcId+" "+value.CidrBlock+" "+"( "+value.VpcName+" )";
						var data = value.unVpcId;
						
						if( key != "subnets" )
						{
							for( var i = 0; i < subnet.length; i++ ) {
								if( value.unVpcId == subnet[i].unVpcId && zone == subnet[i].zone ) {
									
									$('#NETWORK_LIST').append(new Option(display, JSON.stringify( value ), true, false));		
									
									if( default_vpc == null )
										default_vpc = value.unVpcId;
									
									break;
								}
							}
						}
											
					});
					
					$.each(jso.subnets, function(key,value) {
						
						
						if( value.unVpcId == default_vpc && zone == value.zone) {
							
							$('#SUBNET').append(new Option(value.unSubnetId+"( "+value.subnetName+" )", value.unSubnetId, true, false));

						}
					})


					ListSecurityGroup();	
					
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
	
	
	/* List Security Group */
	function ListSecurityGroup(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Tencent.php',
			data:{
				 'ACTION'    :'ListSecurityGroup',
				 'CLUSTER_UUID':'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SERVER_ZONE' :'<?php echo $_SESSION['SERV_REGN']; ?>',
			},
			success:function(jso)
			{
				if (jso != false)
				{		
					sg = jso;
					
					var select_vpc = JSON.parse( $("#NETWORK_LIST").val() ).unVpcId;
					
					$.each(jso, function(key,value)
					{
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
	function SelectTencentInstanceConfig(){
		var info = JSON.parse( $("#NETWORK_LIST").val() );
		
		if( !checkIpInSubnet( info.CidrBlock ) )
			return false;
		
		/* DOM SERVUCE SETTINGS */
		var SERVICE_SETTINGS = {
								'flavor_id'			:$("#FLAVOR_LIST").val(),
								'network_uuid'		:info.unVpcId,
								'sgroup_uuid'		:$("#SECURITY_GROUP").val(),
								'subnet_uuid'		:$("#SUBNET").val(),
								'elastic_address_id':$("#PUBLIC_IP").val(),
								'private_address_id':$("#SPECIFIED_PRIVATE_IP").val(),
								'rcvy_pre_script'	:$("#RECOVERY_PRE_SCRIPT").val(),
								'rcvy_post_script'	:$("#RECOVERY_POST_SCRIPT").val(),
								'disk_type'			:$("#Disk_Type").val(),
								'hostname_tag'		:$("#HOSTNAME_TAG").val()
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
	
	function changeVPC( ){
		
		var select_vpc = JSON.parse( $("#NETWORK_LIST").val() ).unVpcId;

		$('#SUBNET').empty();
		
		$.each(subnet, function(key,value)
		{
			if( select_vpc == value.unVpcId && zone == value.zone )
			{
				$('#SUBNET').append(new Option(value.unSubnetId+"( "+value.subnetName+" )", value.unSubnetId, true, false));	
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
		
		$("#SelectTencentInstanceConfig").click(function(){
			SelectTencentInstanceConfig();
		})
	});
</script>
<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recover Workload"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:25%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:25%' class='active'><a><?php echo _("Step 3 - Configure Instance"); ?></a></li>
					<li style='width:25%'><a>		 		<?php echo _("Step 4 - Recover Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<div class='form-group'>					
					<select id='RECOVERY_PRE_SCRIPT'  class='selectpicker' data-width='46%'></select>
					<select id='RECOVERY_POST_SCRIPT' class='selectpicker' data-width='46%'></select>
					<button id='MgmtRecoveryScript'   class='btn btn-default pull-right btn-md' disabled><?php echo _("Add Script"); ?></button>
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("Instance Type"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Disk Type"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Instance Name"); ?></label>
					<select id='FLAVOR_LIST' class='selectpicker' data-width='33.3%'></select>
					<select id='Disk_Type' class='selectpicker' data-width='33%' data-dropup-auto="false" >
					<option selected="selected">HDD</option>
					<option>SSD</option>
					</select>
					<input type='text' id='HOSTNAME_TAG' class='form-control' value='<?php echo $HOSTNAME_TAG; ?>' placeholder='' style="width:33%">
				</div>

				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("Security Group"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("VPC"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Subnet"); ?></label>
					<select id='SECURITY_GROUP' class='selectpicker' data-width='33.3%' data-dropup-auto="false"></select>
					<select onchange="changeVPC()" id='NETWORK_LIST' class='selectpicker' data-width='33%' disabled></select>
					<select id='SUBNET' class='selectpicker' data-width='33%' disabled></select>
				</div>
								
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("Public IP"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Specified Private IP"); ?>:</label>
					<select id='PUBLIC_IP' class='selectpicker' data-width='33.3%' data-dropup-auto="false" >
					<option>Yes</option>
					<option selected="selected">No</option>
					</select>
					<input type='text' id='SPECIFIED_PRIVATE_IP' class='form-control' value='' style="width:33%" placeholder='Private IP'>	
				</div>				
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToServiceList' 	  		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectTencentInstanceConfig' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>					
				</div>
			</div>
		</div>			
	</div> <!-- id: wrapper_block-->
</div>