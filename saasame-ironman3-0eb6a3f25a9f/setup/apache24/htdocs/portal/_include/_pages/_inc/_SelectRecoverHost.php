<script>
	/* Begin to Automatic Exec */
	ReplicaUnsetSession();
	ListAvailableReplicaWithPlan();
	
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
	function ListAvailableReplicaWithPlan(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'ListAvailableReplicaWithPlan',
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
						if (this.IS_TEMPLATE == false)
						{
							REPL_UUID = this.REPL_UUID;
							
							REPLICA_TYPE = '<?php echo _("Replication"); ?>';
							
						}
						else
						{
							REPLICA_TYPE = '<?php echo _("Recovery Plan"); ?>';
							REPL_UUID = this.PLAN_UUID;
						}
						
						SELECT_DISABLED = (this.CLOUD_TYPE == 'Recover Kit' || this.CLOUD_TYPE == 'General Purpose' || this.CLOUD_TYPE == 'Ctyun') && this.HAS_RECOVERY_RUNNING == true ? ' disabled': '';
						
						MULTI_LAYER = this.MULTI_LAYER.charAt(0).toUpperCase() + this.MULTI_LAYER.slice(1);
						HOST_NAME = (this.MULTI_LAYER != 'NoSeries')? this.HOST_NAME+' ('+MULTI_LAYER+')':this.HOST_NAME;
												
						$('#ReplicaTable > tbody:last-child').append(
						'<tr id="'+this.REPL_UUID+'">\
							<td width="60px" class="TextCenter"><input type="radio" id="REPL_UUID" name="REPL_UUID" value="'+REPL_UUID+'|'+this.IS_TEMPLATE+'" '+SELECT_DISABLED+'/></td>\
							<td width="250px"><div class="HostListOverFlow">'+HOST_NAME+'</div></td>\
							<td width="520px"><div class="ServiceTextOverFlow">'+this.REPL_MESG+'</div></td>\
							<td width="180px">'+REPLICA_TYPE+'</td>\
							<td width="180px">'+this.REPL_TIME+'</td>\
						</tr>');
					});
					
					$('#NextPage').prop('disabled', false);
					$('#NextPage').removeClass('btn-default').addClass('btn-primary');
					
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
					$('#ReplicaTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("There are no workloads to recover. Please create a replication job first."); ?></td></tr>');					
				}
			},
			error: function(xhr)
			{
				$('#ReplicaTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
			}
		});
	}
	
	/* Select Recovery Type */
	function SelectRecoveryType(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'SelectServiceReplica',
				 'REPL_UUID' 	:$("#ReplicaTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					switch (jso.DIRECT_TO)
					{
						case 'WP':
							window.location.href = "RecoverKitSummary";
						break;
						
						case 'EX':
							window.location.href = "RecoverExportSummary";
						break;
						
						case 'RE':
							window.location.href = "SelectRecoverPlan";
						break;
						
						case 'RP':
							switch (jso.CLOUD_TYPE)
							{
								case 'OPENSTACK':
									window.location.href = "RecoverSummary";
								break;
								
								case 'AWS':
									window.location.href = "RecoverAwsSummary";
								break;
								
								case 'Azure':
									window.location.href = "RecoverAzureSummary";
								break;
								
								case 'Aliyun':
									window.location.href = "RecoverAliyunSummary";
								break;
								
								case 'Tencent':
									window.location.href = "RecoverTencentSummary";
								break;
								
								case 'Ctyun':
									window.location.href = "RecoverCtyunSummary";
								break;
								
								case 'VMWare':
									window.location.href = "RecoverVMWareSummary";
								break;
							}
						break;
					}
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
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
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
				
		$("#NextPage").click(function(){
			SelectRecoveryType();
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
						<li style='width:18%' class='active'><a><?php echo _("Step 1 - Select Host"); ?></a></li>
						<li style='width:21%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
						<li style='width:21%'><a>				<?php echo _("Step 3 - Select Disk / Snapshot"); ?></a></li>
						<li style='width:20%'><a>	 			<?php echo _("Step 4 - Configure Instance"); ?></a></li>
						<li style='width:20%'><a>		 		<?php echo _("Step 5 - Recovery Summary"); ?></a></li>
					</ul>
				</div>
				
				<div id='form_block_wizard'>
					<table id="ReplicaTable" style="min-width:1190px;">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="250px"><?php echo _("Host Name"); ?></th>						
							<th width="520px"><?php echo _("Status"); ?></th>
							<th width="180px"><?php echo _("Type"); ?></th>
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
						<button id='BackToLastPage'  	class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>
						<button id='CancelToMgmt' 		class='btn btn-default pull-left btn-lg'><?php echo _("Cancel"); ?></button>
						<button id='NextPage' 			class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>										
					</div>
				</div>
			</div>
	</div> <!-- id: wrapper_block-->
</div>
