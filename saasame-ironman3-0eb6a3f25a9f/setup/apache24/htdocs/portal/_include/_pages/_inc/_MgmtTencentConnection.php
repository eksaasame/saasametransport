<?php if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }?>
<script>
	/* Determine URL Segment */
	DetermineSegment('EditTencentConnection');
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
			url: '_include/_exec/mgmt_Tencent.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>'	 
			},
			success:function(jso)
			{
				$('#INPUT_ACCESS_KEY').val(jso.ACCESS_KEY);
				$('#INPUT_SECRET_KEY').val(jso.SECRET_KEY);
				$('#APP_ID').val(jso.APP_ID);
				
				$('#BackToMgmt').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
	}

	/* Verify Tencent Credential */
	function VerifyTencentAccessCredential(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Tencent.php',
			data:{
				 'ACTION'	 	:'VerifyTencentAccessCredential',
				 'ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val(),
				 'APP_ID' 		:$("#APP_ID").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso == true)
				{
					$('#INPUT_ACCESS_KEY').prop('disabled', true);
					$('#INPUT_SECRET_KEY').prop('disabled', true);
					$('#APP_ID').prop('disabled', true);
				
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifyConnection').prop('disabled',false);
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#CheckAndSubmit').prop('disabled', true);
					$('#VerifyConnection').removeClass('btn-default').addClass('btn-primary')
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#VerifyConnection').prop('disabled',false);
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: "Authentication failed!",						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#CheckAndSubmit').prop('disabled', true);
			},
		});
	}
	
	
	/* Initialize New Tencent Connection */
	function InitializeNewTencentConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Tencent.php',
			data:{
				 'ACTION'	 	:'InitializeNewTencentConnection',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 	:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val(),
				 'APP_ID' 		:$("#APP_ID").val()
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
	
	
	/* Update Tencent Connection */
	function UpdateTencentConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Tencent.php',
			data:{
				 'ACTION'	 		:'UpdateTencentConnection',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val(),
				 'APP_ID' 		:$("#APP_ID").val()				 
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
			VerifyTencentAccessCredential();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if ($URL_SEGMENT == 'AddTencentConnection')
			{				
				InitializeNewTencentConnection();
			}
			else
			{
				UpdateTencentConnection();
			}
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img style="padding-bottom:2px;" src=" data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAeCAYAAAA7MK6iAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4gcbAyIhS/30gAAAAhFJREFUSMft1l9ojlEcB/DP8zZrQ6QsieLSpYu5EombtStK3FPcSUnKjUgpxBSblSs35kZuhFAUN+SGFE0MRVmGxMz+PC6cdx2n99mebbZS+9Xpfc55f9/z/f095zArMyTZJHFrMBy+K3iKgekycg7O4TN+4Hs0vqJ9OqKyNniUlxgtU2PLRrNwpATZSDJvm1KOsyzbled5Z7T0E324EH7hF06iDvWR7jEcnIzT6xNvPmIT5hXo78CXyOsBNE+UdBneJOFbWSJ6i0PxxbiaUlewXo+GaH4IPeMQ5/iETmzBt5CGCcnqyOK+SXbCZuwrU1ONwdqiSt0TDosiacBeDBVU/vmk+EZD/qBE23SOQXypBP5e7HljDdJhDIaRev4BixLSKzX6ejB4P5z896QKOhMt9uMatmMFlmI3HhWAm9AdpybLstsBszzUSiuuJ/ibgmXVhWdYWJC/9kjvfljvTja8MU7Vj0QR/Su8W8cANuFuSEtzjT5/HrwsklVpf1c/hrCuxA3VgvfJJldLtFjdpIk7OjoyvE42uFWyt+cWEY/gQNrkeT566m3AqyRXXRN4UJxIiR9Hk17Mr2oOvHwRA3tiYKVSOTyBk2xJQvo2w0bciZR6cRzv/tyMWX+e523JJfEQR4OR+TikC3AqudV2VsN0tuTr4l+M/allF2eA9HTRS6QVl6eBsAvbpvCqnZX/UH4DhmU5v0bbN/cAAAAASUVORK5CYII=">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud Type"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			<label for='comment'><?php echo _("Secret Id"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_ACCESS_KEY' class='form-control' value='' placeholder='Access Key'>	
			</div>
			
			<label for='comment'><?php echo _("Secret Key"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_SECRET_KEY' class='form-control' value='' placeholder='Secret Key'>	
			</div>
			
			<label for='comment'><?php echo _("App Id"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='APP_ID' class='form-control' value='' placeholder='App Id'>	
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
