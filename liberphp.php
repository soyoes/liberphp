#!/opt/local/bin/php54
<?php
/**
 *	@file: migrate.php
 *	@author: Soyoes 2013/08/13
 *	@uses:
 *	@example:
 *****************************************************************************/
$pwd=dirname(__FILE__);
require_once "$pwd/init.inc";
$args = array_slice($argv, 1);
$cmd = array_shift($args);

$err = "Usage : \n    liberphp create PROJECT_NAME\n".
			"    liberphp migrate # migrate database\n";

if($cmd)
try{$cmd();}catch(Exception $e){
	echo $err;
}else 
	echo $err;


function create(){
	global $args;
	$proj = array_shift($args);
	if(!$proj || $proj==""){
		echo "Specify your project name pls \n./liberphp.php create PROJECT_NAME\n";
		exit;
	}
	global $pwd;
	mkdir("../".$proj);
	exec("tar zxf miscs/files.tgz -C ../$proj/");
	
	exec("sed -i -e 's:YOUR_LIBERPHP_PATH:".$pwd.":g' ../$proj/conf/conf.ini");
	exec("sed -i -e 's:YOUR_APP_NAME:".$proj.":g' ../$proj/conf/conf.ini");
	
	$path =explode("/",$pwd);
	$path = join("/",array_slice($path, 0, count($path)-2))."/".$proj;
	
	exec("cp init.inc ../$proj/");
	
	echo <<<EOF
DONE!
Your project is created under $path
 
#How to make it work
		
# 1) Update you Apache configuration
# Add these rows to your 
<VirtualHost *:80>
    DocumentRoot "$path"
    Options FollowSymLinks
    ServerName $prog.dev
    RewriteEngine on
    RewriteRule  images/(.*)$ /webroot/images/$1  [L]
    RewriteRule  css/(.*)$ /webroot/css/$1    [L]
    RewriteRule  js/(.*)$ /webroot/js/$1    [L]
    RewriteRule  font/(.*)$ /webroot/font/$1    [L]
    RewriteRule  ^(.*)\.html$ /webroot/html/$1.htm    [L]
    RewriteRule !\.(php|svg|ttf|htc|ico|gif|png|jpg|jpeg|css|js|swf|html|htm|json)$ /index.php?__URL__=%{REQUEST_URI} [QSA,NE,L]
</VirtualHost>

# 2) Add virtural host IP to your hosts file 
# /etc/hosts
127.0.0.1		$proj.dev

# 3) DB migration (Option)
> cd $pwd
> ./liberphp.php ../$proj

# 4) Check out Smarty (http://www.smarty.net/download) 
	 to $path/lib/smarty

# 5) Restart your apache
		
# 6) Try it by accessing http://$proj.dev
	
   
EOF;
	
}

/**
 * migrate database of project.
 * @usage :	./liberphp.php migrate PROJECT_NAME
 * @example : ./liberphp.php migrate ../myproject
 * 
 * */
function migrate(){
	try{
		//create database;
		$sql = "CREATE DATABASE IF NOT EXISTS `DBName` CHARACTER SET utf8 COLLATE utf8_general_ci;";
		
		db_query($sql);
		//create assign user
		
		$sys_schemas = glob(LIBER_DIR."common/schemas/*.ini") ;
		$app_schemas = glob(CONF_DIR."schemas/*.ini") ;
		$schemas = array_merge($sys_schemas,$app_schemas);
		
		foreach ($schemas as $file){
			echo $file."\n";
			$parts = explode("/",$file);
			$file = $parts[count($parts)-1];
			$parts = explode(".", $file);
			$schema = $parts[0];
			echo $schema."\n";
			Model::migration($schema);
		}
		echo "DONE";
	}catch(Exception $e){
		echo "FAILED";
	}
	exit;
}
