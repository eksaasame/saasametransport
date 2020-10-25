<?php 
	if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }
	if (!isset($_SESSION['ENDPOINTS'])){ $_SESSION['ENDPOINTS'] = null; }
?>
<script>
$(document).ready(function(){
	/* Determine URL Segment */
	DetermineSegment('EditOpenStackConnection');
	function DetermineSegment(SET_SEGMENT){
		QueryPortocolList();
		URL_SEGMENT = window.location.pathname.split('/').pop();
		if (URL_SEGMENT == SET_SEGMENT)
		{
			QueryAccessCredentialInformation();
		}
		else
		{
			DEFINE_PROJECT_ID = false;
			DEFINE_TEXT = 'Project';
			$('#INPUT_PROJECT_ID').append(new Option(DEFINE_TEXT,DEFINE_TEXT, true, true));
			$("#INPUT_PROJECT_ID").val(DEFINE_TEXT);
			$('.selectpicker').selectpicker('refresh');
		}
	}

	/* Query Portocol List */
	function QueryPortocolList(){
		$('#INPUT_IDENTITY_PROTOCOL').prop('disabled', false);
		Portocol = {"HTTP":"http", "HTTPS":"https"};
		$.each(Portocol, function(key,value)
		{
			$('#INPUT_IDENTITY_PROTOCOL').append(new Option(key,value, true, false));
		});
		$('.selectpicker').selectpicker('refresh');
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
				$("#INPUT_PROJECT_NAME").val(jso.PROJECT_NAME);
				$("#INPUT_CLUSTER_USERNAME").val(jso.CLUSTER_USER);
				$("#INPUT_CLUSTER_PASSWORD").val(jso.CLUSTER_PASS);
				$("#INPUT_CLUSTER_VIP_ADDR").val(jso.CLUSTER_ADDR);
				$("#INPUT_IDENTITY_PROTOCOL").val(jso.IDENTITY_PROTOCOL.toLowerCase());
				$("#INPUT_IDENTITY_PORT").val(jso.IDENTITY_PORT);
				
				$("#BackToMgmt").remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
				
				DEFINE_PROJECT_ID = jso.AUTH_PROJECT_ID;
				DEFINE_TEXT = jso.AUTH_PROJECT_REGION;
				$('#INPUT_PROJECT_ID').append(new Option(DEFINE_TEXT,DEFINE_TEXT, true, true));
				$("#INPUT_PROJECT_ID").val(DEFINE_TEXT);
				$('.selectpicker').selectpicker('refresh');
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
				 'IDENTITY_PROTOCOL':$("#INPUT_IDENTITY_PROTOCOL").val(),
				 'CLUSTER_VIP_ADDR' :$("#INPUT_CLUSTER_VIP_ADDR").val(),
				 'IDENTITY_PORT' 	:$("#INPUT_IDENTITY_PORT").val()			 
			},
			success:function(jso)
			{
				//$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.Code == true)
				{
					$('#INPUT_IDENTITY_PROTOCOL').prop('disabled', true);
					$('#INPUT_PROJECT_NAME').prop('disabled', true);
					$('#INPUT_CLUSTER_USERNAME').prop('disabled', true);
					$('#INPUT_CLUSTER_PASSWORD').prop('disabled', true);
					$('#INPUT_CLUSTER_VIP_ADDR').prop('disabled', true);
					$('#INPUT_IDENTITY_PORT').prop('disabled', true);
								
					if (jso.AuthVersion == 'v3')
					{
						$('#INPUT_PROJECT_ID').prop('disabled', false);
						$('#INPUT_PROJECT_ID').empty();
						
						if (jso.Project == null)
						{
							$('#INPUT_PROJECT_ID').append(new Option('Not Applicable', false, true, false));
							$('#INPUT_PROJECT_ID').prop('disabled', true);
						}
						else
						{
							$.each(jso.Project, function(key,value)
							{
								$('#INPUT_PROJECT_ID').append(new Option(value, key, true, false));
							});
						}						
					}
					else
					{
						$('#INPUT_PROJECT_ID').prop('disabled', false);
						$('#INPUT_PROJECT_ID').empty();
						
						$.each(jso.Project, function(key,value)
						{
							$('#INPUT_PROJECT_ID').append(new Option(value, key, true, false));
						});
					}
					
					$("#INPUT_PROJECT_ID").val(DEFINE_PROJECT_ID);
					$('.selectpicker').selectpicker('refresh');	

					QueryEndpointInformation();
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
	
	/* List API Endpoints */
	function EditApiEndpoints(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_openstack.php',
			data:{
				 'ACTION'			:'EditApiEndpoints',
				 'ENDPOINT_REF_ADDR':ENDPOINT_REF
			},
			success:function(jso)
			{
				$('#ListApiEndpoints').prop('disabled', true);
				window.setTimeout(function(){
					BootstrapDialog.show
					({
						title: '<?php echo _('OpenStack API Endpoints'); ?>',
						cssClass: 'openstack-endpoint-dialog',
						message: jso,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						closable: true,
						onhidden: function(dialogRef){
							$('#ListApiEndpoints').prop('disabled', false);
						},
						buttons:
						[
							{
								label: '<?php echo _('Submit'); ?>',
								cssClass: 'btn-primary',
								action: function(dialogRef)
								{
									var EndPointArray = [];
									$("#EndpointTable tr").each(function(){
										id = $(this).parents('table').find('input').attr('id');
										value = $("#"+id).val();
										EndPointArray.push(id+','+value);
									});
									ENDPOINT_REF = JSON.stringify(EndPointArray);
									dialogRef.close();
								}
							}
						]				
					});
				}, 0);
			},
			error: function(xhr)
			{
				
      		}
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
				 'PROJECT_ID'		:$("#INPUT_PROJECT_ID").val(),				
				 'CLUSTER_USER' 	:$("#INPUT_CLUSTER_USERNAME").val(),
				 'CLUSTER_PASS' 	:$("#INPUT_CLUSTER_PASSWORD").val(),
				 'IDENTITY_PROTOCOL':$("#INPUT_IDENTITY_PROTOCOL").val(),
				 'CLUSTER_VIP_ADDR' :$("#INPUT_CLUSTER_VIP_ADDR").val(),
				 'IDENTITY_PORT' 	:$("#INPUT_IDENTITY_PORT").val(),
				 'ENDPOINT_REF_ADDR':ENDPOINT_REF
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
				 'PROJECT_ID'		:$("#INPUT_PROJECT_ID").val(),
				 'CLUSTER_USER' 	:$("#INPUT_CLUSTER_USERNAME").val(),
				 'CLUSTER_PASS' 	:$("#INPUT_CLUSTER_PASSWORD").val(),
				 'IDENTITY_PROTOCOL':$("#INPUT_IDENTITY_PROTOCOL").val(),
				 'CLUSTER_VIP_ADDR' :$("#INPUT_CLUSTER_VIP_ADDR").val(),
				 'IDENTITY_PORT' 	:$("#INPUT_IDENTITY_PORT").val(),
				 'ENDPOINT_REF_ADDR':ENDPOINT_REF
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
		
		$("#ListApiEndpoints").click(function(){
			EditApiEndpoints();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if (URL_SEGMENT == 'AddOpenStackConnection')
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

function QueryEndpointInformation(){
	
	$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
	
	$('#INPUT_PROJECT_ID').prop('disabled', true);
	
	$('#ListApiEndpoints').prop('disabled', true);
	$('#ListApiEndpoints').removeClass('btn-success').addClass('btn-default');

	$('#CheckAndSubmit').prop('disabled', true);
	$('#CheckAndSubmit').removeClass('btn-primary').addClass('btn-default');
	
	$.ajax({
		type: 'POST',
		dataType:'JSON',
		url: '_include/_exec/mgmt_openstack.php',
		data:{
			 'ACTION'      		:'QueryEndpointInformation',
			 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
			 'PROJECT_NAME' 	:$("#INPUT_PROJECT_NAME").val(),
			 'CLUSTER_USER' 	:$("#INPUT_CLUSTER_USERNAME").val(),
			 'CLUSTER_PASS' 	:$("#INPUT_CLUSTER_PASSWORD").val(),
			 'IDENTITY_PROTOCOL':$("#INPUT_IDENTITY_PROTOCOL").val(),
			 'CLUSTER_VIP_ADDR' :$("#INPUT_CLUSTER_VIP_ADDR").val(),
			 'IDENTITY_PORT' 	:$("#INPUT_IDENTITY_PORT").val(),
			 'PROJECT_ID'		:$("#INPUT_PROJECT_ID").val(),
		},
		success:function(jso)
		{
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			
			ENDPOINT_REF = jso;
			
			$('#INPUT_PROJECT_ID').prop('disabled', false);
			
			$('#ListApiEndpoints').prop('disabled', false);
			$('#ListApiEndpoints').removeClass('btn-default').addClass('btn-success');
			
			$('#CheckAndSubmit').prop('disabled', false);
			$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
		},
		error: function(xhr)
		{
			$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
		}
	});
}
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img style="padding-bottom:2px;" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABoAAAAcCAYAAAB/E6/TAAAEBXpUWHRSYXcgcHJvZmlsZSB0eXBlIGV4aWYAAHja7VdZkiMpDP3nFHMEJCEExyFZIuYGc/x5kEu53G6X3dUfMxGdhBEphCT0JBK7/s/fw/2FhyknF9RSzDF6PCGHzAWD5PenrJ58WP3+Uo45+sx3VI8JBktAZX9N8eB38BnyfPDrpZC83ijK/ZjYPk+UQxGnw8DBPw0J7Qb8odiVQ5HwYTns79uxhZiT3W6hHvLj3GLaf252QYyjRrKAPrA3ixnjxD4Y4tamo1s7Fuph6O7dnaIMn7gLiUcfp4cC9yVJAc3oWSJPTsB470Xy7inQYucN43zG9fnzzHN3un5A/gnS3h5DfY1ukHYroOeE3CEUL/qQT/rBd7eQLtxuLMd4Wf7E5zwDcvOcqM3fGC2N0ffdlRCx5Xhs6tzKGkFum9FaqyKa4ac+zWDPltESSqIijxqSbEOrlIkB46BAjQoN6otWqnAxcGcDZa4sVFEinABG5io7uGg02AB5A/gsFekg4PLlCy2zeZmrlGC4UXLIHoIywpJvNfeK0BizoIh82uOEtIBfPCsNXkzkiJwniAERGkdQdQX4bPfPxFWAoK4wJ2yw+G1qAPyb0kdyyQJaIKigewGTtUMBQgQPFM6QAAEfSZQieWN2RoRAJgBU4DpL4A2wkCo3OMlBUGLGqALYxhqjJcrKOxsHIYBQiU4M2KAoAVYIivyxkJBDRUWDqkY1TZq1RImzwmK0OE/UYmLB1KKZJctWXJIUkqaYLKWUU8mcBSeuZtRjTjnnUmC0QHPB6gKBUjbeZAubbnGzLW15K5VdlRqq1litpppradykoY5bbNZSy6106kilHrr22K2nnnsZSLUhIwwdcdhII7tRLtToKNv79gZqdKDGC6kpaBdq4JqdKmgeJzoxA2IcCIDbRIDEMU/MfKIQeCI3MfOZURXKcFInOI0mYkAwdGIddGH3gZw6ib8HNwcg+Hcg5yZ0LyD3I26PUGtlfehkITTLcAbVC6pv5NFT4VTml/IpdV8JvEo/FGWJaYSy91rmp/odjvu1Zf9hRb6s0MzbyDH4Lv2j6NcVsbRqLabV55Wx+pTjXhF6zkGVhyyuNuFcMyRw2galoPNC+piK2MB5uqHKU0t29d634XoLcaDOI9KMxtlj0c/X7EvuFjgf2zIIR1+l8yCUeYb6GHA55D4dcVC52/ZXP2/vy5Nlu8rVw+/lyaMF7icrxhdwM98x3NeJYmw4q9v8PmAj0r97QqbyVMA9mFjZkLeA0357YZc7dW+E4Sl1b4ThKXVvhOFpFNwbYXgaBccZ9/V1U8aOZsFhSivKTnDzH2X+hQ3lBSH3klTYoxh2bim+W9HPwXS/4yv7R9H/UtEYwzX8VXf/AlDfIT4p4oQ4AAAAYHpUWHRSYXcgcHJvZmlsZSB0eXBlIGlwdGMAAHjaPUnLDYBQCLszhSMAbRTWkXfx5sH9IyHRNm36ket+SrYBXRB0Jpey+cOWlTqOjgGHwhDt2Xmfp/o5e+UISPH1VVV5ATNDFOrlMNCPAAAPVGlUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4KPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iWE1QIENvcmUgNC40LjAtRXhpdjIiPgogPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4KICA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIgogICAgeG1sbnM6aXB0Y0V4dD0iaHR0cDovL2lwdGMub3JnL3N0ZC9JcHRjNHhtcEV4dC8yMDA4LTAyLTI5LyIKICAgIHhtbG5zOnhtcE1NPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvbW0vIgogICAgeG1sbnM6c3RFdnQ9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZUV2ZW50IyIKICAgIHhtbG5zOnBsdXM9Imh0dHA6Ly9ucy51c2VwbHVzLm9yZy9sZGYveG1wLzEuMC8iCiAgICB4bWxuczpHSU1QPSJodHRwOi8vd3d3LmdpbXAub3JnL3htcC8iCiAgICB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iCiAgICB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iCiAgIHhtcE1NOkRvY3VtZW50SUQ9ImdpbXA6ZG9jaWQ6Z2ltcDozN2M5MWQ4YS1hMmYzLTQwZjgtYmQyNy00ZTU2MTM5OGQ0M2MiCiAgIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MDI3NDRhYTUtNmFhMy00ZDI0LWJkNWEtMmVlMTcwMWQyOGI4IgogICB4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ9InhtcC5kaWQ6NWMxYTk5ZjMtZDk0Yi00YWFiLWFlNjAtNzAzM2E4YjBkMjRjIgogICBHSU1QOkFQST0iMi4wIgogICBHSU1QOlBsYXRmb3JtPSJXaW5kb3dzIgogICBHSU1QOlRpbWVTdGFtcD0iMTUzNzk0MjQ4MDY2OTcyMCIKICAgR0lNUDpWZXJzaW9uPSIyLjEwLjYiCiAgIGRjOkZvcm1hdD0iaW1hZ2UvcG5nIgogICB4bXA6Q3JlYXRvclRvb2w9IkdJTVAgMi4xMCI+CiAgIDxpcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgPGlwdGNFeHQ6TG9jYXRpb25TaG93bj4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uU2hvd24+CiAgIDxpcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgPGlwdGNFeHQ6UmVnaXN0cnlJZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OlJlZ2lzdHJ5SWQ+CiAgIDx4bXBNTTpIaXN0b3J5PgogICAgPHJkZjpTZXE+CiAgICAgPHJkZjpsaQogICAgICBzdEV2dDphY3Rpb249InNhdmVkIgogICAgICBzdEV2dDpjaGFuZ2VkPSIvIgogICAgICBzdEV2dDppbnN0YW5jZUlEPSJ4bXAuaWlkOjdjMzE2ODBiLTM1NDctNDYwMS05YTg4LTQ3MzExZmE5OGMwYyIKICAgICAgc3RFdnQ6c29mdHdhcmVBZ2VudD0iR2ltcCAyLjEwIChXaW5kb3dzKSIKICAgICAgc3RFdnQ6d2hlbj0iMjAxOC0wOS0yNlQxNDoxNDo0MCIvPgogICAgPC9yZGY6U2VxPgogICA8L3htcE1NOkhpc3Rvcnk+CiAgIDxwbHVzOkltYWdlU3VwcGxpZXI+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpJbWFnZVN1cHBsaWVyPgogICA8cGx1czpJbWFnZUNyZWF0b3I+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpJbWFnZUNyZWF0b3I+CiAgIDxwbHVzOkNvcHlyaWdodE93bmVyPgogICAgPHJkZjpTZXEvPgogICA8L3BsdXM6Q29weXJpZ2h0T3duZXI+CiAgIDxwbHVzOkxpY2Vuc29yPgogICAgPHJkZjpTZXEvPgogICA8L3BsdXM6TGljZW5zb3I+CiAgPC9yZGY6RGVzY3JpcHRpb24+CiA8L3JkZjpSREY+CjwveDp4bXBtZXRhPgogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgCjw/eHBhY2tldCBlbmQ9InciPz50zrRPAAAABmJLR0QAAAAAAAD5Q7t/AAAACXBIWXMAAA7EAAAOxAGVKw4bAAAAB3RJTUUH4gkaBg4oClc8ZQAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAAFISURBVEjH3dW/SlxBFAbw3+69g6wEWRFsgkSwEFMJdhHEt7DUJpAqT2CZJxB8FbEThICNuCSVZC2C2IjiH1SQwd00t7oMwRG94H4wzTnMfOd8c/4wamiFEAosYh6tV35/iGP0SmxgC+NvlMwDvrdCCH3MvbFyJ210G/iibrupYmiMqPyP7yf2McgIegXLOURXWIsxnuVEHUL4iN+YTBHtY6JmP8fNCxS6wS6ma/bbRifDNj7U7JfYjDE+ZEo3jh+YqrnuWiGEi4TjLxZjjNeZRF308Kke+Oj10WhOhnWMJUb7/Qveu8e3xMp5LHGYyOypOrl4whGKmn1Q4iBR3qf4gutMognsYaZe3mXVrPWGncVCCKGXSfS5uttJSZdCBzs4qfb+s6ZMtak7uWuii6V32UfDBniGbfQbIOoXRVH8wmqixF8Lf/D1H3a8QKcCRIBjAAAAAElFTkSuQmCC">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			<label for='comment' style='width:10%;'><?php echo _("Protocol"); ?></label>
			<label for='comment' style='width:81.1%;'><?php echo _("Identity Address"); ?></label>
			<label for='comment' style='width:8%;'><?php echo _("Port"); ?></label>
			<div class='form-group form-inline'>				
				<select id='INPUT_IDENTITY_PROTOCOL' class='selectpicker' data-width='10%' disabled></select>
				<input type='text' id='INPUT_CLUSTER_VIP_ADDR' 	class='form-control' value='' 	  placeholder='Address' style="width:81.1%" tabindex="1">	
				<input type='text' id='INPUT_IDENTITY_PORT' 	class='form-control' value='5000' placeholder='Identity Port' style="width:8%" tabindex="5">	
			</div>
			
			<label for='comment'><?php echo _("Project / Domain"); ?></label>
			<div class="form-group form-inline">
				<input type='text' id='INPUT_PROJECT_NAME' class='form-control' value='' placeholder='Project / Domain' style="width:78.25%" tabindex="2">
				<select id='INPUT_PROJECT_ID' class='selectpicker' data-width='13.1%' onchange="QueryEndpointInformation();" disabled></select>
				<button id='ListApiEndpoints' class='btn btn-default pull-right btn-md' disabled style="width:8%"><?php echo _("API Endpoins"); ?></button>
			</div>

			<div class='form-group form-inline'>				
				<label for='comment' style="width:49.5%"><?php echo _("Username"); ?></label>
				<label for='comment' style="width:50%"><?php echo _("Password"); ?></label>
				<input type='text' id='INPUT_CLUSTER_USERNAME' class='form-control' value='' placeholder='Username' style="width:49.5%" tabindex="3">
				<div class="input-group"><input type='password' id='INPUT_CLUSTER_PASSWORD' class='form-control pwd' value='' placeholder='Password' style="width:605px" tabindex="4"><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>
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