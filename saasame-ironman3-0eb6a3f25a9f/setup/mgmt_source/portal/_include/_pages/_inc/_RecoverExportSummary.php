<script>
$( document ).ready(function() {
	<!-- Exec -->
	QueryConversionType();
	QueryReplicaInformation();
	
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#BackToSelectHost').prop('disabled', false);
		$('#BackToMgmtRecoverWorkload').prop('disabled', false);
		
		setTimeout(function(){
			/* Change Submit Status */
			$('#ServiceSubmitAndRun').prop('disabled', false);
			$('#ServiceSubmitAndRun').removeClass('btn-default').addClass('btn-primary');		
		},1000);
	});
	
	/* Query Convert Type */
	function QueryConversionType(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'    :'QueryConvertType'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#SET_CONVERT_TYPE').append(new Option(key,value, true, false));
					});
					
					/* SELECT SET USER LANGUAGE */
					$("#SET_CONVERT_TYPE").val(-1);

					/* SET AS DISABLE */
					//$('#SET_CONVERT_TYPE').prop('disabled', true);
					
					$('.selectpicker').selectpicker('refresh');
				}			
			},
			error: function(xhr)
			{

			}
		});
	}	
	
	/* Query Select Replica */
	function QueryReplicaInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryReplicaInformation',
				 'REPL_UUID' :'<?php echo $_SESSION['REPL_UUID']; ?>'				 
			},
			success:function(jso)
			{
				/* Decode Jobs JSON */
				JOB_JSON = jso.JOBS_JSON;			
				
				$('#ReplicaSummaryTable > tbody').append(
					'<tr><th colspan="2"><?php echo _("Host Information"); ?></th></tr>\
				');
				
				var migration_executed = jso.JOBS_JSON.migration_executed;				
				if (typeof(migration_executed) != "undefined" && migration_executed !== null && migration_executed == true)
				{
					$('#TRIGGER_SYNC').prop('disabled', true);
				}			
				
				skip_disk = jso.JOBS_JSON.skip_disk;
				
				QueryHostInformation(jso.PACK_UUID);
				
				CONN_INFO = jQuery.parseJSON(jso.CONN_UUID);
				TARGET_CONN_UUID = CONN_INFO.TARGET;
				QueryConnectionInformation();
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Query Select Host */
	function QueryHostInformation(PACK_UUID){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryHostInformation',
				 'HOST_UUID' :PACK_UUID				 
			},
			success:function(jso)
			{
				if (jso.HOST_TYPE == 'Virtual')
				{
					$('#ReplicaSummaryTable > tbody').append('\
						<tr><td width="165px"><?php echo _("Machine ID"); ?></td>	<td>'+jso.HOST_INFO.uuid+'</td></tr>\
						<tr><td width="165px"><?php echo _("Host Name"); ?></td>	<td>'+jso.HOST_NAME+'</td></tr>\
						<tr><td width="165px"><?php echo _("Host Type"); ?></td>	<td>'+jso.HOST_TYPE+'</td></tr>\
					');
					
				}
				else
				{
					$('#ReplicaSummaryTable > tbody').append('\
						<tr><td width="165px"><?php echo _("Host Name"); ?></td>	<td>'+jso.HOST_NAME+'</td></tr>\
						<tr><td width="165px"><?php echo _("Host Address"); ?></td><td>'+jso.HOST_ADDR+'</td></tr>\
						<tr><td width="165px"><?php echo _("Host Type"); ?></td>	<td>'+jso.HOST_TYPE+'</td></tr>\
					');
				}

				i = 0;
				var SKIP_DISK = skip_disk.split(',');
				$.each(jso.HOST_DISK, function(key,value)
				{
					/* Default Strike Style */
					strike = '';
					$.each(SKIP_DISK, function(SkipKey,SkipValue)
					{
						if (value.DISK_UUID == SkipValue)
						{
							strike = 'strike_line';
						}
					});
					
					$('#ReplicaSummaryTable').append('<tr><td width="165px">Disk '+i+'</td><td class="'+strike+'">'+value.DISK_NAME+' ('+(value.DISK_SIZE/1024/1024/1024)+'GB)</td></tr>');
					i++;
				});				
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
	/* Query Connection	*/
	function QueryConnectionInformation(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryConnectionInformation',
				 'CONN_UUID' :TARGET_CONN_UUID
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$('#ConnectionTable > tbody').append(
						'<tr><th colspan="2"><?php echo _("Image Export Information"); ?></th></tr>\
						 <tr><td width="165px"><?php echo _("Server Host Name"); ?></td><td>'+jso.LAUN_HOST+'</td></tr>\
						 <tr><td width="165px"><?php echo _("Server Address"); ?></td><td>'+jso.LAUN_ADDR+'</td></tr>\
						 <tr><td width="165px"><?php echo _("Service Location"); ?></td><td>'+jso.CONN_DATA.CONN_TYPE+'</td></tr>\
						 <tr><td width="165px"><?php echo _("Image Export Path"); ?></td><td>'+JOB_JSON.export_path+'</td></tr>\
						 <tr><td width="165px"><?php echo _("Management Address"); ?></td><td>'+document.domain+'</td></tr>');
				}
				else
				{
					$('#ConnectionTable > tbody').append('<tr><td class="organisationnumber">-</td>\
					<td colspan="8">No Connection</td>\
					</tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
	/* Click And Run Recover Kit Service */
	function BeginToRunRecoverImageExportServiceAsync(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'       :'BeginToRunRecoverImageExportServiceAsync',
				 'ACCT_UUID'    :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID'    :'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 'REPL_UUID'    :'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'CONN_UUID'    :TARGET_CONN_UUID,
				 'CONVERT_TYPE' :$("#SET_CONVERT_TYPE").val(),
				 'TRIGGER_SYNC' :$('#TRIGGER_SYNC:checkbox:checked').val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				BootstrapDialog.show({
					title: "<?php echo _("Service Message"); ?>",
					message: "<?php echo _("New recovery process added."); ?>",
					draggable: true,
					closable: false,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}
					}],
					onhide: function(dialogRef){
						window.location.href = "MgmtRecoverWorkload";
					},
				});
			},
		});
	}
	

	/* Submit Trigger */
	$(function(){
		$("#BackToMgmtRecoverWorkload").click(function(){
			window.location.href = "MgmtRecoverWorkload";
		})
		
		$("#BackToSelectHost").click(function(){
			window.location.href = "SelectRecoverHost";
		})
		
		$("#ServiceSubmitAndRun").one("click" ,function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');			
			$('#BackToMgmtRecoverWorkload').prop('disabled', true);
			$('#BackToSelectHost').prop('disabled', true);
			$('#ServiceSubmitAndRun').prop('disabled', true);
			$('#ServiceSubmitAndRun').removeClass('btn-primary').addClass('btn-default');
			BeginToRunRecoverImageExportServiceAsync();
		})
	});
});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?>
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:18%'><a>				<?php echo _("Step 1 - Select Host"); ?></a></li>
					<li style='width:22%'><a>				<?php echo _("Step 2 - Select Recovery Type"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 3 - Select Snapshot"); ?></a></li>
					<li style='width:20%'><a>				<?php echo _("Step 4 - Configure Instance"); ?></a></li>
					<li style='width:20%' class='active'><a><?php echo _("Step 5 - Recovery Summary"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				
				<table id="TriggerSync">
					<tbody>
						<div class='form-group' style='float:right;'>
							<div style='display:none;'><?php echo _("Trigger data synchronize:"); ?> <input id="TRIGGER_SYNC" data-toggle="toggle" data-width="60" data-style="slow" type="checkbox" ></div>
							<select id="SET_CONVERT_TYPE" class="selectpicker" data-width="110px"></select>
						</div>
					</tbody>
				</table>			
				
				<table id="ReplicaSummaryTable">
					<tbody></tbody>
				</table>
				
				<table id="ConnectionTable">
					<tbody></tbody>
				</table>
			</div>
			
			<div id='title_block_wizard'>
				<div class='btn-toolbar'>
					<button id='BackToSelectHost' 			class='btn btn-default pull-left btn-lg' disabled><?php echo _("Back"); ?></button>	
					<button id='BackToMgmtRecoverWorkload' 	class='btn btn-default pull-left btn-lg' disabled><?php echo _("Cancel"); ?></button>
					<button id='ServiceSubmitAndRun' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Run"); ?></button>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->
</div>