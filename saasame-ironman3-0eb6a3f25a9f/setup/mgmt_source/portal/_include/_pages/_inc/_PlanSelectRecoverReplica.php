<script>
	/* Begin to Automatic Exec */
	ReplicaUnsetSession();
	ListAvailableReplica();
	
	/* REPLICA UNSET SESSION */
	function ReplicaUnsetSession(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'ReplicaUnsetSession',
			},			
		});
	}
	
	/* FIRST LIST REPLICA */
	function ListAvailableReplica(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'ListAvailableReplica',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
			},
			success:function(jso)
			{
				$('.default').remove();
				
				if (jso != false)
				{					
					count = Object.keys(jso).length;
				
					$.each(jso, function(key,value)
					{
						if (this.EXPORT_JOB == 'N' && this.WINPE_JOB == 'N')
						{
							SELECT_CONTROL = '';
						}
						else
						{
							SELECT_CONTROL = ' disabled';
						}
						
						if (this.EXPORT_JOB == 'Y')
						{
							JOB_TYPE = '<?php echo _("Image Export"); ?>';
						}
						else if(this.WINPE_JOB == 'Y')
						{
							JOB_TYPE = '<?php echo _("Recovery Kit"); ?>';
						}
						else
						{
							JOB_TYPE = '<?php echo _("Replication"); ?>';
						}
						
						MULTI_LAYER = this.MULTI_LAYER.charAt(0).toUpperCase() + this.MULTI_LAYER.slice(1);				
						HOST_NAME = (this.MULTI_LAYER != 'NoSeries')?this.HOST_NAME+' ('+MULTI_LAYER+')':this.HOST_NAME;
						
						$('#ReplicaTable > tbody:last-child').append(
						'<tr id="'+this.REPL_UUID+'">\
							<td width="60px" class="TextCenter"><input type="radio" id="REPL_UUID" name="REPL_UUID" value="'+this.REPL_UUID+'" '+SELECT_CONTROL+' /></td>\
							<td width="250px"><div class="HostListOverFlow">'+HOST_NAME+'</div></td>\
							<td width="500px"><div class="ServiceTextOverFlow">'+this.REPL_MESG+'</div></td>\
							<td width="200px">'+JOB_TYPE+'</td>\
							<td width="180px">'+this.REPL_TIME+'</td>\
						</tr>');
					});
					
					$('#SelectServiceReplica').prop('disabled', false);
					$('#SelectServiceReplica').removeClass('btn-default').addClass('btn-primary');
					
					if (count > 10)
					{
						$(document).ready(function()
						{
							$('#ReplicaTable').DataTable(
							{
								paging		: true,
								bFilter		: true,
								lengthChange: false,
								pageLength	: 10,
								pagingType	: "simple_numbers",
								order: [],
								columnDefs: [ {
									targets	: 'TextCenter',
									orderable: false
								}],
								ordering	: true,
								aoColumns	:[
												null,
												null,
												null,
												null
								],
								language	: {
									search: '_INPUT_'
								}
							});
							
							$('div.dataTables_filter input').attr("placeholder",$.parseHTML("&#xF002; Search")[0].data);
							$('div.dataTables_filter input').addClass("form-control input-sm");
							$('div.dataTables_filter input').css("max-width", "180px");
							$('div.dataTables_filter input').css("padding-right", "80px");								
						});
					}					
				}
				else
				{
					$('#ReplicaTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("There are no workloads to recover. Please create a prepare workload process first."); ?></td></tr>');					
				}
			},
			error: function(xhr)
			{
				$('#ReplicaTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
			}
		});
	}
	
	/* SELECT SERVICE REPLICA */
	function SelectServiceReplica(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'SelectServiceReplica',
				 'REPL_UUID' :$("#ReplicaTable input[type='radio']:checked").val(),
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					if (jso.SKIP_TYPE == 'WP')
					{	
						window.location.href = "RecoverKitSummary";
					}
					else if (jso.SKIP_TYPE == 'EX')
					{
						window.location.href = "RecoverExportSummary";
					}
					else
					{
						window.location.href = "PlanSelectRecoverType";
					}
				}
				else
				{
					BootstrapDialog.show({
						title: "<?php echo _("Service Message"); ?>",
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.Code,						
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
		$("#BackToLastPage").click(function(){
			window.location.href = "MgmtRecoverPlan";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverPlan";
		})
				
		$("#SelectServiceReplica").click(function(){
			SelectServiceReplica();
		})
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
						<li style='width:25%' class='active'><a><?php echo _("Step 1 - Select Host"); ?></a></li>
						<li style='width:25%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>				
						<li style='width:25%'><a>		 		<?php echo _("Step 3 - Configure Instance"); ?></a></li>
						<li style='width:25%'><a>		 		<?php echo _("Step 4 - Recovery Plan Summary"); ?></a></li>
					</ul>
				</div>
				
				<div id='form_block_wizard'>
					<table id="ReplicaTable" style="min-width:1190px;">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="250px"><?php echo _("Host Name"); ?></th>						
							<th width="500px"><?php echo _("Status"); ?></th>
							<th width="200px"><?php echo _("Job Type"); ?></th>
							<th width="180px"><?php echo _("Time"); ?></th>
						</tr>
						</thead>
					
						<tbody>
							<tr class="default">								
								<td colspan="5" class="padding_left_30"><?php echo _("Checking management connection..."); ?></td>
							</tr>
						</tbody>
					</table>
				</div>
				
				<div id='title_block_wizard'>
					<div class='btn-toolbar'>
						<button id='BackToLastPage'  		class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>
						<button id='CancelToMgmt' 			class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>
						<button id='SelectServiceReplica' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>										
					</div>
				</div>
			</div>
	</div> <!-- id: wrapper_block-->
</div>
