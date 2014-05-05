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
	
	/*Auth & permissions*/
	static $auth_group_name 	= "auth_group"; //session key to save auth group id
	static $auth_user_id_name 	= "auth_user_id";	//session key to save auth user id
	static $auth_user_data_name = "auth_user";	//session key to save auth user data
	static $auth_patterns = [
		'/user/login' 	=> [
			'schema'=>'users', 	
			'group'=>1, 
			'fields.email'=>'email', 
			'fields.pass'=>'pass'
		],
		'/vip/login' 	=> [
			'schema'=>'vips', 	
			'group'=>2, 
			'fields.email'=>'email', 
			'fields.pass'=>'pass'
		],
		'/admin/login' 	=> [
			'schema'=>'admins', 
			'group'=>3, 
			'fields.email'=>'email', 
			'fields.pass'=>'pass'
		],
	];
}