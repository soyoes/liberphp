<?php
/**
 *	@file: index.php
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

$modulefiles = ['Lang','Session','Caches','DB','Core','Render','Filter','QL'];
foreach($modulefiles as $f){
	require_once LIBER_DIR.__SLASH__."modules".__SLASH__.$f.".inc";
}

spl_autoload_register(function($class){
	if(!include_once $class.'.inc')
		include_once $class.'.php';
});
try{
	REQ::dispatch();
}catch(Exception $e){
	error_log($e->getMessage());
	exit;
}