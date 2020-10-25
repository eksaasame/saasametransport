<?php

$_SESSION['VOLUME_UUID'] = "";
if (isset($_SESSION['EDIT_PLAN']))
{
	$_SESSION['CLOUD_TYPE'] = $_SESSION['EDIT_PLAN'] -> CloudType;
	$_SESSION['CLUSTER_UUID'] = $_SESSION['EDIT_PLAN'] -> ClusterUUID;	
	
	$NETWORK_UUID = $_SESSION['EDIT_PLAN'] -> network_uuid;
	
	$RCVY_PRE_SCRIPT = $_SESSION['EDIT_PLAN'] -> rcvy_pre_script;
	$RCVY_POST_SCRIPT=$_SESSION['EDIT_PLAN'] -> rcvy_post_script;
	$_SESSION['SERV_REGN']	  	= $_SESSION['EDIT_PLAN'] -> ServiceRegin;
	$_SESSION['SERV_UUID']	  	= $_SESSION['EDIT_PLAN'] -> VM_NAME;
	$_SESSION['SERVER_UUID']	= $_SESSION['EDIT_PLAN'] -> ServerUUID;
	$_SESSION['REPL_OS_TYPE'] 	= $_SESSION['EDIT_PLAN'] -> HostType;
	$HOSTNAME_TAG 	 			= $_SESSION['EDIT_PLAN'] -> hostname_tag;
	$_SESSION['HOST_NAME'] = $_SESSION['EDIT_PLAN'] -> hostname_tag;
	
	$_SESSION['CPU']	  	= $_SESSION['EDIT_PLAN'] -> CPU;
	$_SESSION['Memory']	  	= $_SESSION['EDIT_PLAN'] -> Memory;
	$_SESSION['VMWARE_STORAGE']	= $_SESSION['EDIT_PLAN'] -> VMWARE_STORAGE;
	$_SESSION['VMWARE_FOLDER'] 	= $_SESSION['EDIT_PLAN'] -> VMWARE_FOLDER;
	$_SESSION['REPL_UUID'] = $_SESSION['EDIT_PLAN'] -> ReplUUID;
	$_SESSION['VMWARE_SCSI_CONTROLLER'] = $_SESSION['EDIT_PLAN'] -> VMWARE_SCSI_CONTROLLER;
	$_SESSION['CONVERT'] = $_SESSION['EDIT_PLAN'] -> CONVERT;
	$_SESSION['VM_TOOL'] = $_SESSION['EDIT_PLAN'] -> VM_TOOL;
	$_SESSION['FIRMWARE'] = $_SESSION['EDIT_PLAN'] -> FIRMWARE;
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
	$_SESSION['CPU']	  	= '';
	$_SESSION['Memory']	  	= '';
	$_SESSION['VMWARE_STORAGE']	= '';
	$_SESSION['VMWARE_FOLDER'] 	= '';
	$_SESSION['VMWARE_SCSI_CONTROLLER'] = '';
	$_SESSION['CONVERT'] = '';
	$_SESSION['VM_TOOL'] = '';
	$_SESSION['FIRMWARE'] = '';
}

?>

