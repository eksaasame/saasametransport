<script>
	function CustomizeRecoveryType()
	{
		CLOUD_TYPE = '<?php echo $_SESSION['CLOUD_TYPE']; ?>';
		if (CLOUD_TYPE == 'Ctyun')
		{
			$("input[type=radio][value=RECOVERY_PM]").attr('checked', 'checked');
			$("input[type=radio][value=RECOVERY_DR]").prop("disabled",true);
			$("input[type=radio][value=RECOVERY_DT]").prop("disabled",true);
		}
	}
	
	QueryReplicaConfiguration();
	
	/* Query Replica Configuration */
	function QueryReplicaConfiguration(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'      :'QueryReplicaConfiguration',
				 'REPL_UUID'   :'<?php echo $_SESSION['REPL_UUID']; ?>'
			},
			success:function(jso)
			{
				/* CHANGE PLANNED MIGRATION STATUS  */
				if (jso.is_azure_blob_mode == true)
				{
					$("#RCRY_TYPE").prop("disabled", true);
				}
				else
				{
					if (jso.is_executing == true)
					{
						$("#RCRY_TYPE").prop("disabled", true);
					}
					else
					{
						$("#RCRY_TYPE").prop("disabled", jso.is_migration_executing);
					}
				}

				if( jso.cloud_type == "VMWare" && jso.DRcount > 0 )
					$("#RCRY_TYPE").prop("disabled", true);

				if( jso.cloud_type == "VMWare" && jso.DRexit > 0 )
					$("#RCRY_TYPE_DR").prop("disabled", true);
				
				//CustomizeRecoveryType();
			},
			error: function(xhr)
			{
				
			}
		});		
	}

	/* Recovery Next Page */
	function RecoveryNextRoute(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'     :'RecoveryNextRoute',
				 'SERV_REGN'  :'<?php echo $_SESSION['SERV_REGN']; ?>',
				 'CLOUD_TYPE' :'<?php echo $_SESSION['CLOUD_TYPE']; ?>',
				 'RECY_TYPE'  :$("#RecoveryTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = jso.Route;
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.Route,						
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
		
		$("#BackToSelectRecoverHost").click(function(){
			window.location.href = "SelectRecoverHost";
		})
		
		$("#NextToSelectListSnapshot").click(function(){
			RecoveryNextRoute();
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
					<li style='width:21%' class='active'><a><?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:21%'><a>				<?php echo _("Step 3 - Select Disk / Snapshot"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 4 - Configure Instance"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 5 - Recovery Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
					<table id="RecoveryTable">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="200px"><?php echo _("Type"); ?></th>						
							<th width="840px"><?php echo _("Description"); ?></th>
						</tr>
						</thead>
					
						<tbody>
							<tr height="55px">
								<td class="TextCenter"><input type="radio" id="RCRY_TYPE" name="RCRY_TYPE" value="RECOVERY_PM"></td>
								<td><?php echo _("Planned Migration"); ?></td>
								<td><?php echo _("Migrates a chosen workload state to the target environment."); ?></td>
							</tr>
							<tr height="55px">
								<td class="TextCenter"><input type="radio" id="RCRY_TYPE_DR" name="RCRY_TYPE" value="RECOVERY_DR"></td>
								<td><?php echo _("Disaster Recovery"); ?></td>
								<td><?php echo _("Resumes a recent workload state in the event of a disaster."); ?></td>
							</tr>
							<tr height="55px">
								<td class="TextCenter"><input type="radio" id="RCRY_TYPE" name="RCRY_TYPE" value="RECOVERY_DT" checked></td>
								<td><?php echo _("Development Testing"); ?></td>
								<td><?php echo _("Creates a test environment using chosen snapshot."); ?></td>
							</tr>							
						</tbody>
					</table>
				</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToSelectRecoverHost' 	class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>		
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>
					<button id='NextToSelectListSnapshot'	class='btn btn-primary pull-right btn-lg'><?php echo _("Next"); ?></button>								
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>
