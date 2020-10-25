<?php if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }?>
<script>
	/* Determine URL Segment */
	DetermineSegment('EditAwsConnection');
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT == $SET_SEGMENT)
		{
			QueryAccessCredentialInformation();
		}
	}
	
	/* Query Access Credential Information */
	function QueryAccessCredentialInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>'	 
			},
			success:function(jso)
			{
				$('#SELECT_AWS_REGION').val(jso.DEFAULT_ADDR);
				$('#INPUT_ACCESS_KEY').val(jso.ACCESS_KEY);
				$('#INPUT_SECRET_KEY').val(jso.SECRET_KEY);
			
				$('.selectpicker').selectpicker('refresh');
				
				$('#BackToMgmt').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
	}

	/* Verify Aws Credential */
	function VerifyAwsAccessCredential(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'	 		:'VerifyAwsAccessCredential',				 
				 'AWS_REGION' 		:$("#SELECT_AWS_REGION").val(),
				 'AWS_ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'AWS_SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val()		 
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				
				if (jso.Code == true)
				{
					$('#SELECT_AWS_REGION').prop('disabled', true);
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
	
	
	/* Initialize New Aws Connection */
	function InitializeNewAwsConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'	 		:'InitializeNewAwsConnection',
				 'ACCT_UUID' 		:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 		:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'AWS_REGION' 		:$("#SELECT_AWS_REGION").val(),
				 'AWS_ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'AWS_SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val()	
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
	
	
	/* Update Aws Connection */
	function UpdateAwsConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aws.php',
			data:{
				 'ACTION'	 		:'UpdateAwsConnection',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'AWS_REGION' 		:$("#SELECT_AWS_REGION").val(),
				 'AWS_ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'AWS_SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val()	
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
			VerifyAwsAccessCredential();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if ($URL_SEGMENT == 'AddAwsConnection')
			{				
				InitializeNewAwsConnection();
			}
			else
			{
				UpdateAwsConnection();
			}
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img style="padding-bottom:2px;" src=" data:image/png;base64,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LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4KICA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIgogICAgeG1sbnM6aXB0Y0V4dD0iaHR0cDovL2lwdGMub3JnL3N0ZC9JcHRjNHhtcEV4dC8yMDA4LTAyLTI5LyIKICAgIHhtbG5zOnhtcE1NPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvbW0vIgogICAgeG1sbnM6c3RFdnQ9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZUV2ZW50IyIKICAgIHhtbG5zOnBsdXM9Imh0dHA6Ly9ucy51c2VwbHVzLm9yZy9sZGYveG1wLzEuMC8iCiAgICB4bWxuczpHSU1QPSJodHRwOi8vd3d3LmdpbXAub3JnL3htcC8iCiAgICB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iCiAgICB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iCiAgIHhtcE1NOkRvY3VtZW50SUQ9ImdpbXA6ZG9jaWQ6Z2ltcDo4ZWQ2OWVhNS03ZGJkLTRhMTYtOGU1Zi01ZThjOWFiNDEyYmMiCiAgIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MGJiZDcyNjktMTYyMy00ZDAyLTkwYTQtZDcyNjE5YzUyNmM1IgogICB4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ9InhtcC5kaWQ6ZjE3MDE0N2ItNTFiZi00NGEwLTk3NzctY2EzZWVhYmI1ZGExIgogICBHSU1QOkFQST0iMi4wIgogICBHSU1QOlBsYXRmb3JtPSJXaW5kb3dzIgogICBHSU1QOlRpbWVTdGFtcD0iMTUzNzk0MjcxMzY5MzgzOCIKICAgR0lNUDpWZXJzaW9uPSIyLjEwLjYiCiAgIGRjOkZvcm1hdD0iaW1hZ2UvcG5nIgogICB4bXA6Q3JlYXRvclRvb2w9IkdJTVAgMi4xMCI+CiAgIDxpcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgPGlwdGNFeHQ6TG9jYXRpb25TaG93bj4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uU2hvd24+CiAgIDxpcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgPGlwdGNFeHQ6UmVnaXN0cnlJZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OlJlZ2lzdHJ5SWQ+CiAgIDx4bXBNTTpIaXN0b3J5PgogICAgPHJkZjpTZXE+CiAgICAgPHJkZjpsaQogICAgICBzdEV2dDphY3Rpb249InNhdmVkIgogICAgICBzdEV2dDpjaGFuZ2VkPSIvIgogICAgICBzdEV2dDppbnN0YW5jZUlEPSJ4bXAuaWlkOmE3NjFlNjM5LWNjYTEtNDNjNi05OWEzLTRiNjJkNWVkMjE1MiIKICAgICAgc3RFdnQ6c29mdHdhcmVBZ2VudD0iR2ltcCAyLjEwIChXaW5kb3dzKSIKICAgICAgc3RFdnQ6d2hlbj0iMjAxOC0wOS0yNlQxNDoxODozMyIvPgogICAgPC9yZGY6U2VxPgogICA8L3htcE1NOkhpc3Rvcnk+CiAgIDxwbHVzOkltYWdlU3VwcGxpZXI+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpJbWFnZVN1cHBsaWVyPgogICA8cGx1czpJbWFnZUNyZWF0b3I+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpJbWFnZUNyZWF0b3I+CiAgIDxwbHVzOkNvcHlyaWdodE93bmVyPgogICAgPHJkZjpTZXEvPgogICA8L3BsdXM6Q29weXJpZ2h0T3duZXI+CiAgIDxwbHVzOkxpY2Vuc29yPgogICAgPHJkZjpTZXEvPgogICA8L3BsdXM6TGljZW5zb3I+CiAgPC9yZGY6RGVzY3JpcHRpb24+CiA8L3JkZjpSREY+CjwveDp4bXBtZXRhPgogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg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">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			<div class="form-group form-inline">
				<label for='comment' style="width:10%">&nbsp;<?php echo _("Endpoint"); ?></label>
				<label for='comment' style="width:44.5%">&nbsp;<?php echo _("Access Key"); ?></label>
				<label for='comment' style="width:44.5%">&nbsp;<?php echo _("Secret Key"); ?></label>
				<select id="SELECT_AWS_REGION" class="selectpicker" data-width="10%">
					<option value="us-east-1"><?php echo _("Global"); ?></option>
					<option value="cn-north-1"><?php echo _("China"); ?></option>
				</select>				
				<input type='text' id='INPUT_ACCESS_KEY' class='form-control' value='' placeholder='Access Key' style="width:44.5%">	
				<input type='text' id='INPUT_SECRET_KEY' class='form-control' value='' placeholder='Secret Key' style="width:44.5%">	
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
