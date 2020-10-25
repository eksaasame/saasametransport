<script>
	/* Begin to Automatic Exec */

	$( document ).ready(function() {
		ListCloudDisk();
	});

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
		
		$('#SNAPSHOT_SELECT').append("<label for='comment'><?php echo _("VM Snapshot"); ?>:</label><select id='vm_snapshot' class='selectpicker' data-width='100%'></select><br><br>");
		
		ListVMWareSnapshot( );
		$('.selectpicker').selectpicker('refresh');	
	}
	
	function ListVMWareSnapshot( ){
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'    	: 'ListSnapshot',
				 'CLUSTER_UUID' : '<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'ReplicatId'	: '<?php echo $_SESSION['REPL_UUID']; ?>'
			},
			success:function(jso)
			{
				SNAPSHOT_STATUS = false;
				$.each(jso, function(key,value){
					$('#vm_snapshot').append(new Option("Snapshot Created By SaaSaMe Transport Service @ "+value['time'], value['name'], true, true));
				});
				
				SNAPSHOT_STATUS = true;
				$('#vm_snapshot').selectpicker('refresh');

			},
			error: function(xhr)
			{
			}
		});
	}	
	
	/* SELECT SERVICE SNAPSHOT */
	function SelectServiceSnapshot(){
	
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SelectServiceSnapshot',
				 'SNAP_UUID' : $('#vm_snapshot').val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = "InstanceVMWareConfigurations";
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
					<i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?>
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