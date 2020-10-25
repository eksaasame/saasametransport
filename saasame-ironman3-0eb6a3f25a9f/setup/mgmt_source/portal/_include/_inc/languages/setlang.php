<?php

function setLogo(){
	$langC = new MutiLanguage();

	$lang = $langC->getLanguage();
	
	if( $lang == "zh_tw")
		echo 'data:image/png;base64,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';
	else if( $lang == "zh_cn")
		echo 'data:image/png;base64,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';
	else
		echo 'data:image/png;base64,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';
}

function setCssId(){

	$langC = new MutiLanguage();

	$lang = $langC->getLanguage();
	
	if( $lang == "zh_tw")
		echo "Login";
	else if( $lang == "zh_cn")
		echo "Login_cn";
	else
		echo "Login";
}

class MutiLanguage{
	
	private $lang;
	
	public function __construct(){
		
	}
	
	private function getLang() {
		preg_match("/^([a-z\-]+)/i", $_SERVER["HTTP_ACCEPT_LANGUAGE"], $matches); //分析 HTTP_ACCEPT_LANGUAGE 的屬性
	
		$lang = $matches[1]; //取第一語言設置
		
		//默認語言 & 類型
		//putenv("LANG=en_US");
		//setlocale(LC_ALL, $lang);
		
		$lang = isset($_SESSION["language"]) ? $_SESSION["language"] : $lang;
		$lang = isset($_GET["lang"]) ? $_GET["lang"] : $lang;
		$lang = strtolower($lang); //轉換為小寫
		
		$this->lang = str_replace("-","_", $lang );

	}
	
	public function getLanguage(){
		
		$this->getLang();

		return $this->lang;
	}
	
	public function init(){
		
		$this->getLang();
		
		if ($this->lang == "en-us" || $this->lang == "en_us") { //English
			$this->setDomain( $lang );
		}
		if ($this->lang == "zh-tw" || $this->lang == "zh_tw") { //正體中文 (台灣)
			$this->setDomain( "zh_tw" );
		}
		if ($this->lang == "zh-cn" || $this->lang == "zh_cn") { //简体中文 (中国)
			$this->setDomain( $this->lang );
		}
	}
	
	public function init2(){
		
		$this->getLang();
		
		// define constants
		//define('PROJECT_DIR', realpath('./'));
		//define('DEFAULT_LOCALE', $this->lang);
		
		require_once('gettext\gettext.inc');

		T_setlocale(LC_MESSAGES, $this->lang);

		$domain = 'language';
		bindtextdomain($domain, __DIR__);
		textdomain($domain);
		
	}
	
	private function setDomain( $package = 'demo' ){
		bindtextdomain( $package, __DIR__.'/../languages'); // or $your_path/languages, ex: /var/www/test/languages
		textdomain( $package );
		bind_textdomain_codeset( $package, "utf-8" );
	}
	
	public function translate()
	{
		$args = func_get_args();
		$num = func_num_args();
		$args[0] = gettext($args[0]);
	
		if($num <= 1)
			return $args[0];
	
		$str = $args[0];
		
		foreach( $args as $index => $arg ){
			if( $index == 0 )
				continue;
			
			$key[] = '%'.$index.'%';
			$replace[] = $arg;
		}
		
		return str_replace($key, $replace, $str);
	
	}
}

$MutiLanguage = new MutiLanguage();

$MutiLanguage->init2();

function translate(){
	$MutiLanguage = new MutiLanguage();
	return call_user_func_array(array($MutiLanguage, 'translate'), func_get_args());
}

?>
