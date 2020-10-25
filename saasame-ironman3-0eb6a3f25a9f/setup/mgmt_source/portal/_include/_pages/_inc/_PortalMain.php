<script>
	/* Begin to Automatic Exec */
	UnsetSession();
		
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
</script>
<div id="container">
	this is portal main
</div><!-- /container -->

<?php
echo '<div style="width:66%"><pre>';
print_r($_SESSION);
echo '</pre></div>';