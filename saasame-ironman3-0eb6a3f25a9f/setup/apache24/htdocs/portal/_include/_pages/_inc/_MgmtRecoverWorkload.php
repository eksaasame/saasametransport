<script>
$( document ).ready(function() {
	/* Begin to Automatic Exec */
	UnsetSession();
	ListAvailableService();
	DebugOptionLevel();
	
	/* SESSION UNSET*/
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
		
	/* Show Debug Option */
	function DebugOptionLevel(){
		option_level = '<?php echo $_SESSION['optionlevel']; ?>';
		if (option_level == 'DEBUG')
		{
			$('.debug_hidden').show();
		}
	}	
	
	/* Define Recover DataTable */
	function RecoverDataTable(count){
		if (count > 10)
		{
			$status = true;
		}
		else
		{
			$status = false;
		}		
		
		RecoverTable = $('#ServiceTable').DataTable(
		{
			paging		 : $status,
			bInfo		 : $status,
			bFilter		 : $status,
			lengthChange : false,
			pageLength	 : 10,
			bAutoWidth	 :false,
			pagingType	 : "simple_numbers",
			order: [],
			columnDefs   : [ {
				targets: 'TextCenter',
				orderable: false,
				targets: [0,5]				
			}],
			ordering	 : $status,			
			language	: {
				search: '_INPUT_'
			},
			stateSave	: true
		});
		
		$('div.dataTables_filter input').attr("placeholder",$.parseHTML("&#xF002; Search")[0].data);
		$('div.dataTables_filter input').addClass("form-control input-sm");
		$('div.dataTables_filter input').css("max-width", "180px");
		$('div.dataTables_filter input').css("padding-right", "80px");						
	}
		
	/* List Service */
	function ListAvailableService(){
		Pace.ignore(function(){
			$.ajax({
				type: 'POST',
				dataType:'JSON',
				url: '_include/_exec/mgmt_service.php',
				data:{
					 'ACTION'    :'ListAvailableService',
					 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
				},
				success:function(jso)
				{
					$('.default').remove();
					
					if (jso.Code != false)
					{
						$('#AddNewService').prop('disabled', false);
						$('#AddNewService').removeClass('btn-default').addClass('btn-primary');
						
						if (jso != false)
						{	
							/* Define Service DataTable */
							RecoverDataTable(Object.keys(jso).length);
					
							/* Clear Service Table Data */
							RecoverTable.clear();						
					
							$.each(jso, function(key,value)
							{
								RecoverTable.row.add(
								[
									'<div class="TextCenter"><input type="radio" id="SERV_UUID" name="SERV_UUID" value="'+this.SERV_UUID+'" /><div>',
									'<div class="ServiceHostOverFlow">'+this.HOST_NAME+'</div>',
									'<div class="ServiceTextOverFlow">'+this.SERV_MESG+'</div><div id="TextOverHistory" class="TextOverHistory" data-repl="'+this.SERV_UUID+'"><i class="fa fa-commenting-o"></i></div>',								
									this.SERV_TIME,
									'<div class="TextCenter TopologyLink"><div id="NetworkTopology" replica-detail="'+this.SERV_UUID+'" style="cursor:pointer;"><i class="fa fa-random fa-lg"></i></div></div>',
									'<div class="TextCenter NoSeries"><div id="ServiceInfo" data-service="'+this.SERV_UUID+'" style="cursor:pointer;"><i class="fa fa-sliders fa-lg"></i></div></div>'
								]).draw(false).columns.adjust();
							});
							
							$("#ServiceTable").on("click", "#SERV_UUID", function(e){
								var SERV_UUID = $(this).attr('value'); 
								OnClickSelectService(SERV_UUID);
							});
							
							$("#ServiceTable").on("click", "#TextOverHistory", function(e){
								$('#TextOverHistory').prop('disabled', true);
								var SERV_UUID = $(this).attr('data-repl');
								DisplayJobHistory(SERV_UUID);   
							});
							
							$("#ServiceTable").on("click", "#NetworkTopology", function(e){
								$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
								$('#ServiceInfo').prop('disabled', true);
								var SERV_UUID = $(this).attr('replica-detail');
								DisplayNetworkTopology(SERV_UUID);
							});
							
							$("#ServiceTable").on("click", "#ServiceInfo", function(e){
								$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
								$('#ServiceInfo').prop('disabled', true);
								var SERV_UUID = $(this).attr('data-service');
								DisplayRecoveryJobInfo(SERV_UUID); 
							});
							
							if (Object.keys(jso).length < 20)
							{
								SetSeconds = 2000;
							}
							else
							{
								SetSeconds = 5000;
							}
						
							Interval = setInterval(IntervalUpdateService, SetSeconds);
							
							$(window).on("blur focus", function(e)
							{
								var prevType = $(this).data("prevType");
								if (prevType != e.type)
								{
									switch (e.type) {
										case "blur":
											clearInterval(Interval);
											Interval = setInterval(IntervalUpdateService, 5000);
										break;									
										case "focus":
											clearInterval(Interval);
											Interval = setInterval(IntervalUpdateService, SetSeconds);
										break;
									}
								}
								$(this).data("prevType", e.type);
							})
						}
						else
						{
							$('#ServiceTable > tbody').append('<tr><td colspan="6" class="padding_left_30"><?php echo _("Click New to start a new recovery process."); ?></td></tr>');					
						}
					}
					else
					{
						$('#ServiceTable > tbody').append('<tr><td colspan="6" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
					}
				},
				error: function(xhr)
				{
					
				}
			});
		});
	}
	
	/* OnClick Select Service */
	function OnClickSelectService(SERV_UUID){
		Pace.ignore(function(){
			$.ajax({
				type: 'POST',
				dataType:'JSON',
				url: '_include/_exec/mgmt_service.php',
				data:{
					 'ACTION'   :'QueryServiceInformation',
					 'JOB_UUID' :SERV_UUID
				},
				success:function(jso)
				{
					JOB_INFO = JSON.parse(jso.JOBS_JSON);
					
					MarkDelete	 = JOB_INFO['mark_delete'];
					if (MarkDelete == false)
					{
						CloudType	 = jso.CLOUD_TYPE;
						Recovery_Kit = JOB_INFO['winpe_job'];
						Recovery_Type = JOB_INFO['recovery_type'];
						
						$('#RecoveryAction').prop('disabled', false);
						$('#RecoveryAction').removeClass('btn-default').addClass('btn-primary');
						
						$('#DebugDelete').prop('disabled', false);
						$('#DebugDelete').removeClass('btn-default').addClass('btn-danger');
					
						RadioBoxHTML = $('<div style="height:190px;">');
						
						switch (Recovery_Type) 
						{
							case 'DevelopmentTesting':
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="DeleteInstance" value="DeleteInstance" checked /><label for="DeleteInstance"><?php echo _("Remove Process"); ?></label><div class="indentedtext"><?php echo _("Launched replica for dev-test environment will be deleted and this recovery process will be removed. You can create a new dev-test environment by creating a new recovery process."); ?></div></div>');					
								RadioBoxHTML.append('<div style="height:5px;"></div>');
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="KeepInstance" value="KeepInstance"/><label for="KeepInstance"><?php echo _("Keep Instance"); ?></label><div class="indentedtext"><?php echo _("Launched replica for dev-test environment will remain on target. Only this recovery process will be removed."); ?></div></div>');
							break;
							
							case 'DisasterRecovery':
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="DeleteInstance" value="DeleteInstance" checked /><label for="DeleteInstance"><?php echo _("Remove Process"); ?></label><div class="indentedtext"><?php echo _("Recovered replica will be deleted and this recovery process will be removed. You can create a new replica by creating a new recovery process."); ?></div></div>');					
								RadioBoxHTML.append('<div style="height:5px;"></div>');
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="KeepInstance" value="KeepInstance"/><label for="KeepInstance"><?php echo _("Complete Process"); ?></label><div class="indentedtext"><?php echo _("Recovered replica will remain on target. Only this recovery process will be removed."); ?></div></div>');
							break;
							
							case 'PlannedMigration':
								RadioBoxHTML.append('<div class="funkyradio-primary" style="height:88px;"><input type="radio" name="SelectedAction" id="DeleteInstance" value="DeleteInstance" checked/><label for="DeleteInstance"><?php echo _("Remove Process"); ?></label><div class="indentedtext"><?php echo _("Recovered replica will remain on target. Only this recovery process will be removed."); ?></div></div>');
								RadioBoxHTML.append('<div style="height:5px;"></div>');
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="KeepInstance" value="KeepInstance" /><label for="KeepInstance"><?php echo _("Complete Process"); ?></label><div class="indentedtext"><?php echo _("Migration completed. Prepare Workload and Recover Workload jobs will be cleaned up."); ?></div></div>');
								if (CloudType != 'Aliyun')
								{
									RadioBoxHTML.append('<span class="funkyradio-warning" style="width:200px; position:relative; top:-68px; left:375px;"><input type="checkbox" id="CleanUpSnapshot" disabled /><label for="CleanUpSnapshot" style="border-style:none; height:36px;"><?php echo _("Clean Up Snapshot(s)"); ?></label></span>');
									RadioBoxHTML.append("<script>$(document).ready(function() {$('input[type=radio][name=SelectedAction]').change(function(){if(this.id == 'DeleteInstance'){$('#CleanUpSnapshot').attr('disabled', true);$('#CleanUpSnapshot').prop('checked', false);}else{$('#CleanUpSnapshot').attr('disabled', false); if (this.id == 'CleanUpSnapshot'){$('#KeepInstance').prop('checked', true);}}});});<\/script>");				
								}
							break;
							
							case 'RecoveryKit':
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="DeleteInstance" value="DeleteInstance" checked /><label for="DeleteInstance"><?php echo _("Remove Process"); ?></label><div class="indentedtext"><?php echo _("Recover Workload process will be deleted. You can run conversion again by creating a new Recover Workload process."); ?></div></div>');					
								RadioBoxHTML.append('<div style="height:5px;"></div>');
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="KeepInstance" value="KeepInstance"/><label for="KeepInstance"><?php echo _("Complete Process"); ?></label><div class="indentedtext"><?php echo _("Conversion completed and system running in production mode. Prepare Workload and Recover Workload jobs will be cleaned up."); ?></div></div>');	
							break;
							
							case 'ExportImage':
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="DeleteInstance" value="DeleteInstance" checked /><label for="DeleteInstance"><?php echo _("Remove Process"); ?></label><div class="indentedtext"><?php echo _("Recover Workload process will be deleted. You can run conversion again by creating a new Recover Workload process."); ?></div></div>');					
								RadioBoxHTML.append('<div style="height:5px;"></div>');
								RadioBoxHTML.append('<div class="funkyradio-primary"><input type="radio" name="SelectedAction" id="KeepInstance" value="KeepInstance"/><label for="KeepInstance"><?php echo _("Complete Process"); ?></label><div class="indentedtext"><?php echo _("Conversion completed and system running in production mode. Prepare Workload and Recover Workload jobs will be cleaned up."); ?></div></div>');	
							break;
						}
						
						RadioBoxHTML.append('</div>');
					}					
				},
				error: function(xhr)
				{
					
				}
			});
		});
	}
	
	/* Interval Update Service */
	function IntervalUpdateService(){
		Pace.ignore(function(){
			$.ajax({
				type: 'POST',
				dataType:'JSON',
				url: '_include/_exec/mgmt_service.php',
				data:{
					 'ACTION'    :'ListAvailableService',
					 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
				},
				success:function(jso)
				{
					if (jso != false)
					{
						/* Clear Service Table Data */
						RecoverTable.clear();
						
						/* GET LAST CHECK UUID */
						CheckedUUID = $("#ServiceTable input[type='radio']:checked").val();
											
						$.each(jso, function(key,value)
						{
							if (CheckedUUID == this.SERV_UUID)
							{
								RadioChecked = ' checked';
							}
							else
							{
								RadioChecked = '';
							}
							
							RecoverTable.row.add(
							[
								'<div class="TextCenter"><input type="radio" id="SERV_UUID" name="SERV_UUID" value="'+this.SERV_UUID+'"'+RadioChecked+'/><div>',
								'<div class="ServiceHostOverFlow">'+this.HOST_NAME+'</div>',
								'<div class="ServiceTextOverFlow">'+this.SERV_MESG+'</div><div id="TextOverHistory" class="TextOverHistory" data-repl="'+this.SERV_UUID+'"><i class="fa fa-commenting-o"></i></div>',							
								this.SERV_TIME,
								'<div class="TextCenter TopologyLink"><div id="NetworkTopology" replica-detail="'+this.SERV_UUID+'" style="cursor:pointer;"><i class="fa fa-random fa-lg"></i></div></div>',
								'<div class="TextCenter NoSeries"><div id="ServiceInfo" data-service="'+this.SERV_UUID+'" style="cursor:pointer;"><i class="fa fa-sliders fa-lg"></i></div></div>'
							]).draw(false);						
						});											
					}
					else
					{
						clearInterval(Interval);
						
						$('#RecoveryAction').prop('disabled', true);
						$('#RecoveryAction').removeClass('btn-primary').addClass('btn-default');
						
						$('#DebugDelete').prop('disabled', true);
						$('#DebugDelete').removeClass('btn-danger').addClass('btn-default');
						
						RecoverTable.clear();
						RecoverTable = $('#ServiceTable').DataTable( {
							destroy		 : true,
							paging		 : false,
							bInfo		 : false,
							bFilter		 : false,												
							bAutoWidth	 : false,						
							ordering	 : false,			
							stateSave	 : false,
							language	 : {
								sEmptyTable:'<div class="padding_left_30"><?php echo _("Click New to start a new recovery process."); ?></div>'
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
	function DisplayJobHistory(SERV_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'     :'DisplayJobHistory',
				 'JOBS_UUID'  :SERV_UUID,
				 'ITEM_LIMIT' :250
			},
			success:function(msg)
			{
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: "<?php echo _('History'); ?>",
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
	
	/* Display Recovery Job Information */
	function DisplayRecoveryJobInfo(SERV_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'     :'DisplayRecoveryJobInfo',
				 'JOBS_UUID'  :SERV_UUID,
			},
			success:function(msg)
			{
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: "<?php echo _('Recovery Information'); ?>",
					cssClass: 'workload-dialog',
					message: msg,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);
				
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#ServiceInfo').prop('disabled', false);
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			}
		});		
	}
	
	/* Display Network Topology */
	function DisplayNetworkTopology(SERV_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'NetworkTopology',
				 'JOB_UUID'  :SERV_UUID,
				 'TYPE'		 :'Recovery'
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
				$('#ServiceInfo').prop('disabled', false);
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			}
		});		
	}
	
	/* Delete Select Service Async */
	function DeleteSelectRecoverAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	  :'DeleteSelectRecoverAsync',
				 'SERV_UUID' 	  :$("#ServiceTable input[type='radio']:checked").val(),				 
				 'INSTANCE_ACTION':InstanceAction,
				 'DELETE_SNAPSHOT':CleanUpSnapshot
			},
			success:function(jso)
			{
				$('#RecoveryAction').prop('disabled', true);				
				$('#RecoveryAction').removeClass('btn-primary').addClass('btn-default');				
				
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _('Service Message'); ?>",
					message: jso.msg,
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						$("#ServiceTable input[type='radio']:checked").prop('checked', false);
						//window.location.href = "MgmtRecoverWorkload";
					},
				});
			},
		});		
	}
	
	/* Debug Delete Service */
	function DebugDeleteService(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	  :'DebugDeleteService',
				 'SERV_UUID' 	  :$("#ServiceTable input[type='radio']:checked").val(),
				 'SECURITY_CODE'  :$("#INPUT_SECURITY_CODE").val(),
			},
			success:function(jso)
			{
				
			},
		});		
	}

	/* Submit Trigger */
	$(function(){
		$("#AddNewService").click(function(){
			window.location.href = "SelectRecoverHost";
		})
		
		$("#RecoveryAction").click(function(){
			BootstrapDialog.configDefaultOptions({
				cssClass: 'funkyradio'
			}),
			BootstrapDialog.show
			({
				title: '<?php echo _("Do you want to delete the selected recovery process?"); ?>',
				message: RadioBoxHTML,
				draggable: true,				
				buttons: 
				[
					{
						label: '<?php echo _("Cancel"); ?>',
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
							InstanceAction = $('input[name=SelectedAction]:checked').val(); /*DeleteInstance::KeepInstance*/
							
							if ($("#CleanUpSnapshot").length > 0) /*on::off*/
							{
								if ($('#CleanUpSnapshot:checkbox:checked').length > 0)
								{
									CleanUpSnapshot = 'on';
								}
								else
								{
									CleanUpSnapshot = 'off';
								}
							}
							else
							{
								CleanUpSnapshot = 'off';
							}
							
							DeleteSelectRecoverAsync();
							dialogRef.close();
						}
					}
				]
			});
		})

		$("#DebugDelete").click(function(){
			BootstrapDialog.show({
				title: '<?php echo _("Force to clean up Recovery?"); ?>',
				message: '<div class="form-group"><?php echo _("Confirm the security code to fource delete Recovery from Database:"); ?>&nbsp;&nbsp;<input type="text" id="INPUT_SECURITY_CODE" class="form-control form-control-inline" value="" placeholder="<?php echo _('Security Code'); ?>"></div>',
				draggable: true,
				buttons: 
				[
					{
						label: '<?php echo _("Cancel"); ?>',
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
							DebugDeleteService();
							dialogRef.close();
						}
					}
				]
			});
		})
	});
});
</script>

<style>
	.indentedtext{
		margin-left:0.8em;
	}
	
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
							<i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?>
							<button id='AddNewService'	class='btn btn-default pull-right' 		 		disabled><?php echo _("New"); ?></button>
							<button id='RecoveryAction'	class='btn btn-default pull-right' 		 		disabled><?php echo _("Action"); ?></button>
							<button id='DebugDelete' 	class='btn btn-default pull-right debug_hidden' disabled><?php echo _("Force Delete"); ?></button>		
						</div>
					</div>										
				</div>
				
				<div id='form_block'>
					<table id="ServiceTable">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="168px"><?php echo _("Host Name"); ?></th>						
							<th width="537px"><?php echo _("Status"); ?></th>
							<th width="178px"><?php echo _("Time"); ?></th>
							<th width="55px" class="TextCenter"><?php echo _("Diag."); ?></th>
							<th width="60px" class="TextCenter"><?php echo _("Details"); ?></th>
						</tr>
						</thead>
					
						<tbody>
							<tr class="default">								
								<td colspan="6" class="padding_left_30"><?php echo _("Checking management connection..."); ?></td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
		</div> <!-- id: form_block-->		
	</div> <!-- id: wrapper_block-->
</div>
