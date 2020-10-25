<script>
	<!-- Begin to Exec Get List -->
	ReflushPackerAsync();

	function ReflushPackerAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 :'ReflushPackerAsync',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'HOST_UUID' :'<?php echo $_SESSION['HOST_UUID']; ?>'
			},
			success:function(jso)
			{
				setTimeout(function(){QueryAvailableService();},1500);
			},
			error: function(xhr)
			{
				
			}
		});
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
				 'SERV_TYPE' :'Loader',
				 'SYST_TYPE' :'<?php echo $_SESSION['HOST_OS']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					/* Define Option Level */
					option_level = '<?php echo $_SESSION['optionlevel']; ?>';
					host_uuid = '<?php echo $_SESSION['HOST_UUID']; ?>';
					is_cascade = false;
					
					$.each(jso, function(key,value)
					{
						var list_replica = value['LIST_REPLICA'];
						var HOST_TYPE = (value['VENDOR_NAME'] == 'AzureBlob' && value['SERV_INFO']['is_promote'] == true)?value['HOST_TYPE']:value['VENDOR_NAME'];
						var mapObj = {UnknownVendorType:"General Purpose",OPENSTACK:"OpenStack",OpenStack:"OpenStack",AzureBlob:"Azure",Azure:"Azure",Aliyun:"Alibaba Cloud",Ctyun:"天翼云"};
						var HOST_TYPE = HOST_TYPE.replace(/UnknownVendorType|OPENSTACK|AzureBlob|Aliyun|Ctyun/gi, function(matched){return mapObj[matched];}); /* Display Rewrtie */
						HOST_TYPE = (value['SERV_LOCA'] == 'Local Recover Kit')?"Recover Kit":HOST_TYPE; /* Display as Local Recover Kit*/
						
						/* Define Value and Display */
						option_display = '['+HOST_TYPE+'] '+value['HOST_NAME']+' - '+value['SERV_ADDR'];
						option_value = value['SERV_UUID']+'|'+value['LAUN_UUID']+'|'+value['SERV_LOCA']+'|'+HOST_TYPE;

						if (option_level == 'USER')
						{
							if (value['LAUN_UUID'] != false && value['SERV_LOCA'] != 'On Premise')
							{
								$('#TARGET_SERVER').append(new Option(option_display, option_value, true, true));
							}
						}
						else
						{
							$('#TARGET_SERVER').append(new Option(option_display, option_value, true, true));
						}
			
						$.each(list_replica, function(replica_uuid,packer_uuid)
						{
							if (host_uuid == packer_uuid)
							{
								is_cascade = true;
								$("#TARGET_SERVER option[value='"+option_value+"']").attr("disabled",true);
							}						
						});
					});
					
					$('#TARGET_SERVER').children('option:enabled').eq(0).prop('selected',true);
					$('.selectpicker').selectpicker('refresh');
					
					if ($('#TARGET_SERVER').children('option:enabled').length != 0)
					{					
						$('#ConfigureWorkload').prop('disabled', false);
						$('#ConfigureWorkload').removeClass('btn-default').addClass('btn-primary');
					}
					
				}	
			},
			error: function(xhr)
			{

			}
		});
	}	
	
	/* Post Select Service UUID To Session */
	function SelectTargetServer(){
		
		/* TOKENIZE SELECT SERVER UUID */
		TARGET_SERV_UUID = $("#TARGET_SERVER").val().split("|");
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SelectTargetServer',
				 'LOAD_UUID' :TARGET_SERV_UUID[0],
				 'LAUN_UUID' :TARGET_SERV_UUID[1],
				 'SERV_TYPE' :TARGET_SERV_UUID[2],
				 'CLOUD_TYPE':TARGET_SERV_UUID[3]
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					if (is_cascade == false)
					{
						window.location.href = "ConfigureWorkload";
					}
					else
					{
						window.location.href = "ConfigureSchedule";
					}
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

	
	/* Submit Trigger */
	$(function(){
		$("#BackToMgmtPrepareWorkload").click(function(){
			window.location.href = "MgmtPrepareWorkload";
		})
		
		$("#BackToHostList").click(function(){
			window.location.href = "SelectRegisteredHost";
		})
		
		$("#ConfigureWorkload").click(function(){
			SelectTargetServer();  
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-files-o fa-fw"></i>&nbsp;<?php echo _("Replication"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:20%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 2 - Select Target"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 3 - Configure Host"); ?></a></li>
					<li style='width:20%'><a>	 			<?php echo _("Step 4 - Configure Replication"); ?></a></li>
					<li style='width:20%'><a>		 		<?php echo _("Step 5 - Replication Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<label for='comment'><?php echo _("Target Transport Server:"); ?></label>
				<div class='form-group'>	
					<select id="TARGET_SERVER" class="selectpicker" data-width="100%"></select>
				</div>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToHostList' 			class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>
					<button id='BackToMgmtPrepareWorkload' 	class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>	
					<button id='ConfigureWorkload'			class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>					
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block_wizard-->
</div>
