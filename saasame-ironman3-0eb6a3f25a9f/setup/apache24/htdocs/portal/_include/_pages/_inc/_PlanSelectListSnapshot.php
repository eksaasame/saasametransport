<?php
if (isset($_SESSION['EDIT_PLAN']))
{
	$REPL_UUID 	  = $_SESSION['EDIT_PLAN'] -> ReplUUID;
	$CLUSTER_UUID = $_SESSION['EDIT_PLAN'] -> ClusterUUID;
	$CLOUD_DISK	  = $_SESSION['EDIT_PLAN'] -> volume_uuid;
	$_SESSION["CLOUD_TYPE"]	  		= isset( $_SESSION['EDIT_PLAN'] -> CloudType )?$_SESSION['EDIT_PLAN'] -> CloudType:null;
	$_SESSION["IsAzureBlobMode"]	= isset( $_SESSION['EDIT_PLAN'] -> IsAzureBlobMode )?$_SESSION['EDIT_PLAN'] -> IsAzureBlobMode:null;
	$_SESSION["HOST_NAME"]	  		= isset( $_SESSION['EDIT_PLAN'] -> hostname_tag )?$_SESSION['EDIT_PLAN'] -> hostname_tag:null;
	$_SESSION["SERVER_UUID"]		= isset( $_SESSION['EDIT_PLAN'] -> ServerUUID )?$_SESSION['EDIT_PLAN'] -> ServerUUID:null;
	$_SESSION["SERV_REGN"]		= isset( $_SESSION['EDIT_PLAN'] -> ServiceRegin )?$_SESSION['EDIT_PLAN'] -> ServiceRegin:null;
}
else
{
	$REPL_UUID 	  = $_SESSION['REPL_UUID'];
	$CLUSTER_UUID = $_SESSION['CLUSTER_UUID'];
	$CLOUD_DISK   = '';
}
?>

<script>
	/* Begin to Automatic Exec */
	ListCloudDisk();

	DetermineSegment('EditPlanSelectListSnapshot');
	/* Determine URL Segment */
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			urlPrefix = 'Edit';
		}
		else
		{
			urlPrefix = '';
		}
	}

	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		if (typeof SNAPSHOT_STATUS == 'undefined')
		{
			$('#SelectServicePlanVolume').prop('disabled', false);
			$('#SelectServicePlanVolume').removeClass('btn-default').addClass('btn-primary');
		}
		
		$('#BackToServiceList').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
	});	
	
	function ListCloudDisk(){
			
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'ListCloudDisk',
				 'REPL_UUID' :'<?php echo $REPL_UUID; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					DISK_SELECT = '';					
					CLOUD_DISK = '<?php echo $CLOUD_DISK; ?>'.split(',');
					
					i=0;
					$.each(jso, function(key,value)
					{
						$('#SNAPSHOT_SELECT').append("<label for='comment'><?php echo _('Disk'); ?> "+key+":</label> <select id='Disk"+key+"' class='selectpicker' data-width='96%'></select><br><br>");
						
						DISK_SELECT = DISK_SELECT + '#Disk'+key+',';
						is_select = CLOUD_DISK[i] == 'false' ? true : false;
						QueryVolumeDetailInfo('Disk'+key,this.OPEN_DISK_ID,is_select,key);
						i++;
					});
					DISK_SELECT = DISK_SELECT.slice(0,-1);
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
	
	/* Query Volume Detail Info */
	function QueryVolumeDetailInfo(SelectKey,OpenStackDisk,IsSelect,key){

		var url = "mgmt_openstack.php";
		
		if( "<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Azure" && "<?php echo $_SESSION["IsAzureBlobMode"];?>" == false ){
			url = "mgmt_azure.php";
		}
		else if( "<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Azure" && "<?php echo $_SESSION["IsAzureBlobMode"];?>" == true ){
			$('#'+SelectKey+'').append(new Option("<?php echo $_SESSION["HOST_NAME"];?>"+"_"+key, OpenStackDisk, true, false));
			$('#'+SelectKey+'').append(new Option('<?php echo _("-- Do not recover this disk --"); ?>', false, true, IsSelect));
			return;
		}
		else if ( "<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Ctyun"){
			url = "mgmt_ctyun.php";
		}
		else if ( "<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Aliyun"){
			url = "mgmt_aliyun.php";
		}

		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/'+url,
			data:{
				 'ACTION'    	:'QueryVolumeDetailInfo',
				 'CLUSTER_UUID' :'<?php echo $CLUSTER_UUID; ?>',
				 'DISK_UUID' 	:OpenStackDisk,
				 'ServerId'  	:'<?php echo $_SESSION["SERVER_UUID"];?>',
				 'Zone'			:'<?php echo isset($_SESSION["SERV_REGN"])?$_SESSION["SERV_REGN"]:"";?>'		
			},
			success:function(jso)
			{
				if (jso != false)
				{					
					if( "<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Azure" && "<?php echo $_SESSION["IsAzureBlobMode"];?>" == false ){
						$('#'+SelectKey+'').append(new Option(jso.tags.Name, jso.id, true, false));
						$('#'+SelectKey+'').append(new Option('<?php echo _("-- Do not recover this disk --"); ?>', false, true, IsSelect));
					}
					else{
						$('#'+SelectKey+'').append(new Option(jso.volume_name, jso.volume_id, true, false));
						$('#'+SelectKey+'').append(new Option('<?php echo _("-- Do not recover this disk --"); ?>', false, true, IsSelect));
					}
				}
				$('#'+SelectKey+'').selectpicker('refresh');
			},
			error: function(xhr)
			{
	
			}
		});
	}

	/* SELECT SERVICE VOLUME */
	function SelectServicePlanVolume(){

		VOLUME_UUID = '';
		$(DISK_SELECT).each(function(){
			VOLUME_UUID = VOLUME_UUID + $(this).val()+',';
		});
		VOLUME_UUID = VOLUME_UUID.slice(0,-1);

		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'SelectServicePlanVolume',
				 'VOLUME_UUID' 	:VOLUME_UUID
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					if( "<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Azure" ){
						RouteURL = urlPrefix+"PlanInstanceAzureConfigurations";
					}
					else if ("<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Ctyun"){
						RouteURL = urlPrefix+"PlanInstanceCtyunConfigurations";
					}
					else if ("<?php echo $_SESSION["CLOUD_TYPE"];?>" == "Aliyun"){
						RouteURL = urlPrefix+"PlanInstanceAliyunConfigurations";
					}
					else{
						RouteURL = urlPrefix+"PlanInstanceConfigurations";
					}
					window.location.href = RouteURL;
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: '<?php echo _("Please select at least one recovery volume."); ?>',						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
						}}],
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});	
	}
	
	
	/* Submit Trigger */
	$(function(){
		$("#BackToMgmtRecoverWorkload").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToServiceList").click(function(){
			window.location.href = "PlanSelectRecoverType";
		})
		
		$("#SelectServicePlanVolume").click(function(){
			SelectServicePlanVolume();
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
					<li style='width:17%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:21%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:21%' class='active'><a><?php echo _("Step 3 - Select Volume"); ?></a></li>
					<li style='width:19%'><a>	 			<?php echo _("Step 4 - Configure Instance"); ?></a></li>
					<li style='width:22%'><a>		 		<?php echo _("Step 5 - Recovery Plan Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<div id='SNAPSHOT_SELECT' class='form-group'></div>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToServiceList' 	  		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectServicePlanVolume'	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>