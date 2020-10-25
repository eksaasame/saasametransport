<?php if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }?>
<script>
	/* Determine URL Segment */
	DetermineSegment('EditCtyunConnection');
	function DetermineSegment(SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();
		SELECT_REGION = '';
		if (URL_SEGMENT == SET_SEGMENT)
		{
			QueryAccessCredentialInformation();
		}
	}
	
	/* Query Access Credential Information */
	function QueryAccessCredentialInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>'	 
			},
			success:function(jso)
			{
				$('#INPUT_ACCESS_KEY').val(jso.ACCESS_KEY);
				$('#INPUT_SECRET_KEY').val(jso.SECRET_KEY);
				
				PROJECT_NAME = jso.PROJECT_NAME;
				SELECT_REGION = PROJECT_NAME+'|'+jQuery.parseJSON(jso.USER_UUID).project_region;
			
				$('#SELECT_CLOUD_REGION').append(new Option(PROJECT_NAME,PROJECT_NAME, true, true));
				$("#SELECT_CLOUD_REGION").val(PROJECT_NAME);
				$('.selectpicker').selectpicker('refresh');	
				
				$('#BackToMgmt').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
	}

	/* Verify Ctyun Credential */
	function VerifyCtyunAccessCredential(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'	 		:'VerifyCtyunAccessCredential',
				 'CTYUN_ACCESS_KEY'	:$("#INPUT_ACCESS_KEY").val(),
				 'CTYUN_SECRET_KEY'	:$("#INPUT_SECRET_KEY").val()		 
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				
				if (jso.Code == true)
				{
					$('#SELECT_CLOUD_REGION').prop('disabled', false);	
					$('#SELECT_CLOUD_REGION').empty();
					$.each(jso.Regions, function(key,value)
					{
						$('#SELECT_CLOUD_REGION').append(new Option(value.zoneName,value.zoneName+'|'+value.regionId, true, true));
					});
					$("#SELECT_CLOUD_REGION").val(SELECT_REGION);
					$('.selectpicker').selectpicker('refresh');					
					
					$('#INPUT_ACCESS_KEY').prop('disabled', true);
					$('#INPUT_SECRET_KEY').prop('disabled', true);
				
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifyConnection').prop('disabled',false);
					$('#VerifyConnection').removeClass('btn-default').addClass('btn-primary');
				
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#CheckAndSubmit').prop('disabled', true);
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			},
		});
	}
	
	
	/* Initialize New Ctyun Connection */
	function InitializeNewCtyunConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'	 			:'InitializeNewCtyunConnection',
				 'ACCT_UUID' 			:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 			:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'CTYUN_ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'CTYUN_SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val(),
				 'CTYUN_SELECT_REGION'	:$("#SELECT_CLOUD_REGION").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.type_info,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtCloudConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.type_danger,
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
    			}
			},
			error: function(xhr)
			{
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	    
	}
	
	
	/* Update Ctyun Connection */
	function UpdateCtyunConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_ctyun.php',
			data:{
				 'ACTION'	 			:'UpdateCtyunConnection',
				 'CLUSTER_UUID'			:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'CTYUN_ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'CTYUN_SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val(),
				 'CTYUN_SELECT_REGION'	:$("#SELECT_CLOUD_REGION").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.type_info,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtCloudConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.type_danger,
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
    			}
			},
			error: function(xhr)
			{
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	
	}
	
	/* Submit Trigger */
	$(function(){
		$("#BackToMgmt").click(function(){
			window.location.href = "SelectCloudRegistration";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtCloudConnection";
		})
		
		$("#VerifyConnection").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#VerifyConnection').prop('disabled', true);
			$('#VerifyConnection').removeClass('btn-primary').addClass('btn-default');
			VerifyCtyunAccessCredential();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if (URL_SEGMENT == 'AddCtyunConnection')
			{				
				InitializeNewCtyunConnection();
			}
			else
			{
				UpdateCtyunConnection();
			}
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img src=" data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACIAAAAVCAYAAAAjODzXAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4gcFByQmNAxO1QAAAfhJREFUSMe91k2IjlEUB/CfIQpjfMwCpVgQFpKU7DSmKIWNhWSBoaysbGSSZkWWFlZW7JTCxFKU7KYsDLMwyJg3k5gZXzOaeW3OW7enZ57348Gp2/vce88593/O+d9zX5qTVTiGflQwi1HcRRcW+Q+yH19QLRij6PyXIE7nHDqLHznrFWzHXnRj898CsR4zyUGD2JrsL8C5Opl6gnllgXxKHJ4v0FuNsQIwA80c2oFTuJfDh0cN2G8L3QkM4Q2mEh8HGgHRg88FEe1sMJjLWJPMNyU+HmJfkfGRnIM/4l1SmjLSl/E9hoNZpXU57O/GUixBOw6hrQSQo3Nk+UFNYX40qMMxr2IHnmEav+P3dRNZOYnx4FhNVkQgr7A45tUo2wSew+ME4Y2SJbia+NpYoHclkxnwPVloLwGiN+N8sg4xU9AXROqrJQnZW3Dbds1hsyG6cxXDbXhfshzL443pwUhm71a0+LzHcDj5Xgh3EvR9JUENZLJxqU7zrGVkRFzNmuF4MLpVuZgB8rbgjelK9IZgJX4miy+DZB0tAOnMAJnBiRy9LdEsa3pn8tC1Or7GrbuWs/cUx3EW95OSVPGrkTbf7LgZvipN2Mz5EPZHdJP41uSYwvXwM1gHwHTwkwIiLSvR3NrwIf4w7YmWvxtrI7gXuB0Bj9aM/gBPRwyFBAAxTQAAAABJRU5ErkJggg==">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			<label for='comment'><?php echo _("Access Key"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_ACCESS_KEY' class='form-control' value='' placeholder='公钥'>	
			</div>
			
			<label for='comment'><?php echo _("Secret Key"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_SECRET_KEY' class='form-control' value='' placeholder='密钥'>	
			</div>
			
			<label for='comment'><?php echo _("Region"); ?></label>
			<div class='form-group has-primary'>	
				<select id='SELECT_CLOUD_REGION' class='selectpicker' data-width="100%" disabled></select>			
			</div>
		</div>
					
		<div id='title_block_wizard'>
			<div class='btn-toolbar'>
				<button id='BackToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Back"); ?></button>
				<button id='CancelToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Cancel"); ?></button>
				<button id='CheckAndSubmit' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
				<button id='VerifyConnection' 	class='btn btn-primary pull-right btn-lg'><?php echo _("Verify Connection"); ?></button>
			</div>
		</div>		
	</div> <!-- id: wrapper_block-->
</div>
