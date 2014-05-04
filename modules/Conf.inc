<?php
class Conf{
	static $lang 				= "jp";
	static $image_host 			= null;
	static $default_controller	= "top";
	static $default_action		= null;
	
	static $db_engine			= "mysql";
	static $db_host				= "127.0.0.1";
	static $db_port				= "3306";
	static $db_name				= "todos";
	static $db_user				= "root";
	static $db_pass				= "root";
	
	static $cache_hosts			= "localhost";
	static $cache_port			= "11211";
	
	static $template_check		= true;
	
	//FIXME change this to mode : developing | product
	static $test_mode_enable 	= true;
	
	static $session_enable		= true;
	static $session_lifetime	= 86400;
	
	static $filters				= [];
	
	//FIXME remove this
	static $oauth_clients 		= [];
	
	static $schema_reg			= "regAt";
	static $schema_upd			= "updAt";
	static $schema_user_table	= "users";
	static $schema_user_id		= "id";
	static $schema_user_email	= "email";
	static $schema_user_pass	= "pass";
	static $schema_user_activated	= "activated";
	
	//Permission
	static $auth_group_name = "auth_group";
	static $auth_user_name = "auth_user";
}