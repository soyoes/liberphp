<?php
/**
 *	@file: init.inc
 *	@author: Soyoes 2013/08/14
 *	@uses:
 *	@example:
 *****************************************************************************/
/*__APP__NAME__*/
const __SLASH__ = DIRECTORY_SEPARATOR;
define("APP_DIR", dirname(__FILE__));
set_include_path(
	get_include_path(). PATH_SEPARATOR 
	. APP_DIR.__SLASH__."modules".__SLASH__
	. PATH_SEPARATOR . __SLASH__."filters".__SLASH__
	. PATH_SEPARATOR . __SLASH__."libs".__SLASH__
	. PATH_SEPARATOR . LIBER_DIR.__SLASH__. "modules"
	. PATH_SEPARATOR . LIBER_DIR .__SLASH__. "modules".__SLASH__.'utils'
);

/** Customized */
spl_autoload_register(function($class){
	if(!include_once $class.".inc")
		include_once $class.".php";
});

include_once 'Core.inc';
