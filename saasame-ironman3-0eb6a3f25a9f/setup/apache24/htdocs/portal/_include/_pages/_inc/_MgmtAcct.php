<script>
$(document).ready(function(){
	/* Enable Buttons When All Ajax Request Stop*/
	$(document).one("ajaxStop", function() {
		$('#SelectXray').prop('disabled', false);
		$('#SelectXray').removeClass('btn-default').addClass('btn-primary');

		$('#CheckAndSubmit').prop('disabled', false);
		$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
		
		$('#VerifySMTPSetting').prop('disabled', false);
		$('#VerifySMTPSetting').removeClass('btn-default').addClass('btn-primary');
		
		$('#SubmitOnlineLicenseActivation').prop('disabled', false);
		$('#SubmitOnlineLicenseActivation').removeClass('btn-default').addClass('btn-primary');
		
		$("#INPUT_SMTP_USER").change(function(){if ($("#INPUT_SMTP_FROM").val() == ""){var sender_email = $("#INPUT_SMTP_USER").val();$("#INPUT_SMTP_FROM").val(sender_email);}});
	});
	
	/* Trim space while typing in input box */
	$(function(){
		$('input[type="text"]').change(function(){
			this.value = $.trim(this.value);
		});
	});
	
	/* Show Debug Option */
	function DebugOptionLevel(){
		option_level = '<?php echo $_SESSION['optionlevel']; ?>';
		if (option_level == 'DEBUG')
		{
			$('.debug_hidden').show();
		}
	}
	
	DebugOptionLevel();
	QueryTimeZoneList();
	QueryWebPortList();
	QueryLanguageList();
	QueryOptionLevel();
	QueryReportTimeList();
	QueryAccountSetting();
	MgmtRecoveryScript();
	ListRestoreBackup();
	
	/* Query Sql Backup File */
	function ListRestoreBackup(){
		$("#SqlFileTable tbody").empty();
		$("#SqlFileTable tbody").remove();
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'ListRestoreBackup',
			},
			success:function(backup_files)
			{
				$(".default_sql").empty();
				
				var sql_files = $.parseHTML(backup_files);
				$('#sql_files').append(sql_files);
				
				/* Enable Button If there is a SQL file */
				if ($("input[name=select_sql_file]:first").val() != '')
				{			
					$("#RestoreMgmtBtn").removeClass("btn-default").addClass("btn-success");
					$("#RestoreMgmtBtn").prop("disabled", false);
				}
				
				if ($('#SqlFileTable tbody tr').length > 10)
				{
					$("#SqlFileTable").DataTable({
						paging:true,
						ordering:false,
						searching:false,
						bLengthChange:false,
						destroy:true,
						pageLength:10,
						pagingType:"simple_numbers",
						dom: '<"toolbar">frtip'				
					});					
					$('.restore-toolbar').css('height','50px');
				}
				
				$("#RestoreMgmtBtn").click(function(){
					RESTORE_FILE = $("input[name=select_sql_file]:checked").val();
					if (typeof RESTORE_FILE === "undefined"){
						BootstrapDialog.show({
							title: '<?php echo _("Restore Management"); ?>',
							message: '<?php echo _("Please select a sql file to restore the system"); ?>',
							type: BootstrapDialog.TYPE_PRIMARY,
							draggable: true,
							buttons:[{
								label: '<?php echo _("Close"); ?>',
								action: function(dialogRef){
								dialogRef.close();
								}
							}],					
						});
					}
					else{						
						BootstrapDialog.show({
							title: "<?php echo _('Restore the database?'); ?>",
							message: '<div class="form-group"><?php echo _("Confirm the archive password to retore the database"); ?></div>',
							draggable: true,
							buttons:
							[
								{
									label: '<?php echo _('Submit'); ?>',
									cssClass: 'btn-primary',
									action: function(dialogRef)
									{
										$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
										RestoreDataBase();
										dialogRef.close();
									}
								}
							],
							onshow: function(dialogRef) {
								dialogRef.getModalFooter()
									.addClass('form-inline')
									.find('.bootstrap-dialog-footer-buttons')
									.prepend('<input type="text" id="INPUT_SECURITY_CODE" class="form-control" style="width:90px; border:1px solid #406883;" value="" placeholder="<?php echo _('Password'); ?>">&nbsp;');
							},
						});
					}					
				});
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Restore DataBase */
	function RestoreDataBase(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'RestoreDatabase',
				 'RESTORE_FILE':RESTORE_FILE,
				 'SECURITY_CODE':$("#INPUT_SECURITY_CODE").val()
			},
			success:function(JSON)
			{
				if (JSON.Status == false)
				{
					$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
					BootstrapDialog.show({
						title: '<?php echo _("Restore Management"); ?>',
						message: JSON.Msg,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],					
					});
				}
				else
				{
					RENEW_DATA = JSON;
					UpdateAcctUUID();
				}				
			},
			error: function(xhr)
			{
				
			}
		});			
	}
	
	/* Update Account Data To UI database */
	function UpdateAcctUUID(){
		$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'UpdateAcctUUID',
				 'ACCT_UUID':'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'RENEW_DATA':RENEW_DATA
			},
			success:function(JSON)
			{
				BootstrapDialog.show({
						title: '<?php echo _("Restore Management"); ?>',
						message: RENEW_DATA.Msg,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							setTimeout(function () {window.location.href = './MgmtAcct';}, 500);			
						}
					});
			},
			error: function(xhr)
			{
			}
		});	
	}
		
	/* Query Recovery Script */
	function MgmtRecoveryScript(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'MgmtRecoveryScript',
			},
			success:function(recovery_script)
			{
				$('.default').remove();
				
				var list_files =  $.parseHTML(recovery_script); 			
				$('#list_files').append(list_files);
				
				$("#BrowseFileBtn").removeClass("btn-default").addClass("btn-success");
				$("#BrowseFileBtn").prop("disabled", false);
				//$("#BrowseFileBtn").click(function(){$("#UploadFile").click();});
				
				$("#UploadFile").change(function(){
					$("#BrowseFileBtn").html('<i id="BrowseFileText" class="fa fa-circle-o-notch fa-fw"></i> '+$("#UploadFile")[0].files[0].name).button("refresh");				
					$("#FileUploadBtn").removeClass("btn-default").addClass("btn-warning");
					$("#FileUploadBtn").prop("disabled", false);	
				});

				$("#FileUploadBtn").click(function(){FileUpload();});
				
				$("#FileDeleteBtn").click(function(){
					SELECT_FILES = [];
					if ($.fn.dataTable.isDataTable == true)
					{
						var RowCollection = $("#RecoveryScriptTable").dataTable().$("input[name=select_file]:checked", {"page": "all"});
					}
					else
					{
						var RowCollection = $("input[name=select_file]:checked");
					}
					
					RowCollection.each(function(){SELECT_FILES.push($(this).val());});
					DeleteFiles(SELECT_FILES);
				});
				
				$("#checkAll").click(function(){
					$("#RecoveryScriptTable input:checkbox:enabled").prop("checked", this.checked);
				});

				ReloadMgmtRecoveryScript();				
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Reload Mgmt Recovery Script */
	function ReloadMgmtRecoveryScript(){		
		$("#RecoveryScriptTable tbody").empty();
		$("#RecoveryScriptTable tbody").remove();		
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'ReloadMgmtRecoveryScript',
			},
			beforeSend:function(){
				var default_display = $.parseHTML("<table class='default'><tbody><tr><td width='65px' class='TextCenter'><input type='checkbox' name='select_file' value='' disabled ></td><td width='996px' colspan='3'><div> <?php echo _('Loading... '); ?><i class='fa fa-spinner fa-pulse fa-fw'></i></div></td></tr></tbody></table>"); 			
				$('#list_files').append(default_display);
			},
			success:function(data)
			{
				$(".default").empty();
				
				var recovery_script = $($.parseHTML(data)).filter("#RecoveryScriptTable").find("tbody");
				$('#RecoveryScriptTable').append(recovery_script);
				
				$("#BrowseFileBtn").removeClass("btn-default").addClass("btn-success");
				$("#BrowseFileBtn").prop("disabled", false);
				$("#BrowseFileBtn").click(function(){$("#UploadFile").click();});
			
				if ($('#RecoveryScriptTable tbody tr').length > 10)
				{
					$("#RecoveryScriptTable").DataTable({
						paging:true,
						ordering:false,
						searching:false,
						bLengthChange:false,
						destroy:true,
						pageLength:10,
						pagingType:"simple_numbers"						
					});
				}
				else
				{
					if ($.fn.DataTable.isDataTable('#RecoveryScriptTable')) 
					{
						$("#RecoveryScriptTable").DataTable({ordering:false,destroy:true}).destroy();
					}
				}

				if ($('#RecoveryScriptTable td input:checkbox:first').val() == '')
				{
					$('#FileDeleteBtn').removeClass('btn-danger').addClass('btn-default');
					$('#FileDeleteBtn').prop('disabled', true);
					$('#checkAll').prop('disabled', true);
				}
				else
				{
					$('#FileDeleteBtn').removeClass('btn-default').addClass('btn-danger');
					$('#FileDeleteBtn').prop('disabled', false);
					$('#checkAll').prop('disabled', false);
				}
				
				$("#BrowseFileBtn").removeClass("btn-default").addClass("btn-success");
				$("#BrowseFileBtn").prop("disabled", false);
				
				$('#FileUploadBtn').removeClass('btn-warning').addClass('btn-default');
				$('#FileUploadBtn').prop('disabled', true);
				
				$("#checkAll").click(function() {$("#RecoveryScriptDiv input:checkbox:enabled").prop("checked", this.checked);});				
				$('#checkAll').prop('checked', false);
			}
		});
	}
	
	/* File Upload */
	function FileUpload()
	{
		var file = document.getElementById('UploadFile').files[0];
		MaxUploadSize = <?php echo Misc::convert_to_bytes(ini_get("upload_max_filesize")); ?>;
	
		$("#BrowseFileText").addClass('fa-spin');
		
		if (file.size < MaxUploadSize)
		{		
			var form_data = new FormData();
			form_data.append("ACTION",'FileUpload');
			form_data.append("UploadFile", file);
		
			$.ajax({
				url: '_include/_exec/mgmt_service.php',
				type:"POST",
				data:form_data,
				contentType: false,
				cache: false,
				processData: false,
				beforeSend:function(){
					$("#BrowseFileBtn").removeClass("btn-success").addClass("btn-default");
					$("#BrowseFileBtn").prop("disabled", true);
					
					$("#FileUploadBtn").removeClass("btn-warning").addClass("btn-default");
					$("#FileUploadBtn").prop("disabled", true);
					
					$("#FileDeleteBtn").removeClass("btn-danger").addClass("btn-default");
					$("#FileDeleteBtn").prop("disabled", true);
				},
				success:function(data)
				{
					var obj = jQuery.parseJSON(data);
					ReloadMgmtRecoveryScript();
					if (obj.status == false)
					{
						BootstrapDialog.show({
							title: '<?php echo _("File Upload Message"); ?>',
							message: obj.reason,
							type: BootstrapDialog.TYPE_PRIMARY,
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
				complete: function()
				{				
					$("#BrowseFileBtn").html('Browse...').button("refresh");
				}
			});
		}
		else
		{
			<?php
				$string = array( "Max file upload size is %1%B", ini_get("upload_max_filesize") );
			?>
			$("#BrowseFileBtn").html('Browse...').button("refresh");
			BootstrapDialog.show({
				title: '<?php echo _("File Upload Message"); ?>',
				message: '<?php echo call_user_func_array('translate', $string); ?>',
				type: BootstrapDialog.TYPE_PRIMARY,
				draggable: true,
				buttons:[{
					label: '<?php echo _("Close"); ?>',
					action: function(dialogRef){
					dialogRef.close();
					}
				}],					
			});
		}
	}
	
	/* Delete Files */
	function DeleteFiles(SELECT_FILES)
	{
		$.ajax({
			url: '_include/_exec/mgmt_service.php',
			type:"POST",
			data:{
				 'ACTION'	   :'DeleteFiles',
				 'SELECT_FILES':SELECT_FILES
			},			
			beforeSend:function(){
				
			},   
			success:function(data)
			{
				ReloadMgmtRecoveryScript();
			}
		});
	}
	
	/* Query Account Setting */
	function QueryAccountSetting() {		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'	:'QueryAccountSetting',
				 'ACCT_UUID':'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
			},
			success:function(jso)
			{
				$("#HOST_ACCESS_CODE").val(jso.ACCT_DATA.identity);
	
				if (jso.ACCT_DATA.smtp_settings != undefined)
				{
					$("#SaveSMTPSetting").text("Update");
					
					$('#Replication').prop('disabled', false);
					$('#Recovery').prop('disabled', false);
					$('#DailyReport').prop('disabled', false);
					$('#DailyBackup').prop('disabled', false);
					
					$('#SubmitReportType').prop('disabled', false);
					$('#SubmitReportType').removeClass('btn-default').addClass('btn-primary');
					
					$('#DeleteSMTPSetting').prop('disabled', false);
					$('#DeleteSMTPSetting').removeClass('btn-default').addClass('btn-primary');

					$("#INPUT_SMTP_HOST").val(jso.ACCT_DATA.smtp_settings.SMTPHost);
					$("#INPUT_SMTP_TYPE").val(jso.ACCT_DATA.smtp_settings.SMTPType);
					$("#INPUT_SMTP_PORT").val(jso.ACCT_DATA.smtp_settings.SMTPPort);
					$("#INPUT_SMTP_USER").val(jso.ACCT_DATA.smtp_settings.SMTPUser);
					$("#INPUT_SMTP_FROM").val(jso.ACCT_DATA.smtp_settings.SMTPFrom);
					$("#INPUT_SMTP_TO").val(jso.ACCT_DATA.smtp_settings.SMTPTo);

					$("#DailyReportTime").prop('disabled', false).css('pointer-events', 'pointer');
					$("#DailyReportCalendarPoint").css('pointer-events', 'auto');
						
					$("#DailyBackupTime").prop('disabled', false).css('pointer-events', 'pointer');
					$("#DailyBackupCalendarPoint").css('pointer-events', 'auto');
					
					if (jso.ACCT_DATA.notification_time != undefined)
					{
						$("#DailyReportTime").val(jso.ACCT_DATA.notification_time.DailyReport);
						$("#DailyReportRoutineTime").val(jso.ACCT_DATA.notification_time.DailyReport);
						$("#DailyBackupTime").val(jso.ACCT_DATA.notification_time.DailyBackup);
						$("#DailyBackupRoutineTime").val(jso.ACCT_DATA.notification_time.DailyBackup);
					}
					
					$.each(jso.ACCT_DATA.notification_type, function( index, value ) {
						$('#'+value+'').prop('checked', true);
					});					
					$('.selectpicker').selectpicker('refresh');					
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query TimeZone List */
	function QueryTimeZoneList(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'TimeZoneList'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#USER_SET_TIMEZONE').append(new Option(value,key, true, false));
					});
								
					/* SELECT SET USER TIMEZONE */
					$("#USER_SET_TIMEZONE").val('<?php echo $_SESSION['timezone']; ?>');
					$('.selectpicker').selectpicker('refresh');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query WebPort List */
	function QueryWebPortList(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'WebPortList'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#USER_SET_WEBPORT').append(new Option(value,key, true, false));
					});
								
					/* SELECT SET USER TIMEZONE */
					$("#USER_SET_WEBPORT").val('<?php echo $_SESSION['webport']; ?>');
					$('.selectpicker').selectpicker('refresh');
				}
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Language List */
	function QueryLanguageList(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'LanguageList'
			},
			success:function(jso)
			{
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						$('#USER_SET_LANGUAGE').append(new Option(value,key, true, false));
					});
					
					/* SELECT SET USER LANGUAGE */
					$("#USER_SET_LANGUAGE").val('<?php echo $_SESSION['language']; ?>');
					
					/* SET AS DISABLE */
					//$('#USER_SET_LANGUAGE').prop('disabled', true);
					
					$('.selectpicker').selectpicker('refresh');
				}			
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Query Option Level */
	function QueryOptionLevel(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION':'OptionLevel'
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					$('#USER_SET_LEVELS').append(new Option(value,key, true, false));
				});
				
				/* SELECT SET USER LANGUAGE */
				$("#USER_SET_LEVELS").val('<?php echo $_SESSION['optionlevel']; ?>');
							
				$('.selectpicker').selectpicker('refresh');					
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Update Timezone And Language */
	function UpdateTimezoneAndLanguage(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'	 	:'UpdateTimezoneAndLanguage',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'ACCT_ZONE' 	:$("#USER_SET_TIMEZONE").val(),
				 'ACCT_PORT' 	:$("#USER_SET_WEBPORT").val(),
				 'ACCT_LANG'	:$("#USER_SET_LANGUAGE").val(),
				 'OPTION_LEVEL' :$("#USER_SET_LEVELS").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Setting Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
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
			
        	}
		});
	}
	
	/* Update Web Port */
	function UpdateWebPort(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'		:'UpdateWebPort',
				 'WEB_PORT' 	:$("#USER_SET_WEBPORT").val()
			},			
			success:function(jso)
			{
				
			},
			error: function(xhr)
			{
			
        	}			
		});
	}

	/* Change Password */
	function ChangePassword(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'	 	   :'ChangePassword',
				 'ACCT_UUID' 	   :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'INPUT_PASSWORD'  :$("#INPUT_PASSWORD").val(),
				 'REPEAT_PASSWORD' :$("#INPUT_REPEAT_PASSWORD").val()
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Authorization Message"); ?>',
						message: jso.Msg,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							setTimeout(function () {window.location.href = './index.php?lang='+$('#USER_SET_LANGUAGE').val();}, 500);			
						}
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Authorization Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
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
				
			}
		});
	}
		
	/* Take Xray */
	function TakeXray(SELECT_TYPE){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'	 	   :'TakeXray',
				 'ACCT_UUID' 	   :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'XRAY_PATH'	   :'<?php echo str_replace('\\','/',str_replace('index.php','_include/_inc/_xray/',$_SERVER['SCRIPT_FILENAME'])); ?>',
				 'XRAY_TYPE'	   :SELECT_TYPE,
				 'SERV_UUID'	   :null
			},
			success:function(xray)
			{
				toggle_mgmt('enable');				
				if (xray != false)
				{
					window.location.href = './_include/_inc/_xray/'+xray;					
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Select Type of X-Ray"); ?>',
						message: '<?php echo _("Cannot get Transport server\'s X-Ray."); ?>',
						type: BootstrapDialog.TYPE_DANGER,
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
				toggle_mgmt('enable');
			}
		});
	}
	
	/* ReGen Host Access Code*/
	function ReGenHostAccessCode()
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'	 	   :'ReGenHostAccessCode',
				 'ACCT_UUID' 	  :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
			},
			success:function(AccessCode)
			{
				$("#HOST_ACCESS_CODE").val(AccessCode);
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Copy Account UUID */
	function CopyAccountUUID()
	{
		var copyAcctUUID = document.getElementById("ACCT_UUID");
			copyAcctUUID.select();
			document.execCommand("Copy");
			BootstrapDialog.show({title: "<?php echo _("Information"); ?>",message: "<?php echo _("Account UUID copied!"); ?>", type: BootstrapDialog.TYPE_PRIMARY, draggable: true, buttons:[{label: "<?php echo _("Close"); ?>", action: function(){$.each(BootstrapDialog.dialogs, function(id, dialog){dialog.close();});}}]});
			window.getSelection().removeAllRanges();
			document.selection.empty();
	}
	
	/* TOGGLE MGMT */
	function toggle_mgmt($toggle)
	{
		if ($toggle == 'disable')
		{
			$('#BackToPortal').prop('disabled', true);
			
			$('#CheckAndSubmit').prop('disabled', true);
			$('#CheckAndSubmit').removeClass('btn-primary').addClass('btn-default');
			
			$('#SelectXray').prop('disabled', true);
			$('#SelectXray').removeClass('btn-primary').addClass('btn-default');
			
			$('#TakeXray').prop('disabled', true);
			$('#TakeXray').removeClass('btn-primary').addClass('btn-default');
			
			$('#CloseTakeXray').prop('disabled', true);
			
			$('#UpdateSensorSpin').addClass('fa-spin');		
		}
		else
		{	
			$('#BackToPortal').prop('disabled', false);
				
			$('#CheckAndSubmit').prop('disabled', false);
			$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
			
			$('#SelectXray').prop('disabled', false);
			$('#SelectXray').removeClass('btn-default').addClass('btn-primary');			
			
			$('#TakeXray').prop('disabled', false);
			$('#TakeXray').removeClass('btn-default').addClass('btn-primary');
			
			$('#CloseTakeXray').prop('disabled', false);
						
			$('#UpdateSensorSpin').removeClass('fa-spin');			
		}		
	}
	
	/* Submit Trigger */
	$(function(){
		$("#BackToPortal").click(function(){
			window.location.href = './';
		})
		
		$("#SelectXray").click(function(){
			var $CheckboxHTML = $('<div></div>');
				$CheckboxHTML.append('<input type="checkbox" name="XrayType" value="server" checked> <?php echo _("Transport Server"); ?><br>');
				$CheckboxHTML.append('<input type="checkbox" name="XrayType" value="host"> <?php echo _("Packer for Windows/Linux"); ?><br>');
				$CheckboxHTML.append('<input type="checkbox" name="XrayType" value="mgmt" checked> <?php echo _("Management Server"); ?><br>');
			
			BootstrapDialog.show
			({
				title: '<?php echo _("Select Type of X-Ray"); ?>',
				message: $CheckboxHTML,
				draggable: true,
				closable: true,
				buttons: 
				[
					{
						id: 'CloseTakeXray',
						label: '<?php echo _("Close"); ?>',
						cssClass: 'pull-left btn-default',
						action: function(dialogRef)
						{
							dialogRef.close();
						}
					},
					{
						id: 'TakeXray',
						label: '<i id="UpdateSensorSpin" class="fa fa-cog fa-lg fa-fw"></i><?php echo _("Take X-Ray"); ?>',
						cssClass: 'btn-primary',
						action: function(dialogRef)
						{
							dialogRef.setClosable(false);
							
							var XrayType = $("input[name=XrayType]:checked").map(function () {return this.value;}).get().join(",");
							
							if (XrayType != '')
							{
								toggle_mgmt('disable');
								TakeXray(XrayType);
							}
							else
							{
								BootstrapDialog.show({
									title: '<?php echo _("Select Type of X-Ray"); ?>',
									message: '<?php echo _("Please select a X-Ray type."); ?>',
									type: BootstrapDialog.TYPE_DANGER,
									draggable: true,
									buttons:[{
										label: '<?php echo _("Close"); ?>',
										action: function(dialogRef){
										dialogRef.close();
										}
									}],
								});
							}
						}
					}										
				]
			});
		})
		
		$("#CheckAndSubmit").click(function(){
			UpdateTimezoneAndLanguage();
			UpdateWebPort();
			ChangePassword();
		})

		$("#ReGenCode").click(function(){
			ReGenHostAccessCode();
		})
		
		$("#CopyAcctUUID").click(function(){
			CopyAccountUUID();
		})		
	});
});
</script>
<style>
	.indentedtext
	{
		margin-left:0.8em;
	}
	.form-control-inline{
		height:28px;
		width:120px;		
		display:inline;
	}
