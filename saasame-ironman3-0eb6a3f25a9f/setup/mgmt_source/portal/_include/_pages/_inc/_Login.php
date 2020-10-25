<script>
	/* Redir to default Login URL */
	RedirToDefaultLoginURL();	
	function RedirToDefaultLoginURL(){
		url = window.location.pathname;
		lastSegment = url.split('/').pop();
		
		if (lastSegment != '')
		{
			window.location.href = "./";
		}
	}

	/* Login Check */
	function SubmitUserInput(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'   :'CheckAdministratorLogin',			
				 'USERNAME' :$("#INPUT_USERNAME").val(),
				 'PASSWORD' :$("#INPUT_PASSWORD").val()			
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
					TransformAccount();					
				}
				else
				{
					$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');					
					BootstrapDialog.show({
						title: 'Authorization Message',
						message: '<div style="color:#666666;">Login failed.</div>',
						type: BootstrapDialog.TYPE_WARNING,
						draggable: true
					});
				}
				
			},
			error: function(xhr)
			{
				
			}
		});	    
	}	
	
	/* Transform Account Information */
	function TransformAccount()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'    :'TransformAccount',
				 'USER_NAME' :$("#INPUT_USERNAME").val(),
				 'PASS_WORD' :$("#INPUT_PASSWORD").val(),
			},
			success:function(jso)
			{
				setTimeout(function () {window.location.href = '';}, 800);
				$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Submit Trigger */
	$(function(){
		$("#CheckAndSubmit").click(function(){			
			SubmitUserInput();
		})
	});
</script>
<style>
body{
	background-image: linear-gradient(to top, #557991 15%, #2b5876 85%);
	height: 100%;
    margin: 0;
    background-repeat: no-repeat;
    background-attachment: fixed;
}
</style>

<div id='<?php setCssId(); ?>'>		
	<label for='comment' style='color:#F0F0F0;'><i class="fa fa-user-o fa-fw"></i> <?php echo _("Username"); ?></label>
	<div class='form-group'>				
		<input type='text' id='INPUT_USERNAME' class='form-control' value='' placeholder='Username'>	
	</div>

	<label for='comment' style='color:#F0F0F0;'><i class="fa fa-key fa-fw"></i> <?php echo _("Password"); ?></label>
	<div class='form-group'>				
		<div class="input-group"><input type='password' id='INPUT_PASSWORD' class='form-control pwd' value='' placeholder='Password'><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>
	</div>			
	
	<div class='form-group'>
		<button id='CheckAndSubmit' class='btn btn-success pull-right' style='width:90px;'><?php echo _("Log in"); ?></button>	
	</div>				
</div> <!-- id: wrapper_block-->

<script>
	/* EYE PASSWORD */
	$(".reveal").mousedown(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'text'));
	})
	.mouseup(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'password'));
	})
	.mouseout(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'password'));
	});
</script>
