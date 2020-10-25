<?php
if (isset($_SESSION['EDIT_PLAN']))
{
	$_SESSION['CLOUD_TYPE'] = $_SESSION['EDIT_PLAN'] -> CloudType;
	$_SESSION['CLUSTER_UUID'] = $_SESSION['EDIT_PLAN'] -> ClusterUUID;	
	
	$FLAVOR_ID    = $_SESSION['EDIT_PLAN'] -> flavor_id;
	$NETWORK_UUID = $_SESSION['EDIT_PLAN'] -> network_uuid;
	$SGROUP_UUID  = $_SESSION['EDIT_PLAN'] -> sgroup_uuid;
	
	$RCVY_PRE_SCRIPT = $_SESSION['EDIT_PLAN'] -> rcvy_pre_script;
	$RCVY_POST_SCRIPT=$_SESSION['EDIT_PLAN'] -> rcvy_post_script;
	$_SESSION['SERV_REGN']	  	= $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$_SESSION['SERV_UUID']	  	= $_SESSION['EDIT_PLAN'] -> VM_NAME;
	$_SESSION['SERVER_UUID']	= $_SESSION['EDIT_PLAN'] -> ServerUUID;
	$publicIp					= $_SESSION['EDIT_PLAN'] -> elastic_address_id;
	$privateIp					= $_SESSION['EDIT_PLAN'] -> private_address_id;
	$diskType					= $_SESSION['EDIT_PLAN'] -> disk_type;
	$availabilitySet			= $_SESSION['EDIT_PLAN'] -> availability_set;
	$_SESSION['REPL_OS_TYPE'] 	= $_SESSION['EDIT_PLAN'] -> HostType;
	$HOSTNAME_TAG 	 			= $_SESSION['EDIT_PLAN'] -> hostname_tag;
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
	$availabilitySet	= '';
	$HOSTNAME_TAG		= $_SESSION['HOST_NAME'];
}

?>
<?php include "_include/_pages/_inc/AzureRequest.php"?>
<script>
	var FLAVOR_ID 		= '<?php echo $FLAVOR_ID; ?>';
	var NETWORK_UUID 	= '<?php echo $NETWORK_UUID; ?>';
	var SGROUP_UUID 	= '<?php echo $SGROUP_UUID; ?>';
	var RCVY_PRE_SCRIPT  = '<?php echo $RCVY_PRE_SCRIPT; ?>';
	var RCVY_POST_SCRIPT = '<?php echo $RCVY_POST_SCRIPT; ?>';
	
	var privateIp 		= '<?php echo $privateIp; ?>';
	var publicIp 		= '<?php echo $publicIp; ?>';
	
	var diskType 		= '<?php echo $diskType; ?>';
	var availabilitySet	= '<?php echo $availabilitySet; ?>';
	var InstancesInfo = null;
	/* Begin to Automatic Exec */
	ListRecoveryScript();
	GetResourceGroup();
	getInstanceName();
	ListDataModeInstances();
	
	DetermineSegment('EditPlanInstanceAzureConfigurations');
	
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			BackToLastPage  = 'EditPlanSelectRecoverType';
			NextPage		= 'EditPlanRecoverAzureSummary';
		}
		else
		{
			BackToLastPage 	= 'PlanSelectRecoverType';
			NextPage		= 'PlanRecoverAzureSummary';
		}
	}
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#NextPage').prop('disabled', false);
		$('#NextPage').removeClass('btn-default').addClass('btn-primary');

		$('#BackToLastPage').prop('disabled', false);
		$('#CancelToMgmt').prop('disabled', false);
	});

	var resource_group ;
	
	$(document).ready(function(){
		if( diskType != '' ){
			$("#Disk_Type").val( diskType );
			$('.selectpicker').selectpicker('refresh');	
		}
	});
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
					<select id='RECOVERY_POST_SCRIPT' class='selectpicker' data-width='46%'></select>
					<button id='MgmtRecoveryScript'   class='btn btn-default pull-right btn-md' disabled><?php echo _("Add Script"); ?></button>
				</div>
				
				<div class='form-group'>
					<label for='comment'><?php echo _("Instance Type"); ?></label>
					<select id='FLAVOR_LIST' class='selectpicker' data-width='100%'></select>
				</div>
				
				<div class='form-group' >
					<label for='comment'><?php echo _("Security Group"); ?></label>
					<select id='SECURITY_GROUP' class='selectpicker' data-width='100%' data-dropup-auto="false"></select>
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("Internal Network ( virtual network | subnet )"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Specified Private IP"); ?>:</label>
					<label for='comment' style="width:33%"><?php echo _("Public IP"); ?></label>
					<select id='NETWORK_LIST' class='selectpicker' data-width='33.3%' disabled></select>					
					<input type='text' id='SPECIFIED_PRIVATE_IP' class='form-control' value='' style="width:33%" placeholder='Private IP'>
					<select id='PUBLIC_IP' class='selectpicker' data-width='33%' data-dropup-auto="false" >
					<option selected="selected">Yes</option>
					<option>No</option>
					</select>					
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("Availability Set"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("Disk Type"); ?></label>					
					<label for='comment' style="width:33%"><?php echo _("Instance Name"); ?></label>
					
					<select id='Availability_Set' class='selectpicker' data-width='33.3%' data-dropup-auto="false"></select>
					<select id='Disk_Type' class='selectpicker' data-width='33%' data-dropup-auto="false" >
					<option selected="selected">HDD</option>
					<option>SSD</option>
					</select>
					<input type='text' id='HOSTNAME_TAG' class='form-control' value='<?php echo $HOSTNAME_TAG; ?>' placeholder='' style="width:33%">
				</div>		
			</div>
			
			<div class="form-group form-inline">
				<label for='comment' style="width:33.3%"><?php echo _("Data Mode Host"); ?></label>
				<label for='comment' style="width:66%"></label>

				<select id='DATAMODE_INSTANCE' class='selectpicker' data-width='33%' onchange="SetBootCheckBox();"></select>
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