<?php

/**
 *	@file: init.inc
 *	@author: Soyoes 2013/08/14
 *	@uses:
 *	@example:
 *****************************************************************************/

$wroot = dirname(__FILE__);
const __SLASH__ = DIRECTORY_SEPARATOR;

//system environment
if (!apc_load_constants("ENV")) {
	$env = [
		'APP_ROOT' 			=> $wroot,
		'WWW_ROOT' 			=> $wroot . __SLASH__ . "webroot" . __SLASH__,
		'CTRL_DIR' 			=> $wroot . __SLASH__ . "controllers" . __SLASH__,
		'MODULE_DIR' 		=> $wroot . __SLASH__ . "modules" . __SLASH__,
		'FILTER_DIR' 		=> $wroot . __SLASH__ . "filters" . __SLASH__,
		'REST_DIR' 			=> $wroot . __SLASH__ . "restful" . __SLASH__,
		'CONF_DIR' 			=> $wroot . __SLASH__ . "conf" . __SLASH__,
		'TEST_DIR' 			=> $wroot . __SLASH__ . "test" . __SLASH__,
		'TMP_DIR' 			=> $wroot . __SLASH__ . "tmp" . __SLASH__,
		'LOCAL_LIB_DIR' 	=> $wroot . __SLASH__ . "lib" . __SLASH__,
		'MASTER_DIR' 		=> $wroot . __SLASH__ . "conf" . __SLASH__ . "masters".__SLASH__,
		'TEXT_DIR' 			=> $wroot . __SLASH__ . "conf" . __SLASH__ . "texts".__SLASH__,
		'SCHEMA_DIR' 		=> $wroot . __SLASH__ . "conf" . __SLASH__ . "schemas".__SLASH__,
		'VIEW_DIR' 			=> $wroot . __SLASH__ . "views" . __SLASH__,
		'CSS_DIR' 			=> $wroot . __SLASH__ . "webroot" . __SLASH__ . "css".__SLASH__,
		'JS_DIR' 			=> $wroot . __SLASH__ . "webroot" . __SLASH__ . "js".__SLASH__,
		'IMAGE_DIR' 		=> $wroot . __SLASH__ . "webroot" . __SLASH__ . "images".__SLASH__,
	];
	apc_define_constants("ENV", $env);
}

//user configurations
if (!apc_load_constants("CONF")){
	$CONF = parse_ini_file($wroot.'/conf/conf.ini', true);
	$userEnv = [];
	foreach($CONF as $seg=>$items){
		foreach($items as $key=>$value){
			$name = strtoupper($seg."_".$key);
			$userEnv[$name] = $value;
		}
	}
	$userEnv['LIBER_DIR'] = $CONF["APP"]['lib_dir'];
	/*
	$common_controllers = array_map(glob($userEnv['LIBER_DIR']."/common/controllers/*.php"), function($e){
		return preg_replace(["(.*)\/","\.php"],["@",""],$e);
	});
	$userEnv['LIB_CONTROLLERS'] = "|".join("|",$common_controllers)."|";
	error_log("set liber_ctrl");
	*/
	$userEnv['LIBER_CTRL_DIR'] = $userEnv['LIBER_DIR']."common/controllers".__SLASH__;
	$userEnv['LIBER_LIB_DIR'] = $userEnv['LIBER_DIR']."lib".__SLASH__;
	$userEnv['LIBER_MODULE_DIR'] = $userEnv['LIBER_DIR']."modules".__SLASH__;
	$userEnv['SMARTY_DIR'] = $userEnv['LIBER_LIB_DIR'] . "smarty".__SLASH__;
	$userEnv["JS_PATH"] = isset($CONF["APP"]['js_path'])?$CONF["APP"]['js_path']:"/js/";
	$userEnv["CSS_PATH"] = isset($CONF["APP"]['css_path'])?$CONF["APP"]['css_path']:"/css/";
	$userEnv["IMAGE_PATH"] = isset($CONF["APP"]['image_path'])?$CONF["APP"]['image_path']:"/images/";
	$userEnv["IMAGE_HOST"] = isset($CONF["APP"]['image_host'])?$CONF["APP"]['image_host']:"http://".$_SERVER["HTTP_HOST"]."/".$userEnv["IMAGE_PATH"];
	apc_define_constants("CONF", $userEnv);
}

set_include_path(
	get_include_path()
	. PATH_SEPARATOR . MODULE_DIR
	. PATH_SEPARATOR . FILTER_DIR
	. PATH_SEPARATOR . LIBER_DIR
	. PATH_SEPARATOR . LIBER_MODULE_DIR
	. PATH_SEPARATOR . LIBER_MODULE_DIR . 'database'
	. PATH_SEPARATOR . LIBER_MODULE_DIR . 'utils'
);

/** Customized */
spl_autoload_register(function($class){
	if(!include_once $class.".inc")
		include_once $class.".php";
});

include_once 'Core.inc';