<?php include "_include/_pages/_inc/VMWareRequest.php"?>
<?php include "_include/_pages/_inc/AzureRequest.php"?>
<script>
	var FLAVOR_ID 			= '';
	var NETWORK_UUID 		= <?php echo json_encode($NETWORK_UUID); ?>;
	var SGROUP_UUID 		= '';
	var RCVY_PRE_SCRIPT  	= '';
	var RCVY_POST_SCRIPT 	= '';
	                           
	var CPU_val 			= '<?php echo $_SESSION['CPU']; ?>';
	var Memory_val 			= '<?php echo $_SESSION['Memory']; ?>';
	
	var NextPage 		= "PlanRecoverVMWareSummary";
	var RouteBack 	= 'PlanSelectRecoverType';
	
	var VMWARE_STORAGE 		= '<?php echo $_SESSION['VMWARE_STORAGE']; ?>';
	var VMWARE_FOLDER	= '<?php echo $_SESSION['VMWARE_FOLDER']; ?>';
	
	var VMWARE_SCSI_CONTROLLER	= '<?php echo $_SESSION['VMWARE_SCSI_CONTROLLER']; ?>';
	var CONVERT					= '<?php echo $_SESSION['CONVERT']; ?>';
	var VM_TOOL					= '<?php echo $_SESSION['VM_TOOL']; ?>';
	var FIRMWARE				= '<?php echo $_SESSION['FIRMWARE']; ?>';
	
	/* Define Portal Max Upload Size */
	MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;

	/* Begin to Automatic Exec */
	getVMWareSettingRange();
	
	DetermineSegment('EditPlanInstanceVMWareConfigurations');
	
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			RouteBack  = 'EditPlanSelectRecoverType';
			NextPage		= 'EditPlanRecoverVMWareSummary';
		}
	}
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#SelectVMWareInstanceConfig').prop('disabled', false);
		$('#SelectVMWareInstanceConfig').removeClass('btn-default').addClass('btn-primary');

		$('#BackToServiceList').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
	});

	var resource_group ;
	
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
				
				<div class="float_row" >
					<label for='comment' style="width:33%"><?php echo _("Instance Name"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("OS"); ?></label>
					<label for='comment' style="width:7%"><i class="fa fa-retweet" aria-hidden="true"></i> <?php echo _("Convert"); ?></label>
					<label for='comment' style="width:7%"><i class="fa fa-retweet" aria-hidden="true"></i> <?php echo _("VM Tool"); ?></label>
					<div class='form-group form-inline'>
						<input type='text' id='InstanceName' class='form-control' style="width:33%" value='<?php echo $_SESSION['HOST_NAME']; ?>' placeholder='' >
						<select id='OSType' class='selectpicker' data-width='33%' data-dropup-auto="false" onchange="getConfigDefault(this.value)">
							<option value=18 selected="selected">Microsoft Windows Server 2016 (64-bit)</option>
							<option value=19>Microsoft Windows Server 2012 R2 (64-bit)</option>
							<option value=0>Microsoft Windows Server 2012 (64-bit)</option>
							<option value=1>Microsoft Windows Server 2008 R2 (64-bit)</option>
							<option value=2>Microsoft Windows Server 2008 (64-bit)</option>
							<option value=3>Microsoft Windows Server 2008 (32-bit)</option>
							<option value=4>Microsoft Windows Server 2003 (64-bit)</option>
							<option value=5>Microsoft Windows Server 2003 (32-bit)</option>
							<option value=6>Microsoft Windows Server 2003 Datacenter (64-bit)</option>
							<option value=7>Microsoft Windows Server 2003 Datacenter (32-bit)</option>
							<option value=8>Microsoft Windows Server 2003 Standard (64-bit)</option>
							<option value=9>Microsoft Windows Server 2003 Standard (32-bit)</option>
							<option value=10>Microsoft Windows Server 2003 Web Edition (32-bit)</option>
							<option value=11>Microsoft Windows Small Business Server 2003</option>
							<option value=21>Microsoft Windows 10 (64-bit)</option>
							<option value=20>Microsoft Windows 10 (32-bit)</option>
							<option value=12>Microsoft Windows 8 (64-bit)</option>
							<option value=13>Microsoft Windows 8 (32-bit)</option>
							<option value=14>Microsoft Windows 7 (64-bit)</option>
							<option value=15>Microsoft Windows 7 (32-bit)</option>
							<option value=16>Microsoft Windows Vista (64-bit)</option>
							<option value=17>Microsoft Windows Vista (32-bit)</option>
							<option value=125>Red Hat Enterprise Linux 7 (64-bit)</option>
							<option value=126>Red Hat Enterprise Linux 7 (32-bit)</option>
							<option value=101>Red Hat Enterprise Linux 6 (64-bit)</option>
							<option value=102>Red Hat Enterprise Linux 6 (32-bit)</option>
							<option value=103>Red Hat Enterprise Linux 5 (64-bit)</option>
							<option value=104>Red Hat Enterprise Linux 5 (32-bit)</option>
							<option value=105>Red Hat Enterprise Linux 4 (64-bit)</option>
							<option value=106>Red Hat Enterprise Linux 4 (32-bit)</option>
							<option value=107>Red Hat Enterprise Linux 3 (64-bit)</option>
							<option value=123>SUSE Linux Enterprise 12 (64-bit)</option>
							<option value=124>SUSE Linux Enterprise 12 (32-bit)</option>
							<option value=109>SUSE Linux Enterprise 11 (64-bit)</option>
							<option value=110>SUSE Linux Enterprise 11 (32-bit)</option>
							<option value=111>SUSE Linux Enterprise 10 (64-bit)</option>
							<option value=112>SUSE Linux Enterprise 10 (32-bit)</option>
							<option value=113>SUSE Linux Enterprise 8/9 (64-bit)</option>
							<option value=114>SUSE Linux Enterprise 8/9 (32-bit)</option>
							<option value=115>CentOS 4/5/6/7 (64-bit)</option>
							<option value=116>CentOS 4/5/6/7 (32-bit)</option>
							<option value=129>Debian GNU/Linux 8 (64-bit)</option>
							<option value=130>Debian GNU/Linux 8 (32-bit)</option>
							<option value=127>Debian GNU/Linux 7 (64-bit)</option>
							<option value=128>Debian GNU/Linux 7 (32-bit)</option>
							<option value=117>Debian GNU/Linux 6 (64-bit)</option>
							<option value=118>Debian GNU/Linux 6 (32-bit)</option>
							<option value=119>Oracle Linux 4/5/6/7 (64-bit)</option>
							<option value=120>Oracle Linux 4/5/6 (32-bit)</option>
							<option value=121>Ubuntu Linux (64-bit)</option>
							<option value=122>Ubuntu Linux (32-bit)</option>
						</select>
						<input id="CONVERT" data-toggle="toggle" data-width="7%" data-style="slow" type="checkbox" checked>
						<input id="VM_TOOL" data-toggle="toggle" data-width="7%" data-style="slow" type="checkbox" checked>
					</div>
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:33.3%"><?php echo _("ESX Server"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("VM Folder"); ?>:</label>
					<label for='comment' style="width:33%"><?php echo _("Firmware"); ?></label>
					<select id='ESX_Server' class='selectpicker' data-width='33%' data-dropup-auto="false" disabled></select>
					<select id='VMWARE_FOLDER' class='selectpicker' data-width='33%' data-dropup-auto="false"></select>
					<select id='FIRMWARE' class='selectpicker' data-width='33%' data-dropup-auto="false">
						<option value="0" selected="selected">FIRMWARE BIOS</option>
						<option value="1">FIRMWARE EFI</option>
					</select>
				</div>
				
				<div class="form-group form-inline">
					<label for='comment' style="width:16%"><?php echo _("CPU"); ?></label>
					<label for='comment' style="width:16.5%"><?php echo _("Memory (MB)"); ?>:</label>
					<label for='comment' style="width:33.3%"><?php echo _("ESX Datastores"); ?></label>
					<label for='comment' style="width:33%"><?php echo _("VM SCSI Controller"); ?></label>
					<input id='Cpu' type='number' min=1 value=1 class='form-control' style="width:16%" placeholder=<?php echo _("Cores"); ?>></select>
					<input id='Memory' type='number' min=4 value=512 class='form-control' style="width:16.5%" placeholder=<?php echo _("Memory"); ?>></select>
					<select id='VMWARE_STORAGE' class='selectpicker' data-width='33%' data-dropup-auto="false"></select>
					<select id='VMWARE_SCSI_CONTROLLER' class='selectpicker' data-width='33%' data-dropup-auto="false"></select>
				</div>
			
				<div class="form-group form-inline" id="network_adapters">
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToServiceList' 	  		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectVMWareInstanceConfig' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>					
				</div>
			</div>
		</div>			
	</div> <!-- id: wrapper_block-->
</div>