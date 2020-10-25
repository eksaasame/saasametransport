<script>
$( document ).ready(function() {
	/* Begin to Automatic Exec */
	UnsetSession();
	ListAvailableReplica();
	DebugOptionLevel();
	
	/* Show Debug Option */
	function DebugOptionLevel(){
		option_level = '<?php echo $_SESSION['optionlevel']; ?>';
		if (option_level == 'DEBUG')
		{
			$('.debug_hidden').show();
		}
	}
	
	/* Session Unset */
	function UnsetSession(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'UnsetSession'
			},			
		});
	}
	
	/* Define Prepare DataTable */
	function PrepareDataTable(count)
	{
		if (count > 10)
		{
			$status = true;
		}
		else
		{
			$status = false;
		}
		
		PrepareTable = $('#ReplicaTable').DataTable(
		{
			paging		 : $status,
			bInfo		 : $status,
			bFilter		 : $status,
			lengthChange : false,
			pageLength	 : 10,
			bAutoWidth	 : false,
			pagingType	 : "simple_numbers",
			order: [],
			columnDefs   : [ {
				targets: 'TextCenter',
				orderable: false,
				targets: [0,6]			
			}],
			ordering	 : $status,			
			language	: {
				search: '_INPUT_'
			},
			/*oLanguage: {
				sEmptyTable:'Please click New to start a preparation process.'
			},*/
			stateSave	: true
		});
		
		$('div.dataTables_filter input').attr("placeholder",$.parseHTML("&#xF002; Search")[0].data);
		$('div.dataTables_filter input').addClass("form-control input-sm");
		$('div.dataTables_filter input').css("max-width", "180px");
		$('div.dataTables_filter input').css("padding-right", "80px");
	}
	
	/* List Replica */
	function ListAvailableReplica(){
		Pace.ignore(function(){
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
					
					if (jso.Code != false)
					{
						$('#AddNewReplica').prop('disabled', false);
						$('#AddNewReplica').removeClass('btn-default').addClass('btn-primary');
						
						if (jso != false)
						{
							/* Define Replica DataTable */
							PrepareDataTable(Object.keys(jso).length);
					
							/* Clear Replica Table Data */
							PrepareTable.clear();
					
							$.each(jso, function(key,value)
							{
								MULTI_LAYER = this.MULTI_LAYER.charAt(0).toUpperCase() + this.MULTI_LAYER.slice(1);				
								HOST_NAME = (this.MULTI_LAYER != 'NoSeries')?this.HOST_NAME+' ('+MULTI_LAYER+')':this.HOST_NAME;
								DIV_HOST_NAME = (HOST_NAME.length > 15)?'<div class="HostOverFlow">'+HOST_NAME+'<span class="tooltiptext">'+HOST_NAME+'</span></div>':'<div class="HostOverFlow">'+HOST_NAME+'</div>';						

								PrepareTable.row.add(
								[
									'<div class="TextCenter"><input type="radio" id="REPL_UUID" name="REPL_UUID" value="'+this.REPL_UUID+'"/><div>',
									DIV_HOST_NAME,
									'<div class="TextOverFlow">'+this.REPL_MESG+'</div><div id="TextOverHistory" class="TextOverHistory" data-repl="'+this.REPL_UUID+'"><i class="fa fa-commenting-o"></i></div>',
									this.PROGRESS_PRECENT+'/'+this.LOADER_PROGRESS_PRECENT+'%',
									this.REPL_TIME,									
									'<div class="TextCenter TopologyLink"><div id="NetworkTopology" replica-detail="'+this.REPL_UUID+'" style="cursor:pointer;"><i class="fa fa-random fa-lg"></i></div></div>',
									'<div class="TextCenter '+this.MULTI_LAYER+'"><div id="ReplicaInfo" replica-detail="'+this.REPL_UUID+'" style="cursor:pointer;"><i class="fa fa-sliders fa-lg"></i></div></div>'
								]).draw(false).columns.adjust();
							});
													
							$("#ReplicaTable").on("click", "#REPL_UUID", function(e){
								var REPL_UUID = $(this).attr('value'); 
								OnClickSelectReplica(REPL_UUID);
							});
							
							$("#ReplicaTable").on("click", "div.TextOverHistory", function(e){
								$('#TextOverHistory').prop('disabled', true);
								var REPL_UUID = $(this).attr('data-repl');  //use  .attr() for accessing attibutes
								DisplayJobHistory(REPL_UUID); 
							});
							
							$("#ReplicaTable").on("click", "#NetworkTopology", function(e){
								$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
								$('#ReplicaInfo').prop('disabled', true);
								var REPL_UUID = $(this).attr('replica-detail');
								DisplayNetworkTopology(REPL_UUID);
							});	
							
							$("#ReplicaTable").on("click", "#ReplicaInfo", function(e){
								$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
								$('#ReplicaInfo').prop('disabled', true);
								var REPL_UUID = $(this).attr('replica-detail');
								DisplayPrepareJobInfo(REPL_UUID);
							});
							
							if (Object.keys(jso).length < 20)
							{
								SetSeconds = 2000;
							}
							else
							{
								SetSeconds = 5000;
							}

							Interval = setInterval(IntervalUpdateReplica, SetSeconds);
							
							$(window).on("blur focus", function(e)
							{
								var prevType = $(this).data("prevType");
								if (prevType != e.type)
								{
									switch (e.type) {
										case "blur":
											clearInterval(Interval);
											Interval = setInterval(IntervalUpdateReplica, 5000);
										break;									
										case "focus":
											clearInterval(Interval);
											Interval = setInterval(IntervalUpdateReplica, SetSeconds);
										break;
									}
								}
								$(this).data("prevType", e.type);
							})
						}
						else
						{
							$('#ReplicaTable > tbody').append('<tr><td colspan="7" class="padding_left_30"><?php echo _("Click New to create a new replication process."); ?></td></tr>');					
						}
					}
					else
					{
						$('#ReplicaTable > tbody').append('<tr><td colspan="7" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
					}
				},
				error: function(xhr)
				{
					
				}
			});
		});
	}
	
	/* OnClick Select Replica */
	function OnClickSelectReplica(REPL_UUID){
		Pace.ignore(function(){
			$.ajax({
				type: 'POST',
				dataType:'JSON',
				url: '_include/_exec/mgmt_service.php',
				data:{
					 'ACTION'    :'QueryReplicaInformation',
					 'REPL_UUID' :REPL_UUID
				},
				success:function(jso)
				{
					$('#RepairReplica').prop('disabled', false);
					$('#RepairReplica').removeClass('btn-default').addClass('btn-warning');
					
					$('#DebugDelete').prop('disabled', false);
					$('#DebugDelete').removeClass('btn-default').addClass('btn-danger');
					
					DELETE_CONTROL = jso.JOBS_JSON['delete_control'];
					DELETE_CONTROL = true;
					
					if (DELETE_CONTROL == true)
					{
						$('#DeleteReplica').prop('disabled', false);
						$('#EditReplica').prop('disabled', false);
			
						$('#DeleteReplica').removeClass('btn-default').addClass('btn-primary');
						$('#EditReplica').removeClass('btn-default').addClass('btn-primary');
												
						if (jso.JOBS_JSON['sync_control'] == true)
						{
							option_level = '<?php echo $_SESSION['optionlevel']; ?>';
									
							$('#SyncReplica').prop('disabled', false);
							$('#SyncReplica').removeClass('btn-default').addClass('btn-primary');
							
							RadioBoxHTML = $('<div></div>');
							RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectSyncType" id="DeltaSync" value="DeltaSync" checked/><label for="DeltaSync" id="DeltaSyncLabel" style="color:#494949;"><?php echo _("Delta Replication"); ?></label><div class="indentedtext"><?php echo _("Replicates changed data"); ?></div></div>');
							RadioBoxHTML.append('<div style="height:2px;"></div>');
							RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectSyncType" id="FullSync" value="FullSync" /><label for="FullSync" id="FullSyncLabel" style="color:#494949;"><?php echo _("Full Replication"); ?></label><div class="indentedtext"><?php echo _("Replicates all data (long replication time)​"); ?></div></div>');
							
							if (option_level == 'DEBUG')
							{					
								RadioBoxHTML.append('<div class="funkyradio-warning" style="width:200px; position:fixed; top:141px; left:390px; display:none;"><input type="checkbox" id="DebugSnapshot" disabled /><label for="DebugSnapshot" style="border-style:none; height:36px;"><?php echo _("Fource Clean up​"); ?></label></div>');
								RadioBoxHTML.append("<script>$(document).ready(function() {$('input[type=radio][name=SelectSyncType]').change(function(){if(this.id == 'DeltaSync'){$('#DebugSnapshot').attr('disabled', true);$('#DebugSnapshot').prop('checked', false);}else{$('#DebugSnapshot').attr('disabled', false); if (this.id == 'FullSync'){$('#FullSync').prop('checked', true);}}});});<\/script>");				
							}
							
							//Force to check full sync
							if (jso.JOBS_JSON['is_full_replica'] == true || jso.JOBS_JSON['all_init'] == false)
							{
								RadioBoxHTML.append("<script>$('#DeltaSync').attr('disabled', true); $('#DeltaSyncLabel').css('color','#C3C3C3'); $('#FullSync').prop('checked', true);<\/script>");
							}
							
							//RadioBoxHTML.append('<div class="radio-inline"><input type="radio" name="SyncType" value="full"> Full Replication</div>');
							//RadioBoxHTML.append('<div class="radio-inline"><input type="radio" name="SyncType" value="delta" checked> Delta Replication</div>');		
						}
						else
						{
							$('#SyncReplica').prop('disabled', true);
							$('#SyncReplica').removeClass('btn-primary').addClass('btn-default');
						}
					}
				},
				error: function(xhr)
				{
					$('#SyncReplica').prop('disabled', true);
					$('#DeleteReplica').prop('disabled', true);
					$('#EditReplica').prop('disabled', true);
					$('#RepairReplica').prop('disabled', true);
					$('#DebugDelete').prop('disabled', true);
											
					$('#SyncReplica').removeClass('btn-primary').addClass('btn-default');
					$('#DeleteReplica').removeClass('btn-primary').addClass('btn-default');
					$('#EditReplica').removeClass('btn-primary').addClass('btn-default');
					$('#RepairReplica').removeClass('btn-warning').addClass('btn-default');
					$('#DebugDelete').removeClass('btn-danger').addClass('btn-default');
				}
			});
		});
	}
	
	/* Interval Update Replica */
	function IntervalUpdateReplica(){
		Pace.ignore(function(){
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
					if (jso != false)
					{
						/* Clear Replica Table Data */
						PrepareTable.clear();
					
						/* GET LAST CHECK UUID */
						CheckedUUID = $("#ReplicaTable input[type='radio']:checked").val();

						$.each(jso, function(key,value)
						{
							if (CheckedUUID == this.REPL_UUID)
							{
								RadioChecked = ' checked';
							}
							else
							{	
								RadioChecked = '';
							}
							
							if (CheckedUUID != undefined)
							{
								OnClickSelectReplica(CheckedUUID);
							}
							else
							{
								$('#SyncReplica').prop('disabled', true);
								$('#DeleteReplica').prop('disabled', true);
								$('#EditReplica').prop('disabled', true);
								$('#RepairReplica').prop('disabled', true);
								$('#DebugDelete').prop('disabled', true);
							
								$('#SyncReplica').removeClass('btn-primary').addClass('btn-default');
								$('#DeleteReplica').removeClass('btn-primary').addClass('btn-default');
								$('#EditReplica').removeClass('btn-primary').addClass('btn-default');
								$('#RepairReplica').removeClass('btn-warning').addClass('btn-default');
								$('#DebugDelete').removeClass('btn-danger').addClass('btn-default');
							}
							
							MULTI_LAYER = this.MULTI_LAYER.charAt(0).toUpperCase() + this.MULTI_LAYER.slice(1);				
							HOST_NAME = (this.MULTI_LAYER != 'NoSeries')?this.HOST_NAME+' ('+MULTI_LAYER+')':this.HOST_NAME;
							DIV_HOST_NAME = (HOST_NAME.length > 15)?'<div class="HostOverFlow">'+HOST_NAME+'<span class="tooltiptext">'+HOST_NAME+'</span></div>':'<div class="HostOverFlow">'+HOST_NAME+'</div>';
								
							PrepareTable.row.add(
							[
								'<div class="TextCenter"><input type="radio" id="REPL_UUID" name="REPL_UUID" value="'+this.REPL_UUID+'"'+RadioChecked+'/><div>',
								DIV_HOST_NAME,
								'<div class="TextOverFlow">'+this.REPL_MESG+'</div><div id="TextOverHistory" class="TextOverHistory" data-repl="'+this.REPL_UUID+'"><i class="fa fa-commenting-o"></i></div>',
								this.PROGRESS_PRECENT+'/'+this.LOADER_PROGRESS_PRECENT+'%',
								this.REPL_TIME,
								'<div class="TextCenter TopologyLink"><div id="NetworkTopology" replica-detail="'+this.REPL_UUID+'" style="cursor:pointer;"><i class="fa fa-random fa-lg"></i></div></div>',
								'<div class="TextCenter '+this.MULTI_LAYER+'"><div id="ReplicaInfo" replica-detail="'+this.REPL_UUID+'" style="cursor:pointer;"><i class="fa fa-sliders fa-lg"></i></div></div>'
							]).draw(false);
						});
					}
					else
					{
						clearInterval(Interval);
						
						$('#SyncReplica').prop('disabled', true);
						$('#DeleteReplica').prop('disabled', true);
						$('#EditReplica').prop('disabled', true);
						$('#RepairReplica').prop('disabled', true);
						$('#DebugDelete').prop('disabled', true);
						
						$('#SyncReplica').removeClass('btn-primary').addClass('btn-default');
						$('#DeleteReplica').removeClass('btn-primary').addClass('btn-default');
						$('#EditReplica').removeClass('btn-primary').addClass('btn-default');
						$('#RepairReplica').removeClass('btn-warning').addClass('btn-default');
						$('#DebugDelete').removeClass('btn-danger').addClass('btn-default');
						
						PrepareTable.clear();
						PrepareTable = $('#ReplicaTable').DataTable( {
							destroy		 : true,
							paging		 : false,
							bInfo		 : false,
							bFilter		 : false,												
							bAutoWidth	 : false,						
							ordering	 : false,			
							stateSave	 : false,
							language	 : {
								sEmptyTable:'<div class="padding_left_30"><?php echo _("Click New to create a new replication process."); ?></div>'
							},						
						});					
					}
				},
				error: function(xhr)
				{
					
				}
			});
		});
	}
	
	
	/* Display Job History */
	function DisplayJobHistory(REPL_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			cache: false,
			data:{
				 'ACTION'     :'DisplayJobHistory',
				 'JOBS_UUID'  :REPL_UUID,
				 'ITEM_LIMIT' :250
			},
			success:function(msg)
			{
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: '<?php echo _("History"); ?>',
					cssClass: 'history-dialog',
					message: msg,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);
				
				$('#TextOverHistory').prop('disabled', false);				
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Sync Select Replica Async */
	function SyncSelectReplicaAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    		:'SyncSelectReplicaAsync',
				 'REPL_UUID' 		:$("#ReplicaTable input[type='radio']:checked").val(),
				 'SYNC_TYPE' 		:SyncType,
				 'DEBUG_SNAPSHOT'	:DebugSnapshot
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _("Service Message"); ?>",
					message: jso.msg,
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						$("#ReplicaTable input[type='radio']:checked").prop('checked', false);
						//window.location.href = "MgmtPrepareWorkload";
					},
				});
			},
		});		
	}
	
	/* Display Prepare Job Information */
	function DisplayPrepareJobInfo(REPL_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'     :'DisplayPrepareJobInfo',
				 'JOBS_UUID'  :REPL_UUID,
			},
			success:function(msg)
			{
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: "<?php echo _("Replication Information"); ?>",
					cssClass: 'workload-dialog',
					message: msg,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);
				
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#ReplicaInfo').prop('disabled', false);
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			}
		});		
	}
		
	/* Display Network Topology */
	function DisplayNetworkTopology(REPL_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'NetworkTopology',
				 'JOB_UUID'  :REPL_UUID,
				 'TYPE'		 :'Replication'
			},
			success:function(msg)
			{
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: "<?php echo _("Network Topology"); ?>",
					cssClass: 'network-topology-dialog',
					message: msg,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);
				
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#ReplicaInfo').prop('disabled', false);
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			}
		});		
	}	
		
	/* Delete Select Replica Async */
	function DeleteSelectReplicaAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'DeleteSelectReplicaAsync',
				 'REPL_UUID' :$("#ReplicaTable input[type='radio']:checked").val(),
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _("Service Message"); ?>",
					message: jso.msg,
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						$("#ReplicaTable input[type='radio']:checked").prop('checked', false);
						//window.location.href = "MgmtPrepareWorkload";
					},
				});
			},		
		});		
	}
	
	/* Select Edit Replica */
	function EditSelectReplica(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'EditSelectReplica', 
				 'REPL_UUID' :$("#ReplicaTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = "EditConfigureSchedule"
				}
				else
				{
					BootstrapDialog.show({
						title: "<?php echo _("Service Message"); ?>",
						message: jso.msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtPrepareWorkload"
						},
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Debug Repair Replica */
	function QueryRepairTransport(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'QueryRepairTransport',
				 'REPL_UUID' 	:$("#ReplicaTable input[type='radio']:checked").val(),
			},
			success:function(list_transport)
			{
				$('#RepairReplica').prop('disabled', true);
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _('Select Transport Server'); ?>',
						cssClass: 'snapshot-script-dialog',
						message: list_transport,
						draggable: true,
						closable: true,
						onhidden: function(dialogRef){
							$('#RepairReplica').prop('disabled', false);
						},
						buttons:
						[
							{
								id: 'REPAIR_BUTTON',
								label: '<?php echo _('Submit'); ?>',
								cssClass: 'btn-warning',
								action: function(dialogRef)
								{
									target_transport = $("#RepairReplica input[type='radio']:checked").val();
									security_code = $("#INPUT_SECURITY_CODE").val();
									
									/* Define valueable */
									is_error = false;
									error_message = '';
									
									/* Check select transport server */
									if (target_transport == undefined)
									{
										is_error = true;
										error_message = '<?php echo _('Please select a recovery transport server.'); ?>';
									}									
									
									/* Check input security code */
									if (security_code == '')
									{
										is_error = true;										
										error_message = error_message == '' ? '<?php echo _('Please input the security code. '); ?>' : '<?php echo _('Please select a recovery transport server and input the security code. '); ?>';
									}
									
									/* Executed */
									if (is_error == true)
									{
										BootstrapDialog.show({
											title: 'Message',
											draggable: true,
											cssClass: 'default-dialog warning',
											message: error_message
										});
									}
									else
									{		
										DebugRepairReplicaAsync();
										dialogRef.close();
									}
								}
							}
						],
						onshow: function(dialogRef) {						
							dialogRef.getModalFooter()
								.addClass('form-inline')
								.find('.bootstrap-dialog-footer-buttons')
								.prepend('<select id="RepairFullDelta" class="selectpicker pull-left" data-width="133px"><option value="delta"><?php echo _("Delta Sync"); ?></option><option value="full"><?php echo _("Full Sync"); ?></option></select><input type="text" id="INPUT_SECURITY_CODE" class="form-control" style="width:115px; border:1px solid #f0ad4e;" value="" placeholder="<?php echo _('Security Code'); ?>">&nbsp;');
						},					
					});
				}, 0);
			},
		});	
	}
	
	/* Debug Repair Replica Async */
	function DebugRepairReplicaAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	   :'DebugRepairReplicaAsync',
				 'REPL_UUID' 	   :$("#ReplicaTable input[type='radio']:checked").val(),
				 'TARGET_TRANSPORT':target_transport,
				 'REPAIR_SYNC_MODE':$("#RepairFullDelta").val(),
				 'SECURITY_CODE'   :$("#INPUT_SECURITY_CODE").val(),
			},
			success:function(jso)
			{
				$("#ReplicaTable input[type='radio']:checked").prop('checked', false);
			}
		});
	}
	
	/* Debug Delete Replica */
	function DebugDeleteReplica(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	  :'DebugDeleteReplica',
				 'REPL_UUID' 	  :$("#ReplicaTable input[type='radio']:checked").val(),
				 'SECURITY_CODE'  :$("#INPUT_SECURITY_CODE").val(),
			},
			success:function(jso)
			{
				$("#ReplicaTable input[type='radio']:checked").prop('checked', false);
			},
		});		
	}
		
	/* Submit Trigger */
	$(function(){
		$("#AddNewReplica").click(function(){
			window.location.href = "SelectRegisteredHost";
		})
		
		$("#SyncReplica").click(function(){
			BootstrapDialog.configDefaultOptions({
				cssClass: 'funkyradio'
			}),
			BootstrapDialog.show
			({
				title: '<?php echo _("Select Replication Mode"); ?>',
				message: RadioBoxHTML,
				draggable: true,
				buttons: 
				[
					{
						label: "<?php echo _("Cancel"); ?>",
						action: function(dialogRef)
						{
							dialogRef.close();
						}
					},	
					{
						label: '<?php echo _("Submit"); ?>',
						cssClass: 'btn-primary',
						action: function(dialogRef)
						{
							$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
							SyncType = $('input[name=SelectSyncType]:checked').val();
							
							if ($("#DebugSnapshot").length > 0) /*on::off*/
							{
								if ($('#DebugSnapshot:checkbox:checked').length > 0)
								{
									DebugSnapshot = 'on';
								}
								else
								{
									DebugSnapshot = 'off';
								}
							}
							else
							{
								DebugSnapshot = 'off';
							}
							SyncSelectReplicaAsync();
							dialogRef.close();
						}
					}							
				]
			});
		})
		
		$("#DeleteReplica").click(function(){
			BootstrapDialog.show
			({
				message: '<?php echo _('Do you want to delete the selected replication process?'); ?>',
				draggable: true,
				buttons:
				[
					{
						label: '<?php echo _('Cancel'); ?>',
						action: function(dialogItself)
						{
							dialogItself.close();
						}
					},
					{
						label: '<?php echo _('Submit'); ?>',
						cssClass: 'btn-primary',
						action: function(dialogItself)
						{
							$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
							DeleteSelectReplicaAsync();
							dialogItself.close();
						}
					}					
				]
			});			
		})
		
		$("#EditReplica").click(function(){
			EditSelectReplica();
		})
		
		$("#RepairReplica").click(function(){
			QueryRepairTransport();
		})
				
		$("#DebugDelete").click(function(){
			BootstrapDialog.show({
				title: "<?php echo _('Force to clean up Replication?'); ?>",
				message: '<div class="form-group"><?php echo _("Confirm the security code to fource delete Replication from Database"); ?></div>',
				draggable: true,
				cssClass: 'danger',
				buttons: 
				[
					{
						label: '<?php echo _('Submit'); ?>',
						cssClass: 'btn-danger',
						action: function(dialogRef)
						{
							/* Check does input security code */
							if ($("#INPUT_SECURITY_CODE").val() == '')
							{
								BootstrapDialog.show({
									title: 'Message',
									draggable: true,
									cssClass: 'default-dialog danger',
									message: '<?php echo _('Please input the security code. '); ?>'
								});
							}
							else
							{
								DebugDeleteReplica();
								dialogRef.close();
							}
						}
					}
				],
				onshow: function(dialogRef) {
							dialogRef.getModalFooter()
								.addClass('form-inline')
								.find('.bootstrap-dialog-footer-buttons')
								.prepend('<input type="text" id="INPUT_SECURITY_CODE" class="form-control" style="width:115px; border: 1px solid #d9534f;" value="" placeholder="<?php echo _('Security Code'); ?>">&nbsp;');
				},
			});
		})		
	});
});
</script>
<style>
	.form-control {
		width:100%;
		display: block;
	}
	
	.form-control-inline{
		height:28px;
		width:120px;		
		display:inline;
	}
