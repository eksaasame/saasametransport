<?php
	if (isset($_SESSION['EDIT_PLAN']))
	{
		$SERV_REGN	  = $_SESSION['EDIT_PLAN'] -> ServiceRegin;
		$CLOUD_TYPE   = $_SESSION['EDIT_PLAN'] -> CloudType;
		$RECOVER_TYPE = $_SESSION['EDIT_PLAN'] -> RecoverType;
	}
	else
	{
		$SERV_REGN	  = $_SESSION['SERV_REGN'];
		$CLOUD_TYPE   = $_SESSION['CLOUD_TYPE'];
		$RECOVER_TYPE = 'RECOVERY_DT';
	}
?>

<script>
$( document ).ready(function() {	
	DetermineSegment('EditPlanSelectRecoverType');
	/* Determine URL Segment */
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();

		if (URL_SEGMENT == SET_SEGMENT)
		{
			$('input:radio[name="RCRY_TYPE"][value="<?php echo $RECOVER_TYPE; ?>"]').attr('checked', true);
			urlPrefix = 'Edit';
			$("#BackToLastPage").remove();
			
		}
		else
		{
			$('input:radio[name="RCRY_TYPE"][value="RECOVERY_DT"]').attr('checked', true);
			urlPrefix = '';
		}
	}
	
	/* Recovery Plan Next Page */
	function RecoveryPlanNextRoute(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'     :'RecoveryPlanNextRoute',
				 'SERV_REGN'  :'<?php echo $SERV_REGN; ?>',
				 'CLOUD_TYPE' :'<?php echo $CLOUD_TYPE; ?>',
				 'RECY_TYPE'  :$("#RecoveryTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					RouteURL = urlPrefix+jso.Route;
					window.location.href = RouteURL;
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
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverPlan";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = "PlanSelectRecoverReplica";
		})
		
		$("#NextPage").click(function(){
			RecoveryPlanNextRoute();
		})
	});
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
					<li style='width:25%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:25%' class='active'><a><?php echo _("Step 2 - Select Recovery Type"); ?></a></li>				
					<li style='width:25%'><a>		 		<?php echo _("Step 3 - Configure Instance"); ?></a></li>
					<li style='width:25%'><a>		 		<?php echo _("Step 4 - Recovery Plan Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
					<table id="RecoveryTable">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="200px">Type</th>						
							<th width="840px">Description</th>
						</tr>
						</thead>
					
						<tbody>
							<tr height="55px">
								<td class="TextCenter"><input type="radio" id="RCRY_TYPE" name="RCRY_TYPE" value="RECOVERY_PM" disabled></td>
								<td><?php echo _("Planned Migration"); ?></td>
								<td><?php echo _("Migrates a chosen workload state to the target environment."); ?></td>
							</tr>
							<tr height="55px">
								<td class="TextCenter"><input type="radio" id="RCRY_TYPE" name="RCRY_TYPE" value="RECOVERY_DR"></td>
								<td><?php echo _("Disaster Recovery"); ?></td>
								<td><?php echo _("Resumes a recent workload state in the event of a disaster."); ?></td>
							</tr>
							<tr height="55px">
								<td class="TextCenter"><input type="radio" id="RCRY_TYPE" name="RCRY_TYPE" value="RECOVERY_DT"></td>
								<td><?php echo _("Development Testing"); ?></td>
								<td><?php echo _("Creates a test environment using chosen snapshot."); ?></td>
							</tr>							
						</tbody>
					</table>
				</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToLastPage' class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>		
					<button id='CancelToMgmt' 	class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>
					<button id='NextPage'		class='btn btn-primary pull-right btn-lg'><?php echo _("Next"); ?></button>								
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>
