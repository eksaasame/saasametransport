<?php

$LANG = $_GET['lang'];

#$LANG = 'zh_TW';
#$LANG = 'en_US';

// define constants
define('PROJECT_DIR', realpath('./'));
define('LOCALE_DIR', 'C:\Users\Administrator\Desktop\WAMP\apache24\htdocs\xyzzy\gettext\locales');
define('DEFAULT_LOCALE', $LANG);

require_once('gettext.inc');

$locale = (isset($_GET['lang']))? $_GET['lang'] : DEFAULT_LOCALE;

//var_dump($locale);die();

// gettext setup
T_setlocale(LC_MESSAGES, $locale);
// Set the text domain as 'messages'
$domain = 'messages';
bindtextdomain($domain, LOCALE_DIR);
textdomain($domain);

echo _('Login');