</style>
<div id='container'>
	<div id='wrapper_block'>
		<div id='form_block' style='min-width:1120px'>
			<h3><i class="fa fa-cog fa-fw"></i><?php echo _("Settings"); ?></h3>
				<div class="tab">
					<button class="tablinks" onclick="openSetting(event, 'settings')" id="defaultOpenTab"><i class="fa fa-wrench"></i> <?php echo _("General"); ?></button>
					<button class="tablinks" onclick="openSetting(event, 'notifications')" id=""><i class="fa fa-sticky-note"></i> <?php echo _("Notifications"); ?></button>
					<button class="tablinks" onclick="openSetting(event, 'license')" id=""><i class="fa fa-id-card"></i> <?php echo _("Licensing"); ?></button>
					<button class="tablinks" onclick="openSetting(event, 'filemgmt')" id=""><i class="fa fa-files-o"></i> <?php echo _("File Management"); ?></button>
					<button class="tablinks debug_hidden" onclick="openSetting(event, 'restoremgmt')" id=""><i class="fa fa-refresh"></i> <?php echo _("Restore Management"); ?></button>
				</div>
						
				<!-- User Settings -->
				<div id='settings' class='tabcontent'>
					<div class='form-inline'>
						<label for='comment' style='width:500px;'><i class='fa fa-globe'></i> <?php echo _("Time Zone"); ?></label>
						<label for='comment' style='width:500px;'><i class='fa fa-at'></i> <?php echo _("Web Port"); ?></label>
					</div>
					<div class='form-group has-primary'>				
						<select id='USER_SET_TIMEZONE' class='selectpicker' data-width='500px'></select>
						<select id='USER_SET_WEBPORT'  class='selectpicker' data-width='500px'></select>
					</div>

					<div class='form-inline'>
						<label for='comment' style='width:500px;'><i class='fa fa-language'></i> <?php echo _("Language"); ?></label>
						<label for='comment' style='width:360px;'><i class="fa fa-universal-access" aria-hidden="true"></i> <?php echo _("UI Mode"); ?></label>
						<label for='comment' style='width:140px;'><i class='fa fa-random'></i> <?php echo _("Security Code"); ?></label>
					</div>
					<div class='form-inline form-group'>
						<select id='USER_SET_LANGUAGE' class='selectpicker' data-width='500px'></select>
						<select id='USER_SET_LEVELS'   class='selectpicker' data-width='360px'></select>
						<input type='text' id='HOST_ACCESS_CODE' class='form-control' value='' style='width:98px; border-radius:0px; border-right:none;' readonly><button id='ReGenCode' class='btn btn-success'><i class='fa fa-refresh'></i></button>
					</div>
				
					<div class='form-inline'>
						<label for='comment' style='width:500px;'><i class='fa fa-lock'></i> <?php echo _("Password"); ?></label>
						<label for='comment' style='width:500px;'><i class='fa fa-id-card-o'></i> <?php echo _("Account UUID"); ?></label>
					</div>
					<div class='form-inline form-group'>
						<input type='password' id='INPUT_PASSWORD' class='form-control pwd' value='' placeholder='<?php echo _('New Password'); ?>' style='width:249px; border-radius:0px;'>
						<div class="input-group"><input type='password' id='INPUT_REPEAT_PASSWORD' class='form-control re-pwd' value='' placeholder='<?php echo _('Re-type New Password'); ?>' style='width:205px; border-radius:0px;'><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>
						<input type='text' 	   id='ACCT_UUID' class='form-control' value='<?php echo $_SESSION['admin']['ACCT_UUID']; ?>' placeholder='<?php echo _('Account UUID'); ?>' style='width:465px; border-radius:0px; border-right:none;' readonly><button id='CopyAcctUUID' class='btn btn-primary'><i class='fa fa-clipboard'></i></button>	
					</div>
				
					<div class='btn-toolbar' style='width:1010px'>
						<button id='BackToPortal' 	class='btn btn-default pull-left btn-lg'><?php echo _("Back"); ?></button>
						<button id='CheckAndSubmit' class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>
						<button id='SelectXray'		class='btn btn-default pull-right btn-lg' disabled><?php echo _("X-Ray"); ?></button>					
					</div>
				</div>
				<!-- User Settings -->
				
				<!-- Notifications Settings -->
				<div id='notifications' class='tabcontent'>
					<div class='left_tab'>
						<button class='left_tablinks notify_links' onclick='openNotificationLeftTab(event, "NotificationSummary")' id='defaultNotificationTab'><i class='fa fa-address-card'></i> <?php echo _("Preferences"); ?></button>
						<button class='left_tablinks notify_links' onclick='openNotificationLeftTab(event, "SMTPSettings")' id=''><i class='fa fa-envelope'></i> <?php echo _("SMTP Settings"); ?></button>
					</div>
					
					<div id="NotificationSummary" class="left_tabcontent notify_content">
						<div class="form-group">
							<h4><i class="fa fa-address-card"></i> <?php echo _("Preferences"); ?></h4>
							<div style="padding-top:8px; padding-bottom:5px;">
								<i class="fa fa-address-card-o" style="font-size:90px;"></i>
							</div>
							
							<div id="form-inline" style='width:886px;'>
								<div class="funkyradio">
									<div class="funkyradio-primary"><input type="checkbox" name="Notification" id="Replication" value="Replication" disabled /><label for="Replication"><?php echo _("Replication Notification"); ?></label><div class="indentedtext"><?php echo _("A message will be sent whenever a replication process starts."); ?></div></div>
									<div style="height:5px;"></div>
									<div class="funkyradio-info"><input type="checkbox" name="Notification" id="Recovery" value="Recovery" disabled /><label for="Recovery"><?php echo _("Recovery Notification"); ?></label><div class="indentedtext"><?php echo _("A message will be sent whenever a new recovery process completes."); ?></div></div>
									<div style="height:5px;"></div>
									<div class="funkyradio-success"><input type="checkbox" name="Notification" id="DailyReport" value="DailyReport" disabled onclick="click_onchange('DailyReport');" /><label for="DailyReport"><?php echo _("Daily Report"); ?></label><div class="indentedtext"><?php echo _("A message with the activity from the last 24hs will be sent every day at the specified time."); ?></div></div>
									<div style="height:5px;"></div>
									<div class="funkyradio-warning"><input type="checkbox" name="Notification" id="DailyBackup" value="DailyBackup" disabled onclick="click_onchange('DailyBackup');" /><label for="DailyBackup"><?php echo _("Daily Management Backup"); ?></label><div class="indentedtext"><?php echo _("A message with the management database backup from the last 24hs will be sent every day at the specified time."); ?></div></div>
									
									<div style="position:absolute; top:428px; right:30px;">
										<div class="input-group date form_time" data-date="" data-date-format="hh:ii:00" data-link-field="DailyReportRoutineTime" data-link-format="hh:ii:00" style=" width:120px;">
											<input class="form-control jquery_time" id="DailyReportTime" size="16" value="" type="text" placeholder="<?php _('Run Now'); ?>" readonly disabled style="border:0; box-shadow:none; border-radius:0px; border-left:1px solid #cccccc; border-right: 1px solid #cccccc; height:100%;">					
											<span class="input-group-addon" id="DailyReportCalendarPoint" style="border-radius:0px; border-style:none; height:100%; pointer-events:none"><span class="glyphicon glyphicon-time fa fa-calendar-check-o"></span></span>				
										</div>
										<input type="hidden" id="DailyReportRoutineTime" value="" />
									</div>
									
									<div style="position:absolute; top:502px; right:30px;">
										<div class="input-group date form_time" data-date="" data-date-format="hh:ii:00" data-link-field="DailyBackupRoutineTime" data-link-format="hh:ii:00" style=" width:120px;">
											<input class="form-control jquery_time" id="DailyBackupTime" size="16" value="" type="text" placeholder="<?php _('Run Now'); ?>" readonly disabled style="border:0; box-shadow:none; border-radius:0px; border-left:1px solid #cccccc; border-right: 1px solid #cccccc; height:100%;">					
											<span class="input-group-addon" id="DailyBackupCalendarPoint" style="border-radius:0px; border-style:none; height:100%; pointer-events:none"><span class="glyphicon glyphicon-time fa fa-calendar-check-o"></span></span>				
										</div>
										<input type="hidden" id="DailyBackupRoutineTime" value="" />
									</div>
								</div>
							</div>
						</div>
						<div class="btn-toolbar" style='position:absolute; top:560px; width:890px;'>
							<button id='SubmitReportType' class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
						</div>
					</div>
					
					<div id="SMTPSettings" class="left_tabcontent notify_content">
						<div class="form-group">
							<h4><i class="fa fa-envelope"></i> <?php echo _("SMTP Settings"); ?></h4>
							<div style="padding-top:10px; padding-bottom:25px;">
								<i class="fa fa-envelope-o" style="font-size:90px;"></i>
							</div>
							
							<div class="form-inline">
								<label for='comment' style='width:442px;'><?php echo _("SMTP Address"); ?></label>
								<label for='comment' style='width:218px;'><?php echo _("SMTP Secure"); ?></label>
								<label for='comment' style='width:218px;'><?php echo _("SMTP Port"); ?></label>
							</div>
							
							<div class="form-inline form-group">
								<input type='text' id='INPUT_SMTP_HOST' class='form-control' value='' placeholder='<?php echo _('SMTP Address'); ?>' style='width:442px;'>	
								<select type='text' id='INPUT_SMTP_TYPE' class='selectpicker' value='' placeholder='<?php echo _('SMTP Type'); ?>' style='width:218px;' onchange='GetSMTPType(this)';>
									<option value="tls"><?php echo _("TLS"); ?></option>
									<option value="ssl"><?php echo _("SSL"); ?></option>
									<option value="na"><?php echo _("Normal"); ?></option>
								</select>
								<input type='text' id='INPUT_SMTP_PORT' class='form-control' value='587' placeholder='<?php echo _('SMTP Port'); ?>' style='width:218px;'>								
							</div>
							<div style="height:5px;"></div>
							
							<div class="form-inline">
								<label for='comment' style='width:442px;'><?php echo _("Username"); ?></label>
								<label for='comment' style='width:442px;'><?php echo _("Password"); ?></label>
							</div>							
							<div class="form-inline form-group">
								<input type='text' 		id='INPUT_SMTP_USER' 	class='form-control' value='' placeholder='<?php echo _('Username'); ?>' style='width:442px;'>
								<div class="input-group"><input type='password'	id='INPUT_SMTP_PASS' class='form-control smtp-pwd' value='' placeholder='<?php echo _('Password'); ?>' style='width:402px;'><span class="input-group-addon reveal" style="cursor: pointer;"><i class="glyphicon glyphicon-eye-open"></i></span></div>						
							</div>
							<div style="height:5px;"></div>
							
							<div class="form-inline">
								<label for='comment' style='width:442px;'><?php echo _("Mail From Address"); ?></label>
								<label for='comment' style='width:442px;'><?php echo _("Mail To Address"); ?></label>
							</div>							

							<div class="form-inline form-group">
								<input type='text' id='INPUT_SMTP_FROM'	class='form-control' value='' placeholder='<?php echo _('Mail From Address'); ?>' style='width:442px;'>
								<input type='text' id='INPUT_SMTP_TO' 	class='form-control' value='' placeholder='<?php echo _('Mail To Address'); ?>' style='width:442px;'>
							</div>							
						</div>
						
						<div class="btn-toolbar" style='width:893px'>
							<button id='DeleteSMTPSetting' 	class='btn btn-default pull-left btn-lg' disabled style="position:relative; top:15px;"><?php echo _("Delete"); ?></button>
							<button id='SaveSMTPSetting' 	class='btn btn-default pull-right btn-lg' disabled style="position:relative; top:15px;"><?php echo _("Save"); ?></button>							
							<button id='VerifySMTPSetting' 	class='btn btn-default pull-right btn-lg' disabled style="position:relative; top:15px;"><?php echo _("Verify"); ?></button>
						</div>
					</div>
				</div>
				<!-- Notifications Settings -->
				
				<!-- License Settings -->
				<div id="license" class="tabcontent">
					<div class="left_tab">
						<button class="left_tablinks license_links" onclick="openSettingLeftTab(event, 'ListLicense')"		  	id="defaultLicenseTab"><i class="fa fa-list" aria-hidden="true"></i> <?php echo _("Licenses"); ?></button>
						<button class="left_tablinks license_links" onclick="openSettingLeftTab(event, 'LicenseHistory')"   	id=""><i class="fa fa-list-ol" aria-hidden="true"></i> <?php echo _("Activity"); ?></button>
						<button class="left_tablinks license_links" onclick="openSettingLeftTab(event, 'OnlineActivation')"   	id=""><i class="fa fa-file-text" aria-hidden="true"></i> <?php echo _("Activation"); ?></button>
						<!--<button class="left_tablinks license_links" onclick="openSettingLeftTab(event, 'OnfflineActivation')" 	id=""><i class="fa fa-id-card" aria-hidden="true"></i> <?php echo _("Offline Activation"); ?></button> -->
					</div>
					
					<div id="ListLicense" class="left_tabcontent license_content">
						<div class="form-group">
							<h4><i class="fa fa-list" aria-hidden="true"></i> <?php echo _("Licenses"); ?></h4>
							<div id="list_license" class="license-divTable"></div>
						</div>
					</div>
					<div id="LicenseHistory" class="left_tabcontent license_content">
						<div class="form-group">
							<h4><i class="fa fa-list-ol" aria-hidden="true"></i> <?php echo _("Activity"); ?></h4>
							<div id="list_license_history" class="license-divTable"></div>
						</div>
					</div>
					<div id="OnlineActivation" class="left_tabcontent license_content">
						<div class="form-group">
							<h4><i class="fa fa-file-text" aria-hidden="true"></i> <?php echo _("Activation"); ?></h4>
							<div style="padding-top:10px; padding-bottom:30px;">
								<img src="data:image/png;base64,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" />
							</div>
							<div class="form-inline form-group">
								<input type='text' id='INPUT_NAME'  class='form-control' value='' placeholder='<?php echo _('Name'); ?>'  style='width:445px;'>
								<input type='text' id='INPUT_EMAIL' class='form-control' value='' placeholder='<?php echo _('Email'); ?>' style='width:445px;'>								
							</div>
							<input type='text' id='INPUT_KEY' class='form-control' value='' placeholder='<?php echo _('Activation Key'); ?>' style='width:892px;'>
						</div>						
						<div class="btn-toolbar" style='width:896px'>
							<button id='SubmitOnlineLicenseActivation' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Activate"); ?></button>							
						</div>						
					</div>
				</div>
				<!-- License Settings -->	
				
				<!-- File Management -->
				<div id='filemgmt' class='tabcontent' style="width:1030px;">
					<div id="list_files">
						<div class='default'>
							<table>
							<thead>
								<tr>
									<th width="65px" class="TextCenter"><input type="checkbox" id="checkAll" /></th>
									<th width="720px"><?php echo _("Name"); ?></th>
									<th width="165px"><?php echo _("Date Modified"); ?></th>
									<th width="80px"><?php echo _("Size"); ?></th>
								</tr>
							</thead>
							<tbody>
								<tr>
									<td width="65px" class="TextCenter"><input type="checkbox" name="select_file" value="" disabled ></td>
									<td width="970px" colspan="3"><div> <?php echo _("Loading... "); ?><i class="fa fa-spinner fa-pulse fa-fw"></i></div></td>
								</tr>
							</tbody>
							</table>
						</div>		
					</div>
					
					<div style='border: 1px solid #EEEEEC; height:8px; width: 1px; position: relative;'></div>
					<div class="btn-toolbar">
						<button id='BrowseFileBtn' class='btn btn-default pull-right' disabled><?php echo _("Browse..."); ?></button>	
						<button id='FileUploadBtn' class='btn btn-default pull-right' disabled><?php echo _("Upload"); ?></button>
						<button id='FileDeleteBtn' class='btn btn-default pull-left' disabled><?php echo _("Delete"); ?></button>
					</div>
				</div>
				<!-- File Management -->
				
				<!-- Restore Management -->
				<div id='restoremgmt' class='tabcontent' style="width:1030px;">
					<div id="sql_files">
						<div class='default_sql'>
							<table>
							<thead>
								<tr>
									<th width="65px" class="TextCenter">-</th>
									<th width="720px"><?php echo _("Name"); ?></th>
									<th width="165px"><?php echo _("Date Modified"); ?></th>
									<th width="80px"><?php echo _("Size"); ?></th>
								</tr>
							</thead>
							<tbody>
								<tr>
									<td width="65px" class="TextCenter">-</td>
									<td width="970px" colspan="3"><div> <?php echo _("Loading... "); ?><i class="fa fa-spinner fa-pulse fa-fw"></i></div></td>
								</tr>
							</tbody>
							</table>
						</div>		
					</div>
					
					<div class="restore-toolbar" style='border: 1px solid #EEEEEC; height:8px; width: 1px; position: relative;'></div>
					<div class="btn-toolbar">
						<button id='RestoreMgmtBtn' class='btn btn-default pull-right' disabled><?php echo _("Restore"); ?></button>	
					</div>
				</div>
				<!-- Restore Management -->
		</div> <!-- id: form_block-->		
	</div> <!-- id: wrapper_block-->
