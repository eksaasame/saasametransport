<?php include "_include/_pages/_inc/AzureRequest.php"?>
<script>
	var FLAVOR_ID 			= '';
	var NETWORK_UUID 		= '';
	var SGROUP_UUID 		= '';
	var RCVY_PRE_SCRIPT  	= '';
	var RCVY_POST_SCRIPT 	= '';
	                           
	var privateIp 			= '';
	var publicIp 			= '';
	
	var NextPage = "RecoverAzureSummary";
	
	var diskType 		= '';
	var availabilitySet	= '';
	
	var InstancesInfo = null;
	/* Define Portal Max Upload Size */
	MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;

	/* Begin to Automatic Exec */
	SummaryPageRoute();
	ListRecoveryScript();
	GetResourceGroup();
	getInstanceName();
	ListDataModeInstances();
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#SelectAzureInstanceConfig').prop('disabled', false);
		$('#SelectAzureInstanceConfig').removeClass('btn-default').addClass('btn-primary');

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
				
				
				<div class='form-group'>
					<label for='comment' style="width:46%"><?php echo _("Instance Type"); ?></label>
					<label for='comment' style="width:39%"><?php echo _("Data Mode Host"); ?></label>
					<label for='comment' style="width:7%"><?php echo _("Power"); ?></label>
					<select id='FLAVOR_LIST' class='selectpicker' data-width='46%'></select>
					<select id='DATAMODE_INSTANCE' class='selectpicker' data-width='39%' onchange="SetBootCheckBox();"></select>
					<input id="DATAMODE_POWER" data-toggle="toggle" data-width="7%" data-style="slow" type="checkbox" disabled data-on="<?php echo _('On'); ?>" data-off="<?php echo _('Off'); ?>">
					<button id='ConfigureDataModeAgent' class='btn btn-default pull-right btn-md' style="width:7%" disabled><?php echo _("Agent"); ?></button>
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
					<input type='text' id='HOSTNAME_TAG' class='form-control' value='<?php echo $_SESSION['HOST_NAME']; ?>' placeholder='' style="width:33%">
				</div>			
				
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToServiceList' 	  		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectAzureInstanceConfig' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>					
				</div>
			</div>
		</div>			
	</div> <!-- id: wrapper_block-->
</div>