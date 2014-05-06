#!/opt/local/bin/php
<?php
/**
 *	@file: migrate.php
 *	@author: Soyoes 2013/08/13
 *	@uses:
 *	@example:
 *****************************************************************************/
/*LIBER_DIR*/
$pwd=dirname(__FILE__);
//require_once "$pwd/init.inc";
$args = array_slice($argv, 1);
$cmd = array_shift($args);

$err = "Usage : \n    liberphp create PROJECT_NAME\n".
			"    liberphp build # compress source code to liber.php\n";

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
	$path =explode("/",$pwd);
	$path = join("/",array_slice($path, 0, count($path)-2))."/".$proj;
	
	build($proj);
	//exec("cp init.inc ../$proj/");
	exec("mv liber.php ../$proj/");
	exec("cp modules/Conf.inc ../$proj/conf/conf.inc");
	exec("chmod +x ../$proj/liber.php");
	//exec("sed -i -e 's:/*__APP_NAME__*/:const APP_NAME=\"$proj\";\nconst LIBER_DIR=\"$pwd\";:g' ../$proj/liber.php");
	
	echo <<<EOF
DONE!
Your project is created under $path
 
#How to make it work
		
# 1) Update you Apache configuration
# Add these rows to your httpd.conf (OR httpd-vhosts.conf)
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
    RewriteRule !\.(php|svg|ttf|htc|ico|gif|png|jpg|jpeg|css|js|swf|html|htm|json)$ /liber.php?__URL__=%{REQUEST_URI} [QSA,NE,L]
</VirtualHost>

# 2) Add virtural host IP to your hosts file 
# /etc/hosts
127.0.0.1		$proj.dev

# 3) DB migration (Option)
> cd ../$proj
> php liber.php migrate

# 4)  Restart your apache
		
# 5) Try it by accessing http://$proj.dev
	
   
EOF;
	
}

function build($appName=null){
	global $pwd,$args;
	$cliName = array_shift($args);
	$appName = $appName | $cliName | "__APP_NAME__";
	//$files = glob($pwd."/modules/*.inc");
	$out = <<< EOF
<?php
/**
 *	@file: liber.php
 *	@author: Soyoes 2014/01/28
 *****************************************************************************/
require 'conf/conf.inc';
const APP_NAME = '$appName';
const LIBER_DIR = '$pwd';
const __SLASH__ = DIRECTORY_SEPARATOR;
define('APP_DIR', dirname(__FILE__));
define('IMAGE_DIR', APP_DIR . __SLASH__ . "webroot" . __SLASH__ . "images" . __SLASH__);
set_include_path(
get_include_path(). PATH_SEPARATOR
. LIBER_DIR .__SLASH__. 'modules'.__SLASH__.'utils'. PATH_SEPARATOR
. APP_DIR.__SLASH__.'delegate'.PATH_SEPARATOR
. APP_DIR.__SLASH__.'modules'.__SLASH__ 
);
EOF;
	
	$surfix = <<< EOF2

spl_autoload_register(function(\$class){
	if(!include_once \$class.'.inc')
		include_once \$class.'.php';
});
try{
	\$cli_args = array_slice(\$argv, 1);
	\$cli_cmd = array_shift(\$cli_args);
	if(php_sapi_name() == 'cli' || PHP_SAPI == 'cli'){
		\$cli_cmd="cli_".\$cli_cmd;
		if(function_exists(\$cli_cmd)){\$cli_cmd();}
	}else{
		REQ::dispatch();
	}
}catch(Exception \$e){
	error_log(\$e->getMessage());
	exit;
}
function cli_script(){
	global \$cli_args;
	\$f = array_shift(\$cli_args);
	if(!empty(\$f)){
		\$pwd=dirname(__FILE__);
		\$f = \$pwd."/scripts/\$f.php";
		include \$f;
	}
	exit;
}
function cli_migrate(){
	try{
		\$pwd=dirname(__FILE__);
		\$schemas = glob(\$pwd."/conf/schemas/*.ini") ;
		foreach (\$schemas as \$file){
			echo \$file."\\n";
			\$parts = explode("/",\$file);
			\$file = \$parts[count(\$parts)-1];
			\$parts = explode(".", \$file);
			\$schema = \$parts[0];
			echo \$schema."\\n";
			db_migrate(\$schema);
		}
		echo "DONE\\n";
	}catch(Exception \$e){
		echo "FAILED\\n";
	}
	exit;
}
function llog(\$label, \$value, \$toScreen=false){
	\$value = is_array(\$value)?json_encode(\$value,JSON_UNESCAPED_UNICODE|JSON_PRETTY_PRINT):\$value;
	if(\$toScreen){
		\$label = "<b>".\$label."</b>";
		\$value = "<pre>".\$value."</pre>";
	}
	if(REQ_MODE=="CLI"||\$toScreen){
		echo \$label.":".\$value.(\$toScreen?"<br>":"\n");
	}else{
		error_log(\$label.":".\$value."\n");
	}
}

?>

EOF2;
	$files = ['Core','Lang','Session','Caches','DB','Model','Render','Filter'];
	foreach($files as $f){
		//echo $f."\n";
		$str = file_get_contents($pwd."/modules/".$f.".inc");
		$fo  = '';
		$commentTokens = [T_COMMENT,T_DOC_COMMENT];
		$tokens = token_get_all($str);
		foreach ($tokens as $token) {
			if (is_array($token)) {
				if (in_array($token[0], $commentTokens))
					continue;
				$token = $token[1];
			}
			if( !preg_match('/^<\?php\s*$/', $token) 
				&& !preg_match('/^\s*\?>$/', $token)){
				$fo .= $token;
			}
		}
		$fo= preg_replace(['/(require_once LIBER_DIR\.\'modules\'\.__SLASH__.*)/',"/(^[\r\n]*|[\r\n]+)[\s\t]*[\r\n]+/"],['',"\n"],$fo);
		$out.=$fo;
	}
	$out = str_replace('error_log','llog',$out);
	$out.=$surfix;
	file_put_contents($pwd."/liber.php", $out);
}
