<?php
class Conf{
	//change this to mode : false will close Test, Render will use cache under /tmp instead of compile every time.
	//available modes : Developing | Staging | Product | Maintenance 
	static $mode 				= "Developing";
	
	static $lang 				= "jp";
	static $image_host 			= null;
	static $default_controller	= "top";
	static $default_action		= null;
	//custom path prefix of each request 
	static $path_prefix			= "";
	
	static $db_engine			= "mysql";
	static $db_host				= "127.0.0.1";
	static $db_port				= "3306";
	static $db_name				= "todos";
	static $db_user				= "root";
	static $db_pass				= "root";

	static $cross_domain_methods = "GET, POST, PUT, DELETE, OPTIONS";

	static $cache_hosts			= "localhost";
	static $cache_port			= "11211";
	
	static $session_enable		= true;
	static $session_lifetime	= 86400;
	
	static $filters				= [];
	
	//FIXME remove this
	static $oauth_clients 		= [];
	
	static $schema_reg			= "regAt";
	static $schema_upd			= "updAt";
	
}

switch (Conf::$mode) {
	case 'Developing':
		Conf::$db_engine	= "mysql";
		Conf::$db_host		= "127.0.0.1";
		Conf::$db_port		= "3306";
		Conf::$db_name		= "hug";
		Conf::$db_user		= "root";
		Conf::$db_pass		= "root";
		break;
	case 'Staging':
		Conf::$db_engine	= "mysql";
		Conf::$db_host		= "127.0.0.1";
		Conf::$db_port		= "3306";
		Conf::$db_name		= "hug";
		Conf::$db_user		= "root";
		Conf::$db_pass		= "root";
		break;
	case 'Product':
		break;
	case 'Maintenance':
		Conf::$filters		= ['maintenance'=>'*'];
		break;
	default:
		break;
}