</div>

<script>
	/* CHANCE INPUT KEY TP UPPER CASE */
	$(function(){
		$('#INPUT_KEY').keyup(function() {
			this.value = this.value.toLocaleUpperCase();
			});
	});

	/* QUERY LICENSE */
	QueryLicense();

	/* Open Setting Tabs */
	function openSetting(evt, tabName){
		var i, tabcontent, tablinks;

		tabcontent = document.getElementsByClassName('tabcontent');
		for (i = 0; i < tabcontent.length; i++){
			tabcontent[i].style.display = 'none';
		}

		tablinks = document.getElementsByClassName('tablinks');
		for (i = 0; i < tablinks.length; i++){
			tablinks[i].className = tablinks[i].className.replace(' active', '');
		}

		document.getElementById(tabName).style.display = 'block';
		evt.currentTarget.className += ' active';
	}
	document.getElementById("defaultOpenTab").click();
	
	/* Open Setting Left Tabs */
	function openNotificationLeftTab(evt, tabName){
		var i, notify_content, notify_links;

		notify_content = document.getElementsByClassName('notify_content');
		for (i = 0; i < notify_content.length; i++){
			notify_content[i].style.display = 'none';
		}

		notify_links = document.getElementsByClassName('notify_links');
		for (i = 0; i < notify_links.length; i++){
			notify_links[i].className = notify_links[i].className.replace(' active', '');
		}

		document.getElementById(tabName).style.display = 'block';
		evt.currentTarget.className += ' active';
	}
	document.getElementById("defaultNotificationTab").click();
	
	/* Open Setting Left Tabs */
	function openSettingLeftTab(evt, tabName){
		var i, license_content, license_links;

		license_content = document.getElementsByClassName('license_content');
		for (i = 0; i < license_content.length; i++){
			license_content[i].style.display = 'none';
		}

		license_links = document.getElementsByClassName('license_links');
		for (i = 0; i < license_links.length; i++){
			license_links[i].className = license_links[i].className.replace(' active', '');
		}

		document.getElementById(tabName).style.display = 'block';
		evt.currentTarget.className += ' active';
	}
	document.getElementById("defaultLicenseTab").click();
	
	/* Query License */
	function QueryLicense(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'QueryLicense'
			},
			success:function(data)
			{
				/* RESET LICENSE BODY */
				$('#license_body').remove();
					
				/* RESET LICENSE BODY */
				$('#license_history').remove();
				
				/* LISE LICENSE */
				var list_license =  $($.parseHTML(data)).filter("#license_body");
				$('#list_license').append(list_license);				
				license_body_count = $('#license_body tbody tr').length;
				if (license_body_count > 10)
				{
					$("#license_body").DataTable({
						paging:true,	
						ordering:false,
						searching:false,
						bLengthChange:false,
						pageLength:10});
				}			
						
				/* LISE LICENSE HISTORY */
				var list_license_history =  $($.parseHTML(data)).filter("#license_history"); 			
				$('#list_license_history').append(list_license_history);				
				license_history_count = $('#license_history tbody tr').length;				
				if (license_history_count > 10)
				{
					$("#license_history").DataTable({
						paging:true,					
						ordering:false,
						searching:false,
						bLengthChange:false,
						pageLength:10});
				}
				
				/* QUERY LICENSE */
				$("#ListLicense").on("click", "div.QueryLicense", function(e){
					var LICENSE_UUID = $(this).attr('data-license');  //use  .attr() for accessing attibutes
					QueryPackageInfo(LICENSE_UUID);	
				});
				
				/* ACTIVE LICENSE */
				$("#ListLicense").on("click", "div.ActiveLicense", function(e){
					var LICENSE_UUID = $(this).attr('data-license');  //use  .attr() for accessing attibutes
					OfflineActivationInput(LICENSE_UUID);
				});
				
				/* REMOVE LICENSE */
				$("#ListLicense").on("click", "div.RemoveLicense", function(e){
					var LICENSE_UUID = $(this).attr('data-license');  //use  .attr() for accessing attibutes
					RemoveLicense(LICENSE_UUID);
				});

				/* QUERY LICENSE HISTORY */
				$("#LicenseHistory").on("click", "div.QueryLicenseHistory", function(e){
					var MACHINE_UUID = $(this).attr('data-machine_id');  //use  .attr() for accessing attibutes
					QueryLicenseHistory(MACHINE_UUID);		
				});
			},
			error: function(xhr)
			{

			}
		});
	}
	
	/* Reload Query License */
	function ReloadQueryLicense(){
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'QueryLicense'
			},
			success:function(data)
			{
				/* RESET LICENSE KEYS */
				$('#license_body').DataTable().destroy();
				$('#license_body').remove();
				
				/* RESET LICENSE HISTORY */
				$('#license_history').DataTable().destroy();
				$('#license_history').remove();
				
				/* LISE LICENSE */
				var list_license =  $($.parseHTML(data)).filter("#license_body");	
				$('#list_license').append(list_license);
				license_body_count = $('#license_body tbody tr').length;
				if (license_body_count > 10)
				{
					$("#license_body").DataTable({
						paging:true,					
						ordering:false,
						searching:false,
						bLengthChange:false,
						pageLength:10});
				}
				
				/* LISE LICENSE HISTORY */
				var list_license_history =  $($.parseHTML(data)).filter("#license_history"); 			
				$('#list_license_history').append(list_license_history);
				license_history_count = $('#license_history tbody tr').length;				
				if (license_history_count > 10)
				{
					$("#license_history").DataTable({
						paging:true,					
						ordering:false,
						searching:false,
						bLengthChange:false,
						pageLength:10});
				}
			},
			error: function(xhr)
			{

			}
		});		
	}

	/* Query Package Info By License Key */
	function QueryPackageInfo(LICENSE_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'QueryPackageInfo',				
				 'LICENSE_KEY' 	:LICENSE_UUID
			},
			success:function(jso)
			{
				if (jso.status == true)				
				{
					if((navigator.userAgent.indexOf("Edge") != -1))
					{					
						BootstrapDialog.show({
							title: '<?php echo _("Package Information"); ?>',
							cssClass: 'license-dialog',
							message: jso.msg,
							type: BootstrapDialog.TYPE_PRIMARY,
							draggable: true,
							onhide: function(dialogRef){
								//setTimeout(function () {window.location.href = 'MgmtAcct';}, 500);
							}
						});
					}
					else
					{					
						var blob = new Blob([jso.string], {type: "text/plain;charset=utf-8"});
						saveAs(blob, jso.filename);
					}					
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Package Information"); ?>',
						message: jso.msg,
						type: BootstrapDialog.TYPE_DANGER,
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
				
			}
		});		
	}
	
	/* Remove License By License Key */
	function RemoveLicense(LICENSE_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'RemoveLicense',				
				 'LICENSE_KEY' 	:LICENSE_UUID
			},
			success:function(jso)
			{
				ReloadQueryLicense();
				
				if (jso == true)
				{				
					BootstrapDialog.show({
						title: '<?php echo _("Remove License"); ?>',						
						message: '<?php echo _("License removed."); ?>',
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}]
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Remove License"); ?>',
						message: '<?php echo _("Failed to remove license."); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,					
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}]
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});
		
	}
	
	/* Query License History*/
	function QueryLicenseHistory(MACHINE_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'TEXT',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'QueryLicenseHistory',				
				 'MACHINE_UUID' :MACHINE_UUID
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#HostInfo').prop('disabled', false);
				window.setTimeout(function(){
					BootstrapDialog.show({
					title: '<?php echo _("License History Information"); ?>',
					cssClass: 'license-history-dialog',
					message: jso,
					type: BootstrapDialog.TYPE_PRIMARY,
					draggable: true});
				}, 0);
			},
			error: function(xhr)
			{
				
			}
		});
		
	}
		
	/* On-line License Activation */
	function OnlineLicenseActivation()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'   	:'OnlineLicenseActivation',				
				 'LICENSE_NAME' :$("#INPUT_NAME").val(),
				 'LICENSE_EMAIL':$("#INPUT_EMAIL").val(),
				 'LICENSE_KEY' 	:$("#INPUT_KEY").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#SubmitOnlineLicenseActivation').prop('disabled', false);
				$('#SubmitOnlineLicenseActivation').removeClass('btn-default').addClass('btn-primary');
				ReloadQueryLicense();
				
				if (jso.status == true)
				{				
					$('#INPUT_NAME').val("");
					$('#INPUT_EMAIL').val("");
					$('#INPUT_KEY').val("");

					BootstrapDialog.show({
						title: '<?php echo _("License Information"); ?>',
						message: jso.msg,
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,					
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							setTimeout(function () {document.getElementById("defaultLicenseTab").click();}, 888);
						}
					});
				}
				else
				{
					if (jso.status_code == 1)
					{
						BootstrapDialog.show({
							title: '<?php echo _("License Information"); ?>',
							message: jso.msg,
							type: BootstrapDialog.TYPE_DANGER,
							draggable: true,					
							buttons:[{
								label: '<?php echo _("Download License"); ?>',
								action: function(dialogRef){QueryPackageInfo($("#INPUT_KEY").val());}
							},
							{
									label: '<?php echo _("Close"); ?>',
									action: function(dialogRef){dialogRef.close();}
							}],
							onhide: function(dialogRef){
								setTimeout(function () {document.getElementById("defaultLicenseTab").click();}, 888);
							}
						});
					}
					else
					{
						BootstrapDialog.show({
							title: '<?php echo _("License Information"); ?>',
							message: jso.msg,
							type: BootstrapDialog.TYPE_DANGER,
							draggable: true,					
							buttons:[{
								label: '<?php echo _("Close"); ?>',
								action: function(dialogRef){dialogRef.close();}
							}],
							onhide: function(dialogRef){
								//setTimeout(function () {document.getElementById("defaultLicenseTab").click();}, 888);
							}
						});
					}
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Off-line Activation Input */
	function OfflineActivationInput(LICENSE_UUID)
	{
		var $OfflineActivationInputHTML = $('<div class="form-group">');
			$OfflineActivationInputHTML.append('<div></div>');
			$OfflineActivationInputHTML.append('<h4><i class="fa fa-key" aria-hidden="true"></i> <?php echo _("Offline Activation"); ?></h4>');
			$OfflineActivationInputHTML.append('<textarea class="form-control" rows="6" id="INPUT_LICENSE" placeholder="Paste string here"></textarea>');
			$OfflineActivationInputHTML.append('</div>');
					
		BootstrapDialog.show
		({
			title: '<?php echo _("Input String For Activation"); ?>',
			message: $OfflineActivationInputHTML,
			draggable: true,
			closable: true,
			buttons: 
			[
				{
					label: '<i class="fa fa-cog fa-lg fa-fw"></i><?php echo _("Activate"); ?>',
					cssClass: 'btn-primary',
					action: function(dialogRef)
					{
						OfflineLicenseActivation(LICENSE_UUID);
						dialogRef.close();
					}
				}										
			]
		});		
	}
	
	
	/* Off-line License Activation */
	function OfflineLicenseActivation(LICENSE_UUID)
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'   	:'OfflineLicenseActivation',
				 'LICENSE_KEY'  :LICENSE_UUID,
				 'LICENSE_TEXT' :$("#INPUT_LICENSE").val(),
			},
			success:function(jso)
			{
				ReloadQueryLicense();				
				if (jso.status == true)
				{				
					$('#INPUT_LICENSE').val("");
					
					BootstrapDialog.show({
						title: '<?php echo _("License Information"); ?>',
						message: '<?php echo _("Transport is activated."); ?>',
						type: BootstrapDialog.TYPE_PRIMARY,
						draggable: true,					
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							setTimeout(function () {document.getElementById("defaultLicenseTab").click();}, 500);
						}
					});
				}
				else
				{
					if (jso.why != null)
					{
						error_msg = jso.why;
					}
					else
					{
						error_msg = '<?php echo _("Product activation failed."); ?>';
					}
					
					BootstrapDialog.show({
						title: '<?php echo _("License Information"); ?>',
						message: error_msg,
						type: BootstrapDialog.TYPE_DANGER,
						draggable: true,					
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							//setTimeout(function () {document.getElementById("defaultLicenseTab").click();}, 500);
						}
					});
				}
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Query Report Time List */
	function QueryReportTimeList(){
		var date = new Date();
		var hour = date.getHours();
		hour = hour + 1;		
		if (hour < 10) {hour = "0" + hour;}
		DefaultTime = hour+":00:00";
				
		$(".jquery_time").attr("placeholder", DefaultTime).val("").focus().blur();
		$('.form_time').datetimepicker({						
						weekStart: false,
						todayBtn:  false,
						autoclose: true,						
						startView: 1,
						minView: 0,
						maxView: 0,
						forceParse: false,
						fontAwesome: true,
						minuteStep: 15
		});
	}
	
	/* UPDATE NOTIFICATION TYPE */
	function UpdateNotificationType()
	{
		var notification_time = {};
		if ($('#DailyReport').is(':checked'))
		{
			ReportRoutineTime = ($("#DailyReportRoutineTime").val() == '') ? DefaultTime : $("#DailyReportRoutineTime").val();
			notification_time['DailyReport'] = ReportRoutineTime;
		}
		
		if ($('#DailyBackup').is(':checked'))
		{
			BackupRoutineTime = ($("#DailyBackupRoutineTime").val() == '') ? DefaultTime : $("#DailyBackupRoutineTime").val();
			notification_time['DailyBackup'] = BackupRoutineTime;
		}

		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'   		:'UpdateNotificationType',
				 'ACCT_UUID' 		:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'NOTIFICATION_TYPE':$.map($('input[name="Notification"]:checked'), function(c){return c.value;}),
				 'NOTIFICATION_TIME':notification_time
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			
				BootstrapDialog.show({
					title: '<?php echo _("Information"); ?>',
					message: '<?php echo _("Notification preferences saved."); ?>',
					draggable: true,					
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){dialogRef.close();}
					}],
					onhide: function(dialogRef){
						setTimeout(function () {window.location.href = 'MgmtAcct';}, 500);
					}
				});
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	
	/* GET SMTP TYPE */
	function GetSMTPType(type)
	{
		switch (type.value) 
		{
			case 'tls':
				$("#INPUT_SMTP_PORT").val(587);
			break;
		
			case 'ssl':
				$("#INPUT_SMTP_PORT").val(465);
			break;
		
			default:
				$("#INPUT_SMTP_PORT").val(25);
		}	
	}
	
	/* Test SMTP */
	function TestSMTP()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'   	:'TestSMTP',
				 'LANGUAGE'		:'<?php echo $_SESSION['language']; ?>',
				 'SMTP_HOST' 	:$("#INPUT_SMTP_HOST").val(),
				 'SMTP_TYPE' 	:$("#INPUT_SMTP_TYPE").val(),
				 'SMTP_PORT' 	:$("#INPUT_SMTP_PORT").val(),
				 'SMTP_USER'	:$("#INPUT_SMTP_USER").val(),
				 'SMTP_PASS' 	:$("#INPUT_SMTP_PASS").val(),
				 'SMTP_FROM' 	:$("#INPUT_SMTP_FROM").val(),
				 'SMTP_TO' 		:$("#INPUT_SMTP_TO").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');

				if (jso.status == true)
				{
					$("#INPUT_SMTP_HOST").prop('disabled', true);
					$("#INPUT_SMTP_TYPE").prop('disabled', true);
					$("#INPUT_SMTP_PORT").prop('disabled', true);
					$("#INPUT_SMTP_USER").prop('disabled', true);
					$("#INPUT_SMTP_PASS").prop('disabled', true);
					$("#INPUT_SMTP_FROM").prop('disabled', true);
					$("#INPUT_SMTP_TO").prop('disabled', true);
							
					$('#VerifySMTPSetting').prop('disabled', true);
					$('#VerifySMTPSetting').removeClass('btn-primary').addClass('btn-default');
						
					$('#SaveSMTPSetting').prop('disabled', false);
					$('#SaveSMTPSetting').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifySMTPSetting').prop('disabled', false);
					$('#VerifySMTPSetting').removeClass('btn-default').addClass('btn-primary');
				}

				BootstrapDialog.show({
					title: '<?php echo _("Information"); ?>',
					message: jso.reason,
					draggable: true,					
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){dialogRef.close();}
					}],					
				});
			},
			error: function(xhr)
			{
		
			}
		});
	}
	
	/* Delete SMTP Settings */
	function DeleteSMTPSettings()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'   	:'DeleteSMTPSettings',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
								
				BootstrapDialog.show({
					title: '<?php echo _("Information"); ?>',
					message: '<?php echo _("SMTP Setting Deleted"); ?>',
					draggable: true,					
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){dialogRef.close();}
					}],
					onhide: function(dialogRef){
						$('#VerifySMTPSetting').prop('disabled', false);
						$('#VerifySMTPSetting').removeClass('btn-default').addClass('btn-primary');
						
						$('#SaveSMTPSetting').prop('disabled', true);
						$('#SaveSMTPSetting').removeClass('btn-primary').addClass('btn-default');
						
						setTimeout(function () {window.location.href = 'MgmtAcct';}, 500);
					}
				});
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	/* Save SMTP Settings */
	function SaveSMTPSettings()
	{
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_acct.php',
			data:{
				 'ACTION'   	:'SaveSMTPSettings',
				 'ACCT_UUID' 	:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SMTP_HOST' 	:$("#INPUT_SMTP_HOST").val(),
				 'SMTP_TYPE' 	:$("#INPUT_SMTP_TYPE").val(),
				 'SMTP_PORT' 	:$("#INPUT_SMTP_PORT").val(),
				 'SMTP_USER'	:$("#INPUT_SMTP_USER").val(),
				 'SMTP_PASS' 	:$("#INPUT_SMTP_PASS").val(),
				 'SMTP_FROM' 	:$("#INPUT_SMTP_FROM").val(),
				 'SMTP_TO' 		:$("#INPUT_SMTP_TO").val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
								
				BootstrapDialog.show({
					title: '<?php echo _("Information"); ?>',
					message: '<?php echo _("SMTP Settings Saved."); ?>',
					draggable: true,					
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){dialogRef.close();}
					}],
					onhide: function(dialogRef){
						$('#Replication').prop('disabled', false);
						$('#Recovery').prop('disabled', false);
						$('#DailyReport').prop('disabled', false);
						$('#DailyBackup').prop('disabled', false);
						$('#DailyReportTime').prop('disabled', false).css('pointer-events', 'pointer');
						$("#DailyReportCalendarPoint").css('pointer-events', 'auto');
						$('#DailyBackupTime').prop('disabled', false).css('pointer-events', 'pointer');
						$("#DailyBackupCalendarPoint").css('pointer-events', 'auto');
						
						$('#SubmitReportType').prop('disabled', false);
						$('#SubmitReportType').removeClass('btn-default').addClass('btn-primary');						
						
						$('#DeleteSMTPSetting').prop('disabled', false);
						$('#DeleteSMTPSetting').removeClass('btn-default').addClass('btn-primary');
						
						$('#VerifySMTPSetting').prop('disabled', false);
						$('#VerifySMTPSetting').removeClass('btn-default').addClass('btn-primary');
						
						$('#SaveSMTPSetting').prop('disabled', true);
						$('#SaveSMTPSetting').removeClass('btn-primary').addClass('btn-default');
						
						setTimeout(function () {document.getElementById("defaultNotificationTab").click();}, 888);
						//setTimeout(function () {window.location.href = 'MgmtAcct';}, 500);
					}
				});
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Submit Trigger */
	$(function(){
		$("#SubmitOnlineLicenseActivation").click(function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			$('#SubmitOnlineLicenseActivation').prop('disabled', true);
			$('#SubmitOnlineLicenseActivation').removeClass('btn-primary').addClass('btn-default');
			OnlineLicenseActivation();
		})
		
		$("#SubmitReportType").click(function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			UpdateNotificationType();
		})
		
		$("#DeleteSMTPSetting").click(function(){
			BootstrapDialog.show({
				message: '<?php echo _('Do you want to clean up SMTP information?'); ?>',
				draggable: true,
				buttons:
				[
					{
						label: '<?php echo _('Cancel'); ?>',
						action: function(dialogItself)
						{
							dialogItself.close();
						}
					},
					{
						label: '<?php echo _('Submit'); ?>',
						cssClass: 'btn-primary',
						action: function(dialogItself)
						{
							$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
							DeleteSMTPSettings();
							dialogItself.close();
						}
					}					
				]
			});
		})
		
		$("#VerifySMTPSetting").click(function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			$('#VerifySMTPSetting').prop('disabled', true);
			$('#VerifySMTPSetting').removeClass('btn-primary').addClass('btn-default');
			TestSMTP();
		})
		
		$("#SaveSMTPSetting").click(function(){
			$('#LoadingOverLay').addClass('GrayOverlay GearLoading');
			SaveSMTPSettings();
		})
		
	});
	
	/* CLICK ONCHANGE */
	function click_onchange($type){
		TimeInputId = "#"+$type+'Time';
		CalendarPoint = "#"+$type+'CalendarPoint';
	
		if (document.getElementById($type).checked)
		{
			$(TimeInputId).prop('disabled', false).css('pointer-events', 'pointer');
			$(CalendarPoint).css('pointer-events', 'auto');
		}
		else
		{
			$(TimeInputId).prop('disabled', true).css('pointer-events', 'none');
			$(CalendarPoint).css('pointer-events', 'none');
		}
	}
	
	/* EYE PASSWORD */
	$(".reveal").mousedown(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'text'));
		$(".re-pwd").replaceWith($('.re-pwd').clone().attr('type', 'text'));
		$(".smtp-pwd").replaceWith($('.smtp-pwd').clone().attr('type', 'text'));
	})
	.mouseup(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'password'));
		$(".re-pwd").replaceWith($('.re-pwd').clone().attr('type', 'password'));
		$(".smtp-pwd").replaceWith($('.smtp-pwd').clone().attr('type', 'password'));
	})
	.mouseout(function() {
		$(".pwd").replaceWith($('.pwd').clone().attr('type', 'password'));
		$(".re-pwd").replaceWith($('.re-pwd').clone().attr('type', 'password'));
		$(".smtp-pwd").replaceWith($('.smtp-pwd').clone().attr('type', 'password'));
	});
</script>