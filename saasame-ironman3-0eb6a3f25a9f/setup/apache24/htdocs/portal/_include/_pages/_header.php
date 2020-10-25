<!DOCTYPE html>
<html lang="en">
	<head>
		<meta charset="utf-8">
		<title>SaaSaMe::Transport</title>
		<meta name="keywords" content="SaaSaMe IronMan" />
		<meta name="description" content="SaaSaMe IronMan" />
		<meta http-equiv="Pragma" content="no-cache" />	
		<link rel="shortcut icon" type="image/ico" href="_include/_img/saasame_logo.ico" />
			<!-- Latest compiled Jquery -->
			<script type='text/javascript' src='_include/_style/_js/jquery-latest.min.js'></script>
		
			<!-- Jquery Post and Redirect-->
			<!--<script type='text/javascript' src='_include/_style/_js/jquery.redirect.js'></script> -->
			
			<!-- Latest compiled and minified CSS -->
			<script src='_include/_style/_js/bootstrap.min.js'></script>
				
			<!-- Optional theme -->
			<link rel='stylesheet' href='_include/_style/_css/bootstrap-theme.min.css'>

			<!-- Latest compiled and minified JavaScript -->		
			<link rel='stylesheet' href='_include/_style/_css/bootstrap.min.css'>
			
			<!-- Load BootStrap Dialog CSS and JavaScript -->
			<script src='_include/_style/_bootstrap/bootstrap-dialog/js/bootstrap-dialog.min.js'></script>
			<link rel='stylesheet' href='_include/_style/_bootstrap/bootstrap-dialog/css/bootstrap-dialog.min.css'>
			
			<!-- Load BootStrap Select CSS and JavaScript -->
			<script src='_include/_style/_bootstrap/bootstrap-select/js/bootstrap-select.min.js'></script>
			<link rel='stylesheet' href='_include/_style/_bootstrap/bootstrap-select/css/bootstrap-select.min.css'>
			
			<!-- Load CSS CheckBox Style -->
			<link rel='stylesheet' href='_include/_style/_css/checkbox.css'>
			
			<!-- Load CSS CheckBox funkyradio Style -->
			<link rel="stylesheet" href='_include/_style/_css/funkyradio.css'>
			
			<!-- Load BootStrap Nav Wizard Style -->
			<link rel='stylesheet' href='_include/_style/_css/bootstrap-nav-wizard.css'>
					
			<!-- Load Font Awesone -->
			<link rel='stylesheet' href='_include/_style/_fonts/font-awesome/css/font-awesome.min.css'>
			
			<!-- Load Bootstrap Datetimepicker -->
			<script src='_include/_style/_bootstrap/bootstrap-datetimepicker/js/bootstrap-datetimepicker.min.js'></script>
			<link rel='stylesheet' href='_include/_style/_bootstrap/bootstrap-datetimepicker/css/bootstrap-datetimepicker.min.css' media="screen">
			
			<!-- Load Bootstrap Toggle -->
			<script src='_include/_style/_bootstrap/bootstrap-toggle/js/bootstrap-toggle.min.js'></script>
			<link rel='stylesheet' href='_include/_style/_bootstrap/bootstrap-toggle/css/bootstrap-toggle.min.css' media="screen">
			
			<!-- Latest compiled Jquery dataTables -->
			<script type='text/javascript' src='_include/_style/_js/jquery.dataTables.min.js'></script>
			
			<!-- Latest Jquery dataTables file size sorting plugin -->
			<script type='text/javascript' src='_include/_style/_js/file-size.js'></script>
			
			<!-- DataTables Output CSS -->
			<link href="_include/_style/_bootstrap/datatables-buttons/css/buttons.dataTables.min.css" rel="stylesheet" type="text/css">	
			
			<!-- DataTables Output JavaScript -->
			<script type='text/javascript' src="_include/_style/_bootstrap/datatables-buttons/js/dataTables.buttons.min.js"></script>
			<script type='text/javascript' src="_include/_style/_bootstrap/datatables-buttons/js/jszip.min.js"></script>
			<!-- <script type='text/javascript' src="_include/_style/_bootstrap/datatables-buttons/js/pdfmake.min.js"></script> -->
			<!-- <script type='text/javascript' src="_include/_style/_bootstrap/datatables-buttons/js/vfs_fonts.js"></script> -->
			<script type='text/javascript' src="_include/_style/_bootstrap/datatables-buttons/js/buttons.html5.min.js"></script>

			<!-- Clipboard JavaScript -->
			<script type='text/javascript' src="_include/_style/_js/clipboard.min.js"></script>
			
			<!-- FileSaver JavaScript -->
			<script type='text/javascript' src="_include/_style/_js/FileSaver.min.js"></script>	
			
			<!-- Load Main CSS Style -->
			<link rel="stylesheet" type="text/css" href="_include/_style/_css/style.css?<?php echo rand(10,100); ?>" />
			
			<!-- Load Menu CSS Style -->
			<link rel="stylesheet" type="text/css" href="_include/_style/_css/menu.css?<?php echo rand(10,100); ?>" />
			
			<!-- Load Pace Loading -->
			<script>window.paceOptions = {ajax: {trackMethods: ['POST']}};</script>
			<script src="_include/_style/_js/pace.min.js"></script>
			<link href="_include/_style/_css/pace-theme.css" rel="stylesheet" />
			
			<!-- Load IP Check Modules -->
			<script src="_include/_style/_js/ip_check.js"></script>
			
			<!-- Load vis.js Modules -->
			<script src="_include/_style/_js/vis.js"></script>
			<link href="_include/_style/_css/vis-network.min.css" rel="stylesheet" />
		</head>
<body>
<div id='LoadingOverLay' class=''></div>

<script>
	BootstrapDialog.DEFAULT_TEXTS[BootstrapDialog.TYPE_PRIMARY] = '<?php echo _("Information"); ?>';
	BootstrapDialog.DEFAULT_TEXTS['OK'] = '<?php echo _("Submit"); ?>';
	BootstrapDialog.DEFAULT_TEXTS['CANCEL'] = '<?php echo _("Cancel"); ?>';
</script>