</style>
<div id='container'>
	<div id='wrapper_block'>	
		<div id='form_block'>
			<div class="page">
				<div id='title_block'>
					<div id="title_h1">
						<div class="btn-toolbar">
							<i class="fa fa-files-o fa-fw"></i>&nbsp;<?php echo _("Replication"); ?>
							<button id='AddNewReplica' 	class='btn btn-default pull-right' disabled><?php echo _("New"); ?></button>
							<button id='EditReplica' 	class='btn btn-default pull-right' disabled><?php echo _("Edit"); ?></button>
							<button id='DeleteReplica' 	class='btn btn-default pull-right' disabled><?php echo _("Delete"); ?></button>
							<button id='SyncReplica' 	class='btn btn-default pull-right' disabled><?php echo _("Sync"); ?></button>
							<button id='RepairReplica' 	class='btn btn-default pull-right debug_hidden' disabled><?php echo _("Repair"); ?></button>
							<button id='DebugDelete' 	class='btn btn-default pull-right debug_hidden' disabled><?php echo _("Force Delete"); ?></button>
						</div>
					</div>										
				</div>
				
				<div id='form_block'>
					<table id="ReplicaTable">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="148px"><?php echo _("Host Name"); ?></th>
							<th width="440px"><?php echo _("Status"); ?></th>
							<th width="112px"><?php echo _("Progress"); ?></th>							
							<th width="155px"><?php echo _("Time"); ?></th>
							<th width="55px" class="TextCenter"><?php echo _("Diag."); ?></th>
							<th width="60px" class="TextCenter"><?php echo _("Details"); ?></th>							
						</tr>
						</thead>
					
						<tbody>
							<tr class="default">								
								<td colspan="7" class="padding_left_30"><?php echo _("Checking management connection..."); ?></td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
		</div> <!-- id: form_block-->		
	</div> <!-- id: wrapper_block-->
</div>
