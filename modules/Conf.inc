<?php
class Conf{
	//change this to mode : false will close Test, Render will use cache under /tmp instead of compile every time.
	//available modes : Developing | Product | Maintenance 
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

/**
    when $s3_emulate is true:
        S3 emulation mode.
        Parameters $s3_emulate_path and $s3_emulate_url must be set.
        In this situation, files will be uploaded to a local folder, which is defined in $s3_emulate_path.
        Typically, $s3_emulate_path should be set to a folder which is under a web project's DocumentRoot,
        $s3_emulate_url should be set to a web path of the $s3_emulate_path
    when $s3_emulate is false:
        Files will be uploaded to S3 server.
        Parameters $s3_key, $s3_secret, $s3_bucket, $s3_public_access must be give.
*/
    static $s3_emulate          = false;
    static $s3_emulate_path     = "/var/www/html/example/webroot/images/tmp";
    static $s3_emulate_url      = "http://example.com/images/tmp";
    static $s3_bucket           = "example";
    static $s3_public_access    = true;
    static $s3_key              = "%S3_ACCESS_ID%";
    static $s3_secret           = "%S3_SECRET_KEY%";

	static $cache_hosts			= "localhost";
	static $cache_port			= "11211";
	
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