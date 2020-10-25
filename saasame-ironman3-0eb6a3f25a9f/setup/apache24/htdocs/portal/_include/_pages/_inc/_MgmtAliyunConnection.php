<?php if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }?>
<script>
	/* Determine URL Segment */
	DetermineSegment('EditAliyunConnection');
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
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>'	 
			},
			success:function(jso)
			{
				$('#INPUT_ACCESS_KEY').val(jso.ACCESS_KEY);
				$('#INPUT_SECRET_KEY').val(jso.SECRET_KEY);
				
				$('#BackToMgmt').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
	}

	/* Verify Aliyun Credential */
	function VerifyAliyunAccessCredential(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'	 	:'VerifyAliyunAccessCredential',
				 'ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val()		 
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code == true)
				{
					$('#INPUT_ACCESS_KEY').prop('disabled', true);
					$('#INPUT_SECRET_KEY').prop('disabled', true);
				
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifyConnection').prop('disabled',false);
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
	
	
	/* Initialize New Aliyun Connection */
	function InitializeNewAliyunConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'	 	:'InitializeNewAliyunConnection',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 	:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val()	
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
	
	
	/* Update Aliyun Connection */
	function UpdateAliyunConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_aliyun.php',
			data:{
				 'ACTION'	 		:'UpdateAliyunConnection',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'ACCESS_KEY' 	:$("#INPUT_ACCESS_KEY").val(),
				 'SECRET_KEY' 	:$("#INPUT_SECRET_KEY").val()	
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
			VerifyAliyunAccessCredential();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if ($URL_SEGMENT == 'AddAliyunConnection')
			{				
				InitializeNewAliyunConnection();
			}
			else
			{
				UpdateAliyunConnection();
			}
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img style="padding-bottom:2px;" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAeCAYAAAA7MK6iAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4QcZAjQO0eusHAAAAcpJREFUSMft1r9rVEEQB/DPuxxqVBS0UYLGxkbxF9opVnb6TwhiY+V/Y+8/YCkoCEFMFRQFCaJW/gQjhHBGSHJr4V7Y7O17985DsbiBLfbNzndmZ74zb5nKVP6FhBDGOjvu+VSqVFFV29sj2FOHga/4GfcdzMfvJdnEh4KPHdGcxJMIUrfWcSEB7qA3wuYFztelYB7fRgAEfMTBLHWPW9ht4UYpJQ9HGG7gOW4Xgr6GpzH9TRiLmA0hbNf4AFazQF7jZXS2iCX8yGuV1a2Lc7gSy3Eal7J6H8fnwYe5LLK38UA1IcO7WMiwTwwUCoxcx/cGpu5sjao2vk18KSk6f6vPSy2bp6I9wu+bHY1l6CeqtRDCcsPNJ3Oc9Pr1yPLBjd5jeRyQ7riprqpqIRJmIuk0+wltSTSkS2xDk+MccRf6KVgbYqVtldgea7I5XJgyS7iLiziEWcw0ML3C7jiMzuJWoYdDJGbrkbmKZ7iPm4XAL+MeHsW+rcN5hb0hhKGfRK/FsH8Xb5XaPmhhF9I5n6frahz2a/FvUlorOJXZrsS+Lp3fwBvcGTVV9sWHwP4aPvRj3/YSgp5p4M8WPsURPPwY+NPnz1Sm8t/JL3GFCv2Elw5kAAAAAElFTkSuQmCC">&nbsp;<?php echo _("Cloud"); ?>
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
				<input type='text' id='INPUT_ACCESS_KEY' class='form-control' value='' placeholder='Access Key'>	
			</div>
			
			<label for='comment'><?php echo _("Secret Key"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_SECRET_KEY' class='form-control' value='' placeholder='Secret Key'>	
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
