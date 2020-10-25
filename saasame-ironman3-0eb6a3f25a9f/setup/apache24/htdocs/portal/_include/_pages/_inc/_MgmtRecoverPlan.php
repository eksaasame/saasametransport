<script>
	UnsetSession();
	ListAvailableRecoverPlan();
	
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
	
	/* Define Recover Plan DataTable */
	function RecoverPlanDataTable(count){
		
		/* Disable Error Message */
		$.fn.dataTable.ext.errMode = 'none';

		if (count > 10)
		{
			$status = true;
		}
		else
		{
			$status = false;
		}		
		
		RecoverPlanTable = $('#RecoverPlanTable').DataTable(
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
				targets: [0,4]				
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
	
	
	/* List Available Recover Plan */
	function ListAvailableRecoverPlan(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'ListAvailableRecoverPlan',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
			},
			success:function(jso)
			{
				$('.default').remove();
								
				if (jso != false)
				{
					// Define Service DataTable
					RecoverPlanDataTable(Object.keys(jso).length);
					
					// Clear Service Table Data
					RecoverPlanTable.clear();
				
					$.each(jso, function(key,value)
					{
						switch (this.CLOUD_TYPE ) 
						{
							case 'OPENSTACK':
								if (this.VENDOR_NAME != 'OPENSTACK')
								{
									SERV_LOCATION = this.VENDOR_NAME;
								}
								else
								{
									SERV_LOCATION = 'OpenStack';
								}
							break
								
							case 'AWS':
								SERV_LOCATION = 'AWS';
							break
								
							case 'Azure':
								SERV_LOCATION = 'Azure';
							break
								
							case 'Aliyun':
								SERV_LOCATION = 'Alibaba Cloud';
							break
							
							case 'Ctyun':
								SERV_LOCATION = '天翼云';
							break
							
							default:
								SERV_LOCATION = 'Cloud';
						}
						
						switch (this.RECOVER_TYPE) 
						{
							case 'RECOVERY_PM':
								SERV_TYPE = '<?php echo _("Planned Migration"); ?>';
							break
								
							case 'RECOVERY_DR':
								SERV_TYPE = '<?php echo _("Disaster Recovery"); ?>';
							break
							
							case 'RECOVERY_DT':
								SERV_TYPE = '<?php echo _("Development Testing"); ?>';
							break
								
							default:
								SERV_TYPE = '<?php echo _("Development Testing"); ?>';
						}

						RecoverPlanTable.row.add(
						[
							'<div class="TextCenter"><input type="radio" id="PLAN_UUID" name="PLAN_UUID" value="'+this.PLAN_UUID+'" /><div>',
							'<div class="ServiceHostOverFlow">'+this.HOST_NAME+'</div>',
							SERV_LOCATION,
							SERV_TYPE,
							this.TIMESTAMP,
							'<div class="TextCenter"><div id="TemplateInfo" data-service="'+this.PLAN_UUID+'" style="cursor:pointer;"><i class="fa fa-sliders fa-lg"></i></div></div>'
						]).draw(false).columns.adjust();
					});
				
					$("#RecoverPlanTable").on("click", "#PLAN_UUID", function(e){
						OnClickSelectTemplate();
					});
					
					$("#RecoverPlanTable").on("click", "#TemplateInfo", function(e){
						$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
						$('#TemplateInfo').prop('disabled', true);
						var PLAN_UUID = $(this).attr('data-service');
						DisplayRecoveryPlanInfo(PLAN_UUID); 
					});
				}
				else
				{
					$('#EditServicePlan').prop('disabled', true);
					$('#EditServicePlan').removeClass('btn-primary').addClass('btn-default');
		
					$('#DeleteServicePlan').prop('disabled', true);
					$('#DeleteServicePlan').removeClass('btn-primary').addClass('btn-default');					
					
					$('#RecoverPlanTable > tbody').empty();
					$('#RecoverPlanTable > tbody').append('<tr><td colspan="6" class="padding_left_30"><?php echo _("Click New to create a recovery plan."); ?></td></tr>');						
				}				
			},
			error: function(xhr)
			{
			
			}
		});
	}

	
	/* OnClick Select Template */
	function OnClickSelectTemplate()
	{
		$('#EditServicePlan').prop('disabled', false);
		$('#EditServicePlan').removeClass('btn-default').addClass('btn-primary');
		
		$('#DeleteServicePlan').prop('disabled', false);
		$('#DeleteServicePlan').removeClass('btn-default').addClass('btn-primary');
	}

	/* Delete Recover Plan */
	function DeleteRecoverPlan()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'  	 :'DeleteRecoverPlan',
				 'PLAN_UUID' :$("#RecoverPlanTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				RecoverPlanTable.clear();
				ListAvailableRecoverPlan();
			},
			error: function(xhr)
			{
				
			}
		});	
	}	
	
	/* Display RecoveryPlan Configuration */
	function DisplayRecoveryPlanInfo(PLAN_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryRecoverPlan',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'PLAN_UUID' :PLAN_UUID
			},
			success:function(msg)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#TemplateInfo').prop('disabled', false);
				
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: '<?php echo _("Information"); ?>',
					cssClass: 'workload-dialog',
					message: msg,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);
			},
			error: function(xhr)
			{
				
			}
		});
	}

	/* Select Edit Plan */
	function EditRecoverPlan(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'EditRecoverPlan', 
				'PLAN_UUID' :$("#RecoverPlanTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso != false)
				{
					window.location.href = "EditPlanSelectRecoverType"
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
		$("#AddServicePlan").click(function(){
			window.location.href = "PlanSelectRecoverReplica";
		})
		
		$("#DeleteServicePlan").click(function(){
		
			BootstrapDialog.confirm("<?php echo _("Do you want to delete the selected recovery plan?"); ?>", function(result){
				if (result == true)
				{
					$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
					DeleteRecoverPlan();
				}
			});	
		})
		
		$("#EditServicePlan").click(function(){
			EditRecoverPlan();
		})
	});
</script>
<div id='container'>
	<div id='wrapper_block'>	
		<div id='form_block'>
			<div class="page">
				<div id='title_block'>
					<div id="title_h1">
						<div class="btn-toolbar">
							<i class="fa fa-clone fa-fw"></i>&nbsp;<?php echo _("Recovery Plan"); ?>
							<button id='AddServicePlan'  	class='btn btn-primary pull-right'><?php echo _("New"); ?></button>
							<button id='DeleteServicePlan' 	class='btn btn-default pull-right' disabled><?php echo _("Delete"); ?></button>		
							<button id='EditServicePlan' 	class='btn btn-default pull-right' disabled><?php echo _("Edit"); ?></button>											
						</div>
					</div>										
				</div>
				
				<div id='form_block'>
					<table id="RecoverPlanTable">
						<thead>
						<tr>
							<th width="50px" class="TextCenter">#</th>
							<th width="220px"><?php echo _("Host Name"); ?></th>						
							<th width="220px"><?php echo _("Cloud"); ?></th>
							<th width="220px"><?php echo _("Type"); ?></th>
							<th width="220px"><?php echo _("Time"); ?></th>
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