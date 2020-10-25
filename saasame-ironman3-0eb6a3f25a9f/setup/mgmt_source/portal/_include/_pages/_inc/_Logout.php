<script>
	<!-- Logout User -->
	function SubmitUserInput(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'   :'LogoutUser'			
			},
			success:function(jso)
			{
				window.location.href = "./?lang="+jso.language;
			},
			error: function(xhr)
			{
				
			}
		});	    
	}
	
	SubmitUserInput();
</script>