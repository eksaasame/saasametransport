<script>
	function InputResetPassword(){
		BootstrapDialog.show({
            message: 'Please Input reset type (Hint: Global or Jobs): <input type="text" class="form-control">',
            onhide: function(dialogRef){
                var ResetType = dialogRef.getModalBody().find('input').val();
					ResetDatabase(ResetType);
            },
            buttons: [{
                label: 'Submit',
				cssClass: 'btn-primary',
                action: function(dialogRef) {
                    dialogRef.close();
                }
            }]
        });
	}

	<!-- Reset Database -->
	function ResetDatabase(ResetType){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'    :'ResetDatabase',
				 'REST_TYPE' :ResetType
			},
			success:function(jso)
			{
				if (jso == true)
				{
					BootstrapDialog.show({
							title: 'Reset Message',
							message: 'Database erased.',
							draggable: true,
							buttons:[{
								label: '<?php echo _("Close"); ?>',
								action: function(dialogRef){
								dialogRef.close();
								}
							}],
							onhide: function(dialogRef){
								window.location.href = "./"
							},
					});
				}
				else
				{
					BootstrapDialog.show({
							title: 'Reset Message',
							message: 'Failed to reset database.',
							type: BootstrapDialog.TYPE_DANGER,
							draggable: true,
							buttons:[{
								label: '<?php echo _("Close"); ?>',
								action: function(dialogRef){
								dialogRef.close();
								}
							}],
							onhide: function(dialogRef){
								window.location.href = "./"
							},
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});	    
	}
	InputResetPassword();
</script>
<div id="container">
	Add Reset Image Here
</div><!-- /container -->
