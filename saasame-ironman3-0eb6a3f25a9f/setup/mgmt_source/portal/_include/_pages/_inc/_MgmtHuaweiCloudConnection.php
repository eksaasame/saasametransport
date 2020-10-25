<?php if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }?>
<script>
$(document).ready(function(){
	/* Determine URL Segment */
	DetermineSegment('EditHuaweiCloudConnection');
	function DetermineSegment($SET_SEGMENT){
		URL_SEGMENT = window.location.pathname.split('/').pop();
		SELECT_REGION = '';
		if (URL_SEGMENT == $SET_SEGMENT)
		{
			QueryAccessCredentialInformation();
		}
	}
		
	/* Query Access Credential Information */
	function QueryAccessCredentialInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>'	 
			},
			success:function(jso)
			{
				SELECT_REGION = jso.REGION;
				
				$("#INPUT_PROJECT_NAME").val(jso.PROJECT_NAME);
				$("#INPUT_CLUSTER_USERNAME").val(jso.CLUSTER_USER);
				$("#INPUT_CLUSTER_PASSWORD").val(jso.CLUSTER_PASS);
				
				var mapObj = {"cn-north-1":"华北-北京一", "cn-east-2":"华东-上海二", "cn-south-1":"华南-广州", "cn-northeast-1":"东北-大连", "ap-southeast-1":"亚太-香港"};
				REGION_NAME = SELECT_REGION.replace(/cn-north-1|cn-east-2|cn-south-1|cn-northeast-1|ap-southeast-1/gi, function(matched){return mapObj[matched];});
				
				$('#SELECT_CLOUD_REGION').append(new Option(REGION_NAME,REGION_NAME, true, true));
				$("#SELECT_CLOUD_REGION").val(REGION_NAME);
				$('.selectpicker').selectpicker('refresh');	
				
				
				$("#BackToMgmt").remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
	}
	
	/* Verify OpenStack Connection */
	function VerifyOpenStackConnection(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'	 		:'VerifyOpenStackConnection',
				 'PROJECT_NAME' 	:$("#INPUT_PROJECT_NAME").val(),
				 'CLUSTER_USER' 	:$("#INPUT_CLUSTER_USERNAME").val(),
				 'CLUSTER_PASS' 	:$("#INPUT_CLUSTER_PASSWORD").val(),
				 'IDENTITY_PROTOCOL':'https',
				 'CLUSTER_VIP_ADDR' :'iam.myhuaweicloud.com',
				 'IDENTITY_PORT' 	:443
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
						$('#SELECT_CLOUD_REGION').append(new Option(key,value, true, true));
					});
					
					$("#SELECT_CLOUD_REGION").val(SELECT_REGION);
					$('.selectpicker').selectpicker('refresh');
					
					$('#INPUT_PROJECT_NAME').prop('disabled', true);
					$('#INPUT_CLUSTER_USERNAME').prop('disabled', true);
					$('#INPUT_CLUSTER_PASSWORD').prop('disabled', true);
					$('#INPUT_CLUSTER_VIP_ADDR').prop('disabled', true);					
					
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
					$('#VerifyConnection').removeClass('btn-default').addClass('btn-primary');
    			}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
 			},
		});
	}
	
	
	/* Initialize New OpenStack Connection */
	function InitializeNewOpenStackConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'	 		:'InitializeNewOpenStackConnection',
				 'ACCT_UUID' 		:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 		:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'PROJECT_NAME' 	:$("#INPUT_PROJECT_NAME").val(),
				 'PROJECT_ID'		:false,
				 'CLUSTER_USER' 	:$("#INPUT_CLUSTER_USERNAME").val(),
				 'CLUSTER_PASS' 	:$("#INPUT_CLUSTER_PASSWORD").val(),
				 'IDENTITY_PROTOCOL':'https',
				 'CLUSTER_VIP_ADDR' :'iam.'+$("#SELECT_CLOUD_REGION").val()+'.myhuaweicloud.com',
				 'IDENTITY_PORT'	:443,
				 'ENDPOINT_REF_ADDR':''
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
	
	
	/* Update OpenStack Connection */
	function UpdateOpenStackConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'	 		:'UpdateOpenStackConnection',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 'PROJECT_NAME' 	:$("#INPUT_PROJECT_NAME").val(),
				 'PROJECT_ID'		:false,
				 'CLUSTER_USER' 	:$("#INPUT_CLUSTER_USERNAME").val(),
				 'CLUSTER_PASS' 	:$("#INPUT_CLUSTER_PASSWORD").val(),
				 'IDENTITY_PROTOCOL':'https',
				 'CLUSTER_VIP_ADDR' :'iam.'+$("#SELECT_CLOUD_REGION").val()+'.myhuaweicloud.com',
				 'IDENTITY_PORT'	:443,
				 'ENDPOINT_REF_ADDR':''
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
			VerifyOpenStackConnection();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if (URL_SEGMENT == 'AddHuaweiCloudConnection')
			{				
				InitializeNewOpenStackConnection();
			}
			else
			{
				UpdateOpenStackConnection();
			}
		})
	});
});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img style="padding-bottom:1px;" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACIAAAAaCAYAAADSbo4CAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4gYFDSoVJaiuKwAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAAD+ElEQVRIx62XXagVVRTHf2ff38GOXlMT1LIsM6jsCyxRIcEeJDQtMgUrKCIyeon0QQqJiiASyW5EpUFQQfpQSmRESAmlFPSg9B1qgX1hZmJyIzkzd6YH98g4zpyb4ILNmVlr7//6z15rr71OH/9D+vr6yPMctS+EMD6EMCWEMDrLsqPtdpssy+jv7yfLspkhhL4QQp5lWRdAJcsyzpqoY9V16s9qru4tSEb7zKjP1c/UOzmboqIGdUvJUTFmlOa9V7F11bvUM3dafF1BoPR8cw2JXB0ozclq7L+oI6uYdeRC+WVoaKggFNI0LS9Y1cB9UQSe1WC/ELiqcB4xR1SwTycSF8xvtVofVtSXNDgq9Nf02OhpQEFiJrBLHZ2maTMRdRwwAMxXN5YmH2pwEtTJwKVAq2HObxF7PLAduAF4IubdqURKitXAdCAHVqjLo357jy+eHEeTfKW2gVeAsVG3EriyvCuhtG2jgEejvvi6teok4M0ejiYCYxpsnwCDwB3AskokHmsKzcIaoCnAWuBXYGeDs/FAu8G2Oe7CWzW2JU1Ezm0AuwdYDrzUYO8H+hpy4yNgW92hADpNRHb22P4XgZ+A/XXlB6irWtuABcDsBszdpxAJ4QSXNE33xpjWyThgBfBGjS2Loyw58CrwSI/TtLpUuQkhhDnqjQCtVmsp8G3DwruBHcCBir4LDFV0D8ccmNaAtT5N04/VTgx9P+rr6nF1szpBHaPuaijpn6rXlt6zeJ9sK+m+UK9vWJ+rK+NOLFW/jrrrAjASGBET8iDwQJqmc4Hnar5kbqyiG0q6w0ASn9O45XXH/RAwB9ik7gDeBq4uEjcA+0qTW8A69RtgF7A4hqMsLwAbgT/i+xHgWHx+F7g4FsVC/okn7tZ4Nx0Ebqpg/h6ALTXspwNbY4EbAG4B/izVjQXAs5H4X3GkwOPA/SWc94FZ0b4VWNNwev5GHVmJcd3YpE5UN6hpjO1odXfMqafV29V5cf5hdUlslg72wE3Vh8qX3eXqnmHIHFKXqTNigs9Tx8XLa0LEWauuUq9QB4bBy9WXT95zpWZlkvqkOjjM4u/UhfFuqrYQF6hPqceGwdij3qaevHRPKzadTqeVJMlU4KKaeOYxMQdDCD92u928QqQDXBarbV5XQFqt1oEkSY7GZ/L8xDQrQIuSJFkRE68Tj3UHGAWcA5wH/AA80+129zcQXQw8GBN5EPgXOB5/B/M8z9RjwKo0TY80Ncrnq9/32NLn1SlNfWfR96pzYyI34az3hPTs2qeq+2qa4Nm9CBTSbrcLnLa6Rk0qWK/Vhqz616HomtR7Y7y/TNP0ncJJkiRn+ndkAnBfbBc+SNP086ovgP8AajZoFEsyf0UAAAAASUVORK5CYII=">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			 <!-- No Use For Huawei Cloud-->
			<div hidden>
				<label for='comment'><?php echo _("VIP Address"); ?></label>
				<div class='form-group has-primary'>				
					<input type='text' id='INPUT_CLUSTER_VIP_ADDR' class='form-control' value='' placeholder='VIP Address'>	
				</div>				
			</div>
			<!-- No Use For Huawei Cloud-->
			
			<label for='comment' style='width:32%;'><?php echo _("Account Name"); ?></label>
			<label for='comment' style='width:32%;'><?php echo _("IAM User name"); ?></label>
			<label for='comment' style='width:31.5;'><?php echo _("Password"); ?></label>
			<div class='form-group form-inline'>
				<input type='text' id='INPUT_PROJECT_NAME' class='form-control' value='' placeholder='Account Name' style='width:32%;'>	
				<input type='text' id='INPUT_CLUSTER_USERNAME' class='form-control' value='' placeholder='IAM User name' style='width:32%;'>
				<div class="input-group"><input type='password' id='INPUT_CLUSTER_PASSWORD' class='form-control pwd' value='' placeholder='Password' style='width:416px;'><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>
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
