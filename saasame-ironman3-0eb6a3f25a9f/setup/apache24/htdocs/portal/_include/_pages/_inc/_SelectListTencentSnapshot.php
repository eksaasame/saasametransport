<script>
	/* Begin to Automatic Exec */
	ListCloudDisk();

	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		if (SNAPSHOT_STATUS == true)
		{
			$('#SelectServiceSnapshot').prop('disabled', false);
			$('#SelectServiceSnapshot').removeClass('btn-default').addClass('btn-primary');
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
				 'REPL_UUID' :'<?php echo $_SESSION['REPL_UUID']; ?>'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					DISK_SELECT = '';
					
					$.each(jso, function(key,value)
					{
						$('#SNAPSHOT_SELECT').append("<label for='comment'><?php echo _('Disk'); ?> "+key+":</label> <select id='Disk"+key+"' class='selectpicker' data-width='96%'></select><br><br>");
						
						DISK_SELECT = DISK_SELECT + '#Disk'+key+',';
						ListAvailableDiskSnapshot('Disk'+key,this.OPEN_DISK_ID);
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
	
		
	/* List Available Snapshot */
	function ListAvailableDiskSnapshot(SelectKey,VolumeId){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_tencent.php',
			data:{
				 'ACTION'    	:'ListAvailableDiskSnapshot',
				 'CLUSTER_UUID' :'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'SERVER_ZONE'	:'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'DISK_UUID' 	:VolumeId
			},
			success:function(jso)
			{
				if (jso != false)
				{					
					SNAPSHOT_STATUS = false;
					$.each(jso, function(key,value)
					{
						if (value['status'] == 'normal')
						{
							var data = value['id'] ;
							$('#'+SelectKey+'').append(new Option(value['description'], data, true, true));
							SNAPSHOT_STATUS = true;
						}
						else
						{
							//$('#'+SelectKey+'').append(new Option(value['description']+' ('+'in progress'+')', data, true, true)).attr('disabled', true );
						}
					});
					$('#'+SelectKey+'').selectpicker('refresh');
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
	
	
	
	
	/* SELECT SERVICE SNAPSHOT */
	function SelectServiceSnapshot(){
		SNAP_UUID = '';
		$(DISK_SELECT).each(function()
		{
			SNAP_UUID = SNAP_UUID + $(this).val()+',';
		});
		/* Remove Comma */
		SNAP_UUID = SNAP_UUID.slice(0,-1);

		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SelectServiceSnapshot',
				 'SNAP_UUID' :SNAP_UUID
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = "InstanceTencentConfigurations";
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
		$("#BackToMgmtRecoverWorkload").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToServiceList").click(function(){
			window.location.href = "SelectRecoverPlan";
		})
		
		$("#SelectServiceSnapshot").click(function(){
			SelectServiceSnapshot();
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
					<li style='width:18%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:22%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 3 - Select Snapshot"); ?></a></li>
					<li style='width:20%'><a>	 			<?php echo _("Step 4 - Configure Instance"); ?></a></li>
					<li style='width:20%'><a>		 		<?php echo _("Step 5 - Recovery Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<div id='SNAPSHOT_SELECT' class='form-group'></div>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToServiceList' 	  		class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='SelectServiceSnapshot' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>