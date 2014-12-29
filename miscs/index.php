<?php
/**
 *	@file: liber.php
 *	@author: Soyoes 2014/01/28
 *****************************************************************************/
require 'conf/conf.inc';
const APP_NAME = 'YOUR_APP_NAME';
const LIBER_DIR = 'YOUR_LIBER_DIR';
const __SLASH__ = DIRECTORY_SEPARATOR;
define('APP_DIR', dirname(__FILE__));
define('IMAGE_DIR', APP_DIR . __SLASH__ . "webroot" . __SLASH__ . "images" . __SLASH__);
set_include_path(
get_include_path(). PATH_SEPARATOR
. LIBER_DIR .__SLASH__. 'modules'.__SLASH__.'utils'. PATH_SEPARATOR 
. APP_DIR.__SLASH__.'delegate'.__SLASH__. PATH_SEPARATOR
. APP_DIR.__SLASH__.'modules'.__SLASH__ 
);

$modulefiles = ['Lang','Session','Caches','DB','Core','Model','Render','Filter','Auth'];
foreach($modulefiles as $f){
	require_once LIBER_DIR.__SLASH__."modules".__SLASH__.$f.".inc";
}

spl_autoload_register(function($class){
	if(!include_once $class.'.inc')
		include_once $class.'.php';
});
try{
	$cli_args = array_slice($argv, 1);
	$cli_cmd = array_shift($cli_args);
	if(php_sapi_name() == 'cli' || PHP_SAPI == 'cli'){
		$cli_cmd="cli_".$cli_cmd;
		if(function_exists($cli_cmd)){$cli_cmd();}
	}else{
		REQ::dispatch();
	}
}catch(Exception $e){
	error_log($e->getMessage());
	exit;
}
function cli_script(){
	global $cli_args;
	$f = array_shift($cli_args);
	if(!empty($f)){
		$pwd=dirname(__FILE__);
		$f = $pwd."/scripts/$f.php";
		include $f;
	}
	exit;
}
function cli_migrate(){
	try{
		$pwd=dirname(__FILE__);
		$schemas = glob($pwd."/conf/schemas/*.ini") ;
		foreach ($schemas as $file){
			echo $file."\n";
			$parts = explode("/",$file);
			$file = $parts[count($parts)-1];
			$parts = explode(".", $file);
			$schema = $parts[0];
			echo $schema."\n";
			db_migrate($schema);
		}
		echo "DONE\n";
	}catch(Exception $e){
		echo "FAILED\n";
	}
	exit;
}
function llog($label, $value, $toScreen=false){
	$value = is_array($value)?json_encode($value,JSON_UNESCAPED_UNICODE|JSON_PRETTY_PRINT):$value;
	if($toScreen){
		$label = "<b>".$label."</b>";
		$value = "<pre>".$value."</pre>";
	}
	if(REQ_MODE=="CLI"||$toScreen){
		echo $label.":".$value.($toScreen?"<br>":"
");
	}else{
		error_log($label.":".$value."
");
	}
}

?>
