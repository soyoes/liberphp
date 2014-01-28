<?php
/**
 *	@file: liber.php
 *	@author: Soyoes 2014/01/28
 *****************************************************************************/
const APP_NAME = '__APP__NAME__';
const LIBER_DIR = '__LIBER__DIR__';
const __SLASH__ = DIRECTORY_SEPARATOR;
define('APP_DIR', dirname(__FILE__));

set_include_path(
get_include_path(). PATH_SEPARATOR
. APP_DIR.__SLASH__.'modules'.__SLASH__
. PATH_SEPARATOR . __SLASH__.'filters'.__SLASH__
. PATH_SEPARATOR . __SLASH__.'libs'.__SLASH__
. PATH_SEPARATOR . LIBER_DIR.__SLASH__. 'modules'
. PATH_SEPARATOR . LIBER_DIR .__SLASH__. 'modules'.__SLASH__.'utils'
);

spl_autoload_register(function($class){
	if(!include_once $class.'.inc')
		include_once $class.'.php';
});

try{
	REQ::dispatch();
}catch(Exception $e){
	error_log($e->getMessages());
	exit;
}
	
class Consts{
	
	static $lang 				= "jp";
	static $image_host 			= null;
	static $default_controller	= "top";
	static $default_action		= null;
	
		static $template_check		= true;		
	
		static $csrf_check			= false;
	static $csrf_lifetime		= 300;
	
	static $session_enable		= true;
	
	static $db_engine			= "mysql";
	static $db_host				= "127.0.0.1";
	static $db_port				= "3306";
	static $db_name				= "todos";
	static $db_user				= "root";
	static $db_pass				= "root";
	
	static $cache_hosts			= "localhost";
	static $cache_port			= "11211";
	
	static $filters				= [];
	
	static $schema_reg			= "regAt";
	static $schema_upd			= "updAt";
	
	
	
	static $db_regexp_op = ["mysql"=>"REGEXP","postgres"=>"~"];
	
	static $db_query_filters;
	
	static $arr_query_filters;
	
	static $query_filter_names = [
		"eq" 	=> "=",
		"ne" 	=> "!",
		"lt" 	=> "<",
		"gt"	=> ">",
		"le" 	=> "<=",
		"ge"	=> ">=",
		"in"	=> "[]",
		"nin" 	=> "![]",
		"bt" 	=> "()",
		"nb" 	=> "()",
		"l" 	=> "?",
		"nl" 	=> "!?",
		"m" 	=> "~",
		"nm" 	=> "!~",
		"mi" 	=> "~~",
		"nmi" 	=> "!~~"
	];
	
	static $error_codes = [
		"200"=>"OK",
		"201"=>"Created",
		"202"=>"Accepted",
		"204"=>"No Content",
		"301"=>"Moved Permanently",
		"302"=>"Found",
		"400"=>"Bad Request",
		"401"=>"Unauthorized",
		"403"=>"Forbidden",
		"404"=>"Not Found",
		"413"=>"Request Entity Too Large",
		"414"=>"Request-URI Too Large",
		"415"=>"Unsupported Media Type",
		"419"=>"Authentication Timeout",
		"500"=>"Internal Server Error",
		"501"=>"Not Implemented"];
	
	static function init(){
		self::$db_engine = strtolower(self::$db_engine);
		if(empty(self::$default_action)) self::$default_action = strtolower($_SERVER["REQUEST_METHOD"]);
	}
}
Consts::init();
function assign($key, $value){
	$render = REQ::getRender();
	$render->assign($key, $value);
}
function render($arg1=false, $arg2=false){
	switch(REQ::getFormat()){
		case "json":return render_json($arg1);
		case "text":return render_text($arg1);
		default:return render_html($arg1,$arg2);			
	}
}
function render_html($templateName=null, $datas=array()){
	list($render,$render_layout) = [REQ::getRender(), REQ::getRenderLayout()];
	$appName = str_has(REQ::getURI(),"/".APP_NAME."/")?"/".APP_NAME:"";
	$render->assign('TITLE',APP_NAME);
	$render->assign('CONTENTS', REQ::getContent());
	header('Content-type: text/html; charset=UTF-8');
	if (!isset($templateName))
		$templateName = REQ::getController()."_".REQ::getAction().".html";
	if(!str_ends($templateName, ".html"))
		$templateName .= ".html";
	$render->render($templateName,$datas,$render_layout);
	REQ::setResponseBody("true");
	if(isset($_REQUEST["after_wrapper"]))
		after_wrapper(REQ::getParams());
}
function render_json($data,$code="200"){
	$body = json_encode($data);
	header('Content-type: application/json; charset=UTF-8');
	REQ::setResponseBody(json_encode($data));
	echo $body;
}
function render_text($text,$code="200"){
	header('Content-type: text/plain; charset=UTF-8');
	REQ::setResponseBody($text);
	echo $text;
}
function render_default_template(){
	$path = APP_DIR.__SLASH__."views".__SLASH__.REQ::getClientTemplateType();
	$ns = REQ::getNamespace();
	if($ns!=""){
		$path .= "/".str_replace(".","/",$ns);
	}
	$template_file = REQ::getController().'_'.REQ::getAction().'.html';
		if(file_exists($path."/".$template_file)){		render_html($template_file);
	}else{
				error(400,"json","action does not exist ");
	}
	exit;
}
function error($code, $contentType="text", $reason=""){
	$msg = Consts::error_codes("".$code);
	header("HTTP/1.1 $code $msg", FALSE);
	switch($contentType){
		case "json":
			header("Content-type: application/json; charset=utf-8");
			echo '{"error":"'."$code $msg. $reason".'"}';
			break;
		case "html":
			header("Content-type: text/html; charset=utf-8");
						break;
		default:			header("Content-type: text/plain; charset=utf-8");
			$reason = $contentType=="text" ? $reason:$contentType;
			echo "$code ERROR: $msg. $reason";
			break;
	}
	exit;
}
function redirect($url,$method="GET",$data=array()) {
	$appName = str_has($_SERVER['REQUEST_URI'],APP_NAME."/")?
	APP_NAME."/":"";
	$redirectUrl = str_starts($url, "http:") ||str_starts($url, "https:")?
	$url: "http://".$_SERVER["HTTP_HOST"]."/".$appName . $url;
	
	if("GET"==$method){
		echo "<html><script type='text/javascript'>window.location='".$redirectUrl."';</script></html>";
	}else{
				$render = new Smarty();
		$clientType = REQ::getClientTemplateType();
		$vdir = APP_DIR.__SLASH__."views".__SLASH__;
		$path = $vdir.'templates'.__SLASH__."pc".$adminSurfix;
		$render->setTemplateDir($path);
		$render->setCompileDir($vdir.'templates_c');
		$render->setCacheDir($vdir.'cache');
		$render->setConfigDir($vdir.'configs');
		if(!str_starts($url,'http') && !Strings::startsWith($url,'https')){
			$url = self::getURL($url);		}
		$render->assign('uri',$url);
		$render->assign('data',$data);
		$render->assign("appPath", "http://".$_SERVER["HTTP_HOST"]."/".APP_NAME);
		$render->assign("jsPath", "http://".$_SERVER["HTTP_HOST"]."/".APP_NAME."/js");
		$render->display('_post.tpl');
	}
	exit();
}
function call($url, $method, array $data, array $options=array()){
	$defaults = $method == "POST" || $method == "PUT" ?
	[	CURLOPT_POST => 1,
		CURLOPT_HEADER => 0,
		CURLOPT_URL => $url,
		CURLOPT_FRESH_CONNECT => 1,
		CURLOPT_RETURNTRANSFER => 1,
		CURLOPT_FORBID_REUSE => 1,
		CURLOPT_TIMEOUT => 4,
		CURLOPT_POSTFIELDS => http_build_query($data)
	]:[
		CURLOPT_URL => $url. (strpos($url, '?') === FALSE ? '?' : ''). http_build_query($data),
		CURLOPT_HEADER => 0,
		CURLOPT_RETURNTRANSFER => TRUE,
		CURLOPT_TIMEOUT => 4
	];
	$ch = curl_init();
	curl_setopt_array($ch, ($options + $defaults));
	if( ! $result = curl_exec($ch)){
		trigger_error(curl_error($ch));
	}
	curl_close($ch);
	return $result;
}
function T($key,$replace=array(),$lang="jp"){
	$filename = null;
	if(str_has($key, ".")){
		list($filename,$key) = explode(".", $key, 2);
	}else{
		$filename = strtolower(REQ::getController());
	}
	global $TEXTS;	if(!isset($TEXTS) || !isset($TEXTS[$filename])){
		$file = APP_DIR.__SLASH__."conf".__SLASH__."text".__SLASH__.$lang."/".$filename.".json";
		if (file_exists($file)){
			$texts = file_get_contents($file);
			$texts = preg_replace(['/\s*/','/\'/'], ["",'"'], $texts);
			$texts = json_decode($texts, true);
			if(!isset($TEXTS))
				$TEXTS = array($filename=>$texts);
			else
				$TEXTS[$filename] = $texts;
		}
	}
	$text = "";
	if(!empty($TEXTS[$filename])){
		$text = $TEXTS[$filename][$key];
	}
	if(!empty($text) && !empty($replace)){
		if(!is_hash($replace)){
			$replace = Arrays::toSerializedHash($replace);
		}
		foreach ($replace as $key=>$v){
			$text = str_replace("%$key%", $v, $text);
		}
	}
	return $text;
}
function paginate($page,$total,$perPage,$center=null){
	if($center==null)
		$center = $page;
	$pos = $center%$perPage;
	$max = ceil($total/$perPage);
	$pages = array();
	if($max < 10){
		for($i=1; $i<=$max; $i++)
			$pages[]=array("label"=>$i,"page"=>$i);
	}else{
		$left = $center-3;
		$right = $center+3;
				if($left<1)
			return paginate($page,$total,$perPage,$center-$left+1);
				if($right>$max)
			return paginate($page,$total,$perPage,$center-($right-$max));
				for($i=$left;$i<=$right;$i++){
			$pages[]=array("label"=>$i,"page"=>$i);;
		}
				switch ($max-$right){
			case 0:					break;
			case 1:					$pages[]=array("label"=>$max,"page"=>$max);
				break;
			default:
				$pages[]=array("label"=>"...","page"=>$right+1);
				$pages[]=array("label"=>$max,"page"=>$max);
				break;
		}
				switch ($left-1){
			case 0:				break;
			case 1:				array_unshift($pages, array("label"=>1,"page"=>1));
				break;
			default:
				array_unshift($pages, array("label"=>"...","page"=>2));
				array_unshift($pages, array("label"=>1,"page"=>1));
				break;
		}
	}
	return $pages;
}
class REQ {
	
	private static $resources = null;
	
	private static $conf = null;
	
	private static $db = null;
	private static $render = null;
	private static $render_path = null;
	private static $render_contents = [];
	private static $render_layout = "_layout.html";
	
	private static $token = null;
	
	private static $dispatched = false;
	private static $interrupted = false;
	private static $redirecting = null;
	
	private static $uri = null;
	private static $namespace = "";
	private static $controller = null;
	private static $action = null;
	private static $format = "html";
	private static $params = [];
	private static $client_type = "pc";
	private static $client_template_type = "pc";
	private static $rest_schema = null;
	
		private static $response_body = null;
	
	private function __construct(){}
	
	
	static function getConf($keypath=null, $defaultValue=null){
		if(!isset(self::$conf)){
			self::$conf = cache_get("APPCONF", function($key){
				$content = parse_ini_file(APP_DIR.__SLASH__.'conf'.__SLASH__.'conf.ini', true);
				$conf = [];
				foreach($content as $seg=>$items){
					foreach($items as $key=>$value){
						$name = strtoupper($seg."_".$key);
						$conf[$name] = $value;
					}
				}
				return $conf;
			});
		}
		if (isset($keypath)){
			return (self::$conf[strtoupper($keypath)])?self::$conf[strtoupper($keypath)]:$defaultValue;
		}
		return self::$conf;
	}
	
	static function getDB(){return self::$db;}
	static function setDB($dbh){if(isset($dbh) && $dbh instanceof PDO)self::$db=$dbh;}
	static function getRender($path=null){
		if(!isset(self::$render)){
			$clientType = self::$client_template_type;
			if ($path==null){
				$path = APP_DIR.__SLASH__."views".__SLASH__.$clientType;
				$path = self::$namespace==""? $path:$path."/".self::$namespace;
			}
			$render = Render::factory($path);
			$render->assign('CLIENT_TYPE',$clientType);
			$render->assign('controller',self::$controller);
			$render->assign('action',self::$action);
			self::$render_path = $path;
			self::$render = $render;
		}
		return self::$render;
	}
	static function getRenderPath(){return self::$render_path;}
	static function setRenderPath($path){if(isset($path) && is_string($path))self::$render_path=$path;}
	static function getRenderLayout(){return self::$render_layout;}
	static function setRenderLayout($path){if(isset($path) && is_string($path))self::$render_layout=$path;}
	static function getContent(){return self::$render_contents;}
	static function addContent($content){if(isset($content) && is_string($content))self::$render_contents[]=$content;}
	
	static function getClientType(){return self::$client_type;}
	static function getClientTemplateType(){return self::$client_template_type;}
	static function getNamespace(){return self::$namespace;}
	static function getController(){return self::$controller;}
	static function getAction(){return self::$action;}
	static function getFormat(){return self::$format;}
	static function getURI(){return self::$uri;}
	
	static function setResponseBody($body){if(isset($body) && is_string($body))self::$response_body=$body;}
	
	
	
	public static function dispatch(){
		if(self::$dispatched===true)return;
		self::parse_uri();
		try{
			$filters = $_REQUEST["filters"];
			$size = count($filters);
			$filterCls = [];
			if(Consts::$session_enable)
				Session::start();
						if(Consts::$csrf_check){
				if(isset(self::$token) && self::$token==$_SESSION["@token_old"]){
					unset($_SESSION["@token_old"]);
					return error(419);
				}else if(!isset(self::$token) || $_SESSION["@token"] != self::$token)  
					return error(400,"XS");
			}
			for ($token=$size*(-1); $token<=$size; $token++){
				if(true===self::$interrupted)
					break;
				if ($token == 0){					if(isset(self::$rest_schema)){
						self::rest_process();
					}else{
						self::process();
					}
				}else if($size>0){					$nextIdx = $token < 0 ? $size + $token : $size - $token ;
					$filterName = $filters[$nextIdx];
					if(!empty($filterName)){
						$filter = array_key_exists($filterName, $filterCls)? $filterCls[$filterName]: Filter::factory($filterName);
						if(!array_key_exists($filterName, $filterCls)){
							$filterCls[$filterName] = $filter;
						}
						($token<0) ? $filter->before() : $filter->after();
					}
				}
			}
			
			if(isset(self::$redirecting)){
				redirect(self::$redirecting);
			}
						if(!isset(self::$response_body)){
				render_html();
			}
					}catch(Exception $e){
			error_log("exec exception");
			print $e->getMessage();
		}
				exit;
	}
	
	
	private static function parse_uri(){
		$host =$_SERVER["SERVER_NAME"];
		$uri = htmlEntities($_SERVER["REQUEST_URI"], ENT_QUOTES|ENT_HTML401);
		$uri = preg_replace(['/\sHTTP.*/','/(\/)+/','/\/$/','/^[a-zA-Z0-9]/'], ['',"/","","/$1"], $uri);
				$parts = parse_url("http://".$host.$uri);
		$uri = $parts["path"];
		$fmts = ['json','bson','text','html','csv'];
				$params = [];
		
		if(isset($parts["query"]))
			parse_str($parts["query"],$params);
		list($uri, $ext) = explode(".", $uri);
		$specifiedFmt = in_array($ext,$fmts);
		if($ext==1||$ext==""||$specifiedFmt){			preg_match_all('/\/(?P<digit>\d+)\/*/', $uri, $matches);
			if(!empty($matches["digit"])){
				$params["@id"] = (int)$matches["digit"][0];
				$uri = preg_replace("/\/\d+\/*/", "", $uri);
			}
			self::$uri = $uri;
			self::$params = $params;
			if($specifiedFmt) self::$format = $ext;
			self::parse_user_agent();
			self::parse_rest_uri();
		}else{						self::$uri = "/webroot".$uri.".$ext";
		}
	
	}
	
	private static function parse_rest_uri(){
		$uri = preg_replace("/^\//","",self::$uri);
		$uparts =ds_remove(explode("/",$uri), "");
	
		$method = strtolower($_SERVER["REQUEST_METHOD"]);
	
		if ($method == "put" || $method == "delete") {
			parse_str(file_get_contents('php://input'), self::$params);
		}
		$target =($method=="post"||$method=="put")?$_POST: $_GET;
		foreach($target as $k=>$v)
			self::$params[$k] = $v;
		unset(self::$params["__URL__"]);
		$fmts = ['json','bson','text','html','csv'];
		foreach(self::$params as $k=>$v){
			if($k=="@format" && in_array($v, $fmts))
				self::$format = $v;
			if($k=="@token")
				self::$token = $v;				
			self::$params[$k] = htmlEntities($v); 		}
		unset(self::$params["@format"]);
		unset(self::$params["@token"]);
	
		$resources = self::load_resources();
	
		list($namespace, $controller, $action) =
		["",Consts::$default_controller,Consts::$default_action];
		$len = count($uparts);
		
				if(empty($uparts)){$uparts=[$controller,$action];}
		if(count($uparts)==1)$uparts[]=$action;
	
		if($uri==""){
			self::$uri = $controller;
		}else if(in_array($uri,$resources["namespaces"])){			$namespace = $uri;
		}else if(in_array($uri,$resources["controllers"])){			$namespace = join("/",array_slice($uparts, 0 , $len-1));
			$controller = $uparts[$len-1];
					}else if(in_array(join("/",array_slice($uparts, 0 , $len-1)),$resources["controllers"])){			$namespace = join("/",array_slice($uparts, 0 , $len-2));
			$controller = $uparts[$len-2];
			$action = $uparts[$len-1];
					}else{			if(str_starts($uri,"@")){				$uri = substr($uparts[0],1);
				if(in_array($uri,$resources["schemas"])){
					$controller = "@REST";
					self::$rest_schema = $uri;
					$action = $method;
				}
			}else if(in_array(self::$client_type."/".join("_",$uparts),$resources["views"])){								render_html(join("_",$uparts).".html");
				exit;
			}else error(400,"$uri");
		}
		self::$namespace = $namespace;
		self::$controller = $controller;
		self::$action = $action;
		error_log("ns=$namespace, ctrl=$controller, act=$action");
	}
	
	private static function parse_user_agent(){
		$ua = $_SERVER['HTTP_USER_AGENT'];
				$type = "pc";
		if(preg_match('/(curl|wget|ApacheBench)\//i',$ua))
			$type = "cmd";
		else if(preg_match('/(iPad|MSIE.*Touch|Android)/',$ua)) 			$type = "pad";
		else if(preg_match('/(iPhone|iPod|(Android.*Mobile)|BlackBerry|IEMobile)/i',$ua))
			$type = "sm";
		
				if(preg_match('/Googlebot|bingbot|msnbot|Yahoo|Y\!J|Yeti|Baiduspider|BaiduMobaider|ichiro|hotpage\.fr|Feedfetcher|ia_archiver|Tumblr|Jeeves\/Teoma|BlogCrawler/i',$ua))
			$type = "bot";
		else if(preg_match('/Googlebot-Image|msnbot-media/i',$ua))
			$type = "ibot";
		self::$client_type = $type;
		
		$vtypes = self::load_resources()["view_types"];
		self::$client_template_type = in_array($type,$vtypes)? $type:"pc";	}
	
	
	private static function load_resources(){
		if(self::$resources) 
			return self::$resources;
		self::$resources = cache_get("APP_RESOURCES", function($key){
			$ctrldir = APP_DIR.__SLASH__."controllers";
			exec("find $ctrldir",$res);
			$namespaces = array_unique(array_map(function($e){
				return strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."controllers")."/",'/\/(.*)\.inc$/',"/^\//"],["","",""], $e));
			},array_slice($res,0)));
			$controllers = array_unique(array_map(function($e){
				return strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."controllers")."/",'/\.inc$/',"/^\//"],["","",""], $e));
			},array_slice($res,0)));
			$schemadir = APP_DIR.__SLASH__."conf".__SLASH__."schemas";
			exec("find $schemadir",$res2);
			$schemas = array_unique(array_map(function($e){
				return strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."conf".__SLASH__."schemas")."/",'/\.ini$/',"/^\//"],["","",""], $e));
			},$res2));
			$vdir = APP_DIR.__SLASH__."views";
			exec("find $vdir",$res3);
			$views = array_unique(array_map(function($e){
				return strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."views".__SLASH__)."/",'/\.html$/',"/^\//"],["","",""], $e));
			},$res3));
			$view_types = glob($vdir.__SLASH__."*",GLOB_ONLYDIR);
			return ["namespaces" => ds_remove($namespaces, ""), "controllers" => ds_remove($controllers, ""),
			"schemas"=>ds_remove($schemas, ""),"views"=>ds_remove($views, ["","pc","sm","bot","ibot","pad","mail"]),
			"view_types"=>array_map(function($e){
				return strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."views".__SLASH__)."/","/^\//"],["",""], $e));
			},$view_types)];
		},false);
		return self::$resources;
	}
	
	
	private static function process($useSession=true){
		try {
						$ctrldir = APP_DIR.__SLASH__."controllers".__SLASH__;
			$controller_dir = isset(self::$namespace) && self::$namespace!="" ? $ctrldir.self::$namespace."/":$ctrldir;
			$file_path = $controller_dir.self::$controller.".inc";
			require_once $file_path;
						if(isset($auth_actions)){
				$auth_actions = str_replace(" ","",$auth_actions);
				if($auth_actions=="*" || str_has(",".$auth_actions.",", ",$actionName,")){
					$res = Auth::check();
					if($res===false){
						return error("401","json");
					}
				}
			}
						$action = self::$action;
			$exec = function($action){
				$has_wrapper =  !isset($exclude_wrappers) || !in_array($action, $exclude_wrappers);
				if (function_exists("before_wrapper") && $has_wrapper)
					before_wrapper(self::$params);
				$action(self::$params);
				if (function_exists("after_wrapper")  && $has_wrapper){
					if(self::$format!="html"){						after_wrapper(self::$params);
					}else{
						$_REQUEST["after_wrapper"] = true;
					}
				}
			};
			if(function_exists($action)){				$exec($action);
			}else if(str_starts($action,"test_") && function_exists(str_replace("test_","",$action))){				$action = str_replace("test_","",$action);
				$file = APP_DIR.__SLASH__."test".__SLASH__.self::$controller.".json";
				if (file_exists($file)){
					$content = file_get_contents($file);
					$content = str_replace("\n","",str_replace("\t", "", (string)$content));					$cases = json_decode($content, true);
					if(isset($cases) && isset($cases[$action])){
						$cases = $cases[$action];
						foreach ($cases as $case){
							Tests::exec(self::$controller, $action, $case);
						}
					}
				}else{
					echo "<H1>Error:</H1><br>Test case does not exist <br> Pls add case to /test/[CONTROLLER].json<br>And specify your case by your ACTION_NAME";
					error(400,"html");
				}
				self::$interrupted = true;
			}else{				return render_default_template();
			}
		} catch(Exception $e) {
			echo $e->getMessage();
			throw new Exception($controllerName.',Controller not found');
		}
	}
	
	static function clean(){
		unset(self::$params);
	}
	
	
	private static function rest_process(){
		$schema = self::$rest_schema;
				require_once APP_DIR.__SLASH__."restful".__SLASH__.$schema.".inc";
				$pk = db_schema($schema)["general"]["pk"];
		$params = self::$params;
		if(isset($params[$pk]) && !isset($params["@id"]))
			$params["@id"] = $params[$pk];
		$delegate = "rest_".$schema."_".self::$action;
		if(!function_exists($delegate)){
			switch(strtolower($_SERVER["REQUEST_METHOD"])){
				case "get"	:return self::rest_get($schema,$params);
				case "post"	:return self::rest_post($schema,$params);
				case "put"	:return self::rest_put($schema,$params);
				case "delete":return self::rest_delete($schema,$params);
				default : return error(401,"RESTful ERROR : Sorry, You are not permited to do that.");
			}
		}else{
			$re  = $delegate($schema, $params);
			if(!$re)
				error(401, "RESTful ERROR : Sorry, You are not permited to do that.");
		}
	}
	
	private static function rest_get($schema, $params){
		$res = (isset($params["@id"]))?
			db_find1st($schema, $params):
			db_find($schema, $params);
		render_json($res);
	}
	
	private static function rest_post($schema, $params){
		if(isset($params["@id"])){
			error(400,"RESTful ERROR : Sorry, You can't use RESTful POST with @id, try PUT for update or using normal controllers");
		}else{
			return render_json(db_save($schema, $params, true));
		}
	}
	private static function rest_put($schema, $params){
		if(isset($params["@id"])){
			return render_json(db_save($schema, $params));
		}else{
			error(400,"RESTful ERROR : You must specify a @id to use RESTful PUT");
		}
	}
	private static function rest_delete($schema, $params){
		if(isset($params["@id"])){
			return render_json(db_delete($schema, $params));
		}else{
			error(400,"RESTful ERROR : You must specify a @id to use RESTful DELETE");
		}
	}
}

function str_has($haystack, $needle){
	if(!is_string($haystack)||!is_string($needle))return false;
	return strpos($haystack, $needle) !== false;
}
function str_starts($haystack,$needle,$case=true) {
	if($case){return (strcmp(substr($haystack, 0, strlen($needle)),$needle)===0);}
	return (strcasecmp(substr($haystack, 0, strlen($needle)),$needle)===0);
}
function str_ends($haystack,$needle,$case=true) {
	if($case){return (strcmp(substr($haystack, strlen($haystack) - strlen($needle)),$needle)===0);}
	return (strcasecmp(substr($haystack, strlen($haystack) - strlen($needle)),$needle)===0);
}
function hex2str($hex){
	$string='';
	for ($i=0; $i < strlen($hex)-1; $i+=2){
		$string .= chr(hexdec($hex[$i].$hex[$i+1]));
	}
	return $string;
}
function str2hex($string){
	$hex='';
	for ($i=0; $i < strlen($string); $i++){
		$hex .= dechex(ord($string[$i]));
	}
	return $hex;
}
function is_email($email){
	return false !== filter_var( $email, FILTER_VALIDATE_EMAIL );
}
function is_ip($str){
	return false !== filter_var( $email, FILTER_VALIDATE_IP);
}
function is_url($str){
	return false !== filter_var( $str, FILTER_VALIDATE_URL );
}
function is_hash($arr){
	return !empty($arr) && is_array($arr) && array_keys($arr) !== range(0, count($arr) - 1);
}
function hash_incr($data, $key, $amount){
	$v = self::get($data,$key,true,0);
	$v += $amount;
	return self::set($data, $key, $v);
}
function hash_set(&$data, $keyPath, $val){
	$paths = explode(".", $keyPath);
	$o = &$data;
	$current_path = "";
	$path_size = count($paths);
	$key = $paths[0];
	$org = isset($data[$key])? $data[$key]: null;
	for ($i=0; $i<$path_size; $i++){
		$path = $paths[$i];
		if (is_string($o) && (str_starts($o, "{") || Strings::startsWith($o, "[")))
			$o = json_decode($o,true);
		if ($i == $path_size-1){
			$o[$path] = $val;
		}else{
			if (!isset($o[$path]))
				$o[$path] = [];
			$o = &$o[$path];
		}
	}
	return ["key"=>$key, "val"=>$data[$key], "org"=>$org];
}
function hash_get(&$data, $keyPath, $autoCreate=true, $defaultValue=null){
	if (empty($data)) {
		if($autoCreate){
			hase_set($data, $keyPath, $defaultValue);
		}else
			return $defaultValue;
	}
	$paths = explode(".", $keyPath);
	$o = $data;
	$current_path = "";
	while (count($paths)>1){
		$path = array_shift($paths);
		if (is_string($o) && (str_starts($o, "{") || Strings::startsWith($o, "[")))
			$o = json_decode($o,true);
		if (!isset($o[$path])){
			return $defaultValue;
		}
		$o = $o[$path];
	}
	if (is_string($o) && (str_starts($o, "{") || Strings::startsWith($o, "[")))
		$o = json_decode($o,true);
	$key = array_pop($paths);
	if(!isset($o[$key]))
		return $defaultValue;
	return $o[$key];
}
function arr2hash($arr, $keyName, $valueName=null){
	$hash = [];
	foreach ($arr as $e){
		$hash["".$e[$keyName]] = $valueName==null ? $e : $e[$valueName];
	}
	return $hash;
}
function ds_remove(&$arr, $conditions, $firstOnly=FALSE){
	if(!isset($conditions)||(is_array($conditions)&&count($conditions)==0))
		return $arr;
	$res = array();
	$found = false;
	foreach ($arr as $el){
		$match = TRUE;
		if($firstOnly && $found){
			$match = FALSE;
		}else{
			if(is_hash($conditions)){
				foreach ($conditions as $k=>$v){
					if (!isset($el[$k]) || $el[$k]!=$v){
						$match = FALSE;
						break;
					}
				}
			}else if(is_array($conditions)){
				$match = in_array($el, $conditions);
			}else if(is_callable($conditions)){
				$match = $conditions($el);
			}else{
				$match = ($el===$conditions);
			}
		}
		if (!$match){
			$res[]=$el;
			$found = true;
		}
	}
	$arr = $res;
	return $res;
}
function ds_find($arr, $opts,$firstOnly=false){
	if(empty(Consts::$arr_query_filters))
		Consts::$arr_query_filters = [
		"=" 	=> function($o,$k,$v){return $o[$k]===$v;},
		"!" 	=> function($o,$k,$v){return $o[$k]!==$v;},
		"<" 	=> function($o,$k,$v){return $o[$k]<$v;},
		">" 	=> function($o,$k,$v){return $o[$k]>$v;},
		"<=" 	=> function($o,$k,$v){return $o[$k]<=$v;},
		">=" 	=> function($o,$k,$v){return $o[$k]>=$v;},
		"[]" 	=> function($o,$k,$v){return is_array($v)&&in_array($o[$k],$v);},
		"![]" 	=> function($o,$k,$v){return is_array($v)?!in_array($o[$k],$v):true;},
		"()" 	=> function($o,$k,$v){return is_array($v) && count($v)==2 && $o[$k]>=min($v[0],$v[1]) && $o[$k]<=max($v[0],$v[1]);},
		"!()" 	=> function($o,$k,$v){return !is_array($v) || count($v)<2 || $o[$k]<min($v[0],$v[1]) || $o[$k]>max($v[0],$v[1]);},
		"?"  	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && str_has($o[$k], $v); },
		"!?"  	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !str_has($o[$k], $v); },
		"~" 	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && preg_match("/".$v."/", $o[$k]);},
		"!~"	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !preg_match("/".$v."/", $o[$k]);},
		"~~" 	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && preg_match("/".$v."/i", $o[$k]);},
		"!~~"	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !preg_match("/".$v."/i", $o[$k]);},
	];
	if(empty($opts))return false; 
	$res = null;
	foreach ($arr as $a){
		$match = true;
		foreach ($opts as $k=>$v){
			$cmd = strstr($k, '@');
			$func = Consts::$arr_query_filters[$cmd];
			if (!$func($a[$k],$v)){
				$match = false;break;
			}
		}
		if($match){
			if($firstOnly) return $a;
			$res[] = $a;
		}
	}
	return $res;
}
function ds_sort($arr, $sortKey=null, $sortOrder=1, $comparator=null){
	if(isset($sortKey)){
		if($comparator==null){
			$cfmt = '$av=$a["%s"];if(!isset($av))$av=0;$bv=$b["%s"];if(!isset($bv))$bv=0;if($av==$bv){return 0;} return is_string($av)?strcmp($av,$bv)*%d:($av>$bv)?-1*%d:1*%d;';
			$code = sprintf($cfmt, $sortKey, $sortKey, $sortOrder, $sortOrder,$sortOrder);
			$cmp = create_function('$a, $b', $code);
			usort($arr, $cmp);
		}else{
			usort($arr, $comparator);
		}
		return $arr;
	}else{
		asort($arr);
		return $arr;
	}
}
function ms(){
    list($usec, $sec) = explode(" ", microtime());
    return ((int)((float)$usec*1000) + (int)$sec*1000);
}

class Session {
	static function start(){
				
		session_start();
		$ua = $_SERVER['HTTP_USER_AGENT'];
		$time = time();
		if(!isset($_COOKIE["sid"])){
			setcookie("sid", md5($_SERVER['REMOTE_ADDR']."|".$ua), $time+86400*30, "/");
			$_SESSION["IP"] = $_SERVER['REMOTE_ADDR'];
			$_SESSION["UA"] = $ua;
			$_SESSION["ISSUED_HOST"] = $_SERVER["SERVER_NAME"];
			$_SESSION["ISSUED_AT"] = $time;
		}else{
			if($_COOKIE["sid"]!=md5($_SESSION["IP"]."|".$_SESSION["UA"])
				|| $time-(isset($_SESSION["ISSUED_AT"])?$_SESSION["ISSUED_AT"]:0)>=Consts::$session_lifetime){
								setcookie("sid", "", 1);
				unset($_COOKIE["sid"]);
				session_unset();
				session_destroy();
				return self::start();
			}
		}
				if(Consts::$csrf_check){
			$span = Consts::$csrf_lifetime;
			$expired = isset($_SESSION['@token']) && $time-(isset($_SESSION['@token_time'])?$_SESSION['@token_time']:0)>=$span;
			if($expired) $_SESSION["@token_old"] = $_SESSION["@token"];
			if(!isset($_SESSION['@token']) || $expired){
				$_SESSION['@token'] = md5(uniqid(rand(), TRUE));
				$_SESSION['@token_time'] = $time;
			}
		}
	}
	
}
function cache_get($key, $nullHandler=null,$sync=true){
	$k = APP_NAME."::".$key;
	$res = apc_fetch($k);
	if((!$res || !isset($res)) && $sync){
		$res = mc_get($key);
	}
	if(!$res && isset($nullHandler)){
		$res = $nullHandler($key);
		if(isset($res)&&$res!=false){
			cache_set($key, $res, 3600, $sync);
		} 
	}
	return $res;	
}
function cache_set($key, $value, $time=3600, $sync=true){
	$k = APP_NAME."::".$key;
	$s = apc_store($k, $value, $time);
	return ($time && $sync)?mc_set($key,$value,$time):$s;
}
function cache_del($key,$sync=true){
	$k = APP_NAME."::".$key;
	$r = apc_delete($k);
	return ($sync)?mc_del($key):$r;
}
function cache_inc($key,$amount=1,$sync=false){
	$k = APP_NAME."::".$key;
	$res = apc_inc($k, $amount);
	return ($sync)?mc_inc($key,$amount):$res;
}
function cache_dump(){
	return @apc_bin_dumpfile([],null, APP_DIR.__SLASH__."tmp".__SLASH__."apc.data");
}
function cache_load(){
	return @apc_bin_loadfile(APP_DIR.__SLASH__."tmp".__SLASH__."apc.data");
}
function mc_conn(){
	if(empty(Consts::$cache_hosts)){
		return false;
	}else{
		$conn = new Memcached(APP_NAME);
		$hosts=explode(",", Consts::$cache_hosts);
		$ss = $conn->getServerList();
		if (empty ( $ss )) {
			$conn->setOption(Memcached::OPT_RECV_TIMEOUT, 1000);
			$conn->setOption(Memcached::OPT_SEND_TIMEOUT, 1000);
			$conn->setOption(Memcached::OPT_TCP_NODELAY, true);
			$conn->setOption(Memcached::OPT_SERVER_FAILURE_LIMIT, 50);
			$conn->setOption(Memcached::OPT_CONNECT_TIMEOUT, 500);
			$conn->setOption(Memcached::OPT_RETRY_TIMEOUT, 300);
			$conn->setOption(Memcached::OPT_DISTRIBUTION, Memcached::DISTRIBUTION_CONSISTENT);
			$conn->setOption(Memcached::OPT_REMOVE_FAILED_SERVERS, true);
			$conn->setOption(Memcached::OPT_LIBKETAMA_COMPATIBLE, true);
			foreach ($hosts as $host){
				list($h,$p) = explode(":",trim($host));
				$conn->addServer ($h, isset($p)&&$p!=""?(int)$p:11211, 1 );
			}
		}
		return $conn;
	}
}
function mc_get($key){
	$conn = mc_conn();
	$k = APP_NAME."::".$key;
	if($conn)
		return $conn->get($k);
	return false;
}
function mc_gets($keys){
	$conn = mc_conn();
	$ks = array_map(function($e){return APP_NAME."::".$k;}, $keys);
	if($conn)
		return $conn->getMulti($ks);
	return false;
}
function mc_set($key, $value, $time=3600){
	$k = APP_NAME."::".$key;
	$conn = mc_conn();
	return ($conn)? $conn->set($k,$value,$time):false;
}
function mc_sets($datas,$time=3600){
	$ds = [];
	foreach($datas as $k=>$v){
		$ds[APP_NAME."::".$k] = $v;		
	}
	$conn = mc_conn();
	return ($conn)? $conn->setMulti($ds,$time):false;
}
function mc_del($key){
	$k = APP_NAME."::".$key;
	$conn = mc_conn();
	return ($conn)? $conn->delete($k,$time):false;
}
function mc_inc($key,$amount=1){
	$k = APP_NAME."::".$key;
	$conn = mc_conn();
	return ($conn)? $conn->increment($k,$amount):false;
}

if (!apc_load_constants("DB_FMTS")){
	apc_define_constants("DB_FMTS", [
	'FORMAT_CREATE_DB' 			 => "CREATE TABLE `%s`.`%s` ( %s PRIMARY KEY (`%s`)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_CREATE_DB_MULTI_KEY' => "CREATE TABLE `%s`.`%s` ( %s CONSTRAINT %s PRIMARY KEY (%s)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_INSERT_DB' 			 => "INSERT %s INTO `%s` (%s) VALUES(%s);",
	'FORMAT_UPDATE_DB'			 => "UPDATE `%s` SET %s WHERE `%s`=%s;"
	]);
}
function db_conn($opts=null){
	$db = REQ::getDB();
	if(!isset($db)){
		$conn_str = Consts::$db_engine.":host=".Consts::$db_host.";port=".Consts::$db_port.";dbname=".Consts::$db_name.";charset=utf8";
		$db = new PDO($conn_str,Consts::$db_user,Consts::$db_pass,
				[PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
		REQ::setDB($db);
	}
	return $db;
}
function db_query($sql, $datas=[], $useCache = false, $PDOOpt = PDO::FETCH_ASSOC) {
	try {
		$key = $sql;
		$isQeury = str_starts(strtolower($sql), "select");
		if ($useCache && $isQeury) {
			$key = $sql."-".json_encode($datas);
			$value = cache_get($key);
			if (isset ( $value ) && $value != false)
				return $value;
		}
		$db = db_conn();
		$statement = $db->prepare($sql);
				if ($statement->execute ($datas) == FALSE) {
			return null;
		}
		$res =$isQeury? $statement->fetchAll($PDOOpt):true;
		if ($useCache && $isQeury && $res != null) {
			cache_set($key, $res);
		}
		return $res;
	} catch ( PDOException $e ) {
		error_log ("DB ERR :". $e->getMessage() );
		error_log ("DB ERR SQL:". $sql );
		return null;
	}
}
function db_count($sql=null, $datas=[], $useCache=false){
	try {
		if($useCache){
			$value = cache_get($sql);
			if(isset($value) && $value!=false)
				return $value;
		}
		$db = db_conn();
		$sth = $db->prepare($sql);
		$sth->execute($datas);
		$res = $sth->fetchColumn();
		if($useCache && $res){
			cache_set($sql, $res);
		}
		return intval($res);
	} catch (PDOException $e) {
		error_log($sql);
		return -1;
	}
}
function db_attr($attr, $val){
	$db = db_conn();
	$db->setAttribute($attr, $val);
}
function db_find($table, $opts=[], $withCount=false){
	list($colStr, $optStr,$datas) = db_make_query($table, $opts);
	$sql = "SELECT ".$colStr." FROM ".$table.$optStr;
	error_log("SEARCH: ".$sql);
	$res = db_query($sql, $datas, $opts["useCache"]);
	if($withCount){
		$sql = "SELECT count(*) FROM ".$table.preg_replace("/ORDER BY\s*(.*)\s*([LIMIT .*]*)/", '${3}',$optStr);
		$cnt = db_count($sql, $datas, $opts["useCache"]);
		return ["count"=>$cnt,"result"=>$res];
	}else{
		return $res;
	}
}
function db_find1st($table, $opts=[]){
	$opts["limit"]=1;
	$res = db_find($table,$opts,false,$decodeBson);
	return isset($res)&&$res!=false ? $res[0]:false;
}
function db_import($table, $datas){
	if(!isset($table) || count($datas)==0)
		return false;
	
	$schema = db_schema($table)["schema"];
	
	$cols = [];
	foreach ($datas as $d){
		$cols = array_unique(array_merge($cols,array_keys($d)));
	}
	
	$regName = Consts::$schema_reg;
	$updName = Consts::$schema_upd;
	
	$hasRegStamp = array_key_exists($regName,$schema);
	if($hasRegStamp && !in_array($regName, $cols)) $cols[] = $regName;
	$hasTimestamp = array_key_exists($updName,$schema);
	if($hasTimestamp && !in_array($updName, $cols)) $cols[] = $updName;
	$sql = "INSERT IGNORE INTO ".$table." (`".join("`,`", $cols)."`) VALUES ";
	$time = time();
	foreach ($datas as $d){
		if($hasRegStamp){$d[$regName]=$time;}
		if($hasTimestamp){$d[$updName]=$time;}
		$vals = [];
		foreach ($cols as $c){
			$v = array_key_exists($c, $d) ? $d[$c] : null;
			$vals[]=db_v($v, $schema[$c]);
		}
		$sql.=" (".join(",", $vals)."), ";
	}
	$sql = substr($sql, 0, strlen($sql)-2);
	db_attr(PDO::ATTR_TIMEOUT,1000);
	if(isset($sql_dump_path)){
		$handle = fopen($sql_dump_path, "w+");
		fwrite($handle, $sql);
	}else{
				db_query($sql);
	}
}
function db_make_query($table, $opts=[], $omit=[]){
	
	if(empty(Consts::$db_query_filters))
		Consts::$db_query_filters = [
		"=" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`=:$k";},
		"!" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`!=:$k";},
		"<" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`<:$k";},
		">" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`>:$k";},
		"<=" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`<=:$k";},
		">=" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`>=:$k";},
		"[]" 	=> function($k,$v,&$o){if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` IN (".join(",",$vs).")";},
		"![]" 	=> function($k,$v,&$o){if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` NOT IN (".join(",",$vs).")";},
		"()" 	=> function($k,$v,&$o){if(count($v)!=2)return false; return "(`$k` BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";},
		"!()" 	=> function($k,$v,&$o){if(count($v)!=2)return false; return "(`$k` NOT BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";},
		"?"  	=> function($k,$v,&$o){if(!str_has($v,"%"))$v="%$v%";return "`$k` LIKE '".$v."'";},
		"!?"  	=> function($k,$v,&$o){if(!str_has($v,"%"))$v="%$v%";return "`$k` NOT LIKE '".$v."'";},
		"~" 	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "`$k` $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
				
		"!~"	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "`$k` NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
		"~~"	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "LOWER(`$k`) $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
		"!~~"	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "LOWER(`$k`) NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
			];
	if(!isset($table))return false;
	$colStr =  is_array($opts['fields'])? (count($opts['fields'])==0? "*" :join(",", $opts['fields']))
		 : (isset($opts['fields'])? $opts['fields']:'*');
	$optStr = "";
	$schemaDef = db_schema($table);
	$schema = $schemaDef["schema"];
	$pk = $schemaDef["general"]["pk"];
	$eg = Consts::$db_engine;
	$data = [];
	if(is_hash($opts)){
		foreach ($opts as $k => $v){
			if(in_array($k,$omit))continue;
			if($k=="@id")$k=$pk;
			if(is_callable($v)){ 
				list($key,$value)=explode("=", $k);
				$val = $v($key, $value);
				$optStr.= !empty($optStr) ? " AND $val ": " WHERE $val ";
			}else{
				list($k,$cmd) = explode("@",$k);
				if(array_key_exists($k, $schema)){
					$cmd = !isset($cmd)||$cmd=="" ? "=":$cmd;
					$cmd = strpbrk($cmd, 'begilmnqt') ? Consts::$query_filter_names[$cmd]:$cmd;
					error_log($cmd);
					$vStr =  Consts::$db_query_filters[$cmd]($k, $v, $data);
					if($vStr)
						$optStr.= !empty($optStr) ? " AND ". $vStr : " WHERE ".$vStr;
				}	
			}
		}
		if(!in_array("order",$omit) && !empty($opts["order"]))
			$optStr .= " ORDER BY ".$opts["order"];
		if(!in_array("limit",$omit) && !empty($opts["limit"]))
			$optStr .= " LIMIT ".$opts["limit"];
	}else {
		$optStr = !empty($opts)? " WHERE ". $opts : "";
	}
	return [$colStr,$optStr,$data];
}
function db_exists($table, $id){
	if(!isset($table) || !isset($id))
		return false;
	$pk = db_schema($table)["general"]["pk"];
	$entity = db_count("select count(*) from $table where `$pk`=:$pk",[$pk=>$id]);
	return $entity>0;
}
function db_delete($table, $opts){
	if(empty($opts))return false;
	list($cs,$optStr,$data) = db_make_query($table, $opts,["order","limit","fields"]);
	$sql = "DELETE FROM $table ".$optStr;
	return db_query($sql,$data);
}
function db_update($table, $data, $opts=[]){
	if(!isset($table) || empty($data) || !is_hash($data))
		return false;
	$vStrs = [];
	$schema = db_schema($table)["schema"];
	foreach($data as $k=>$v){
		$vStrs[]="`$k`=".db_v($v, $schema[$k]);
	} 
	$vStrs = join(",",$vStrs);
	list($cs,$optStr,$data) = db_make_query($table, $opts,["order","limit","fields"]);
	$sql = "UPDATE $table SET $vStrs ".$optStr;
	return db_query($sql,$data);
}
function db_migrate($table){
	$dbn = Consts::$db_name;
	$sql = sprintf("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '%s' AND table_name = '%s';", $dbn, $table);
	$res = db_count($sql,[], false);
	if ($res<=0 ){		$schema_def = db_schema($table);
		$schema = $schema_def["schema"];
		$pk = $schema_def["general"]["pk"];
		$colStmt = "";
		foreach ($schema as $col => $type){
			$colStmt .= "`".$col."` ".$type.", ";
		}
		$incStmt = "";
		$sql = "";
		if (str_has($pk, "|")){
			$parts = explode("|", $pk);
			$pkName = $parts[0];
			$keys = $parts[1];
			$sql = sprintf(FORMAT_CREATE_DB_MULTI_KEY, $dbn, $table, $colStmt, $pkName, $keys, $incStmt);
		}else{
			$sql = sprintf(FORMAT_CREATE_DB, $dbn, $table, $colStmt, $pk, $incStmt);
		}
		$res = db_query($sql);
	}
	echo "Created ".$table."</br>\n";
}
function db_save($table, $data, $returnId=false, $bson=false){
	if(!isset($table) || !is_hash($data) || empty($data))return false;
	$regName = Consts::$schema_reg;
	$updName = Consts::$schema_upd;
	
	$schema_def = db_schema($table);
	$schema = $schema_def["schema"];
	$pk = $schema_def["general"]["pk"];
	$id = isset($data[$pk]) ? $data[$pk] : null;
	if(array_key_exists($updName,$schema))
		$data[$updName] = Dates::format();
	$sql = "";
	$isUpdate = isset($id) && db_exists($table, $id);
	$qdatas = [];
	if ($isUpdate){			cache_del($table."_".$id);
		foreach ($data as $col => $val){
			if($col==$pk || !isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			$colStmt .= "`$col`=:$col ";
			$qdatas[$col]=db_v($val, $schema[$col], $bson);
		}
		$sql = sprintf(FORMAT_UPDATE_DB, $table, $colStmt, $pk, db_v($id));
	}else{								if(array_key_exists($regName,$schema) && !isset($data[$regName]))
			$data[$regName] = Dates::format();
		foreach ($data as $col => $val){
			if(!isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			if(!empty($valStmt))$valStmt .= ",";
			$colStmt .= "`$col`";
			$valStmt .=":$col";
			$qdatas[$col]=db_v($val, $schema[$col], $bson);
		}
				$sql = sprintf(FORMAT_INSERT_DB, $ignore, $table, $colStmt, $valStmt);
	}
	try {
		if($returnId==true && !$isUpdate) {
			echo ($sql."\n");
			var_dump($qdatas);
			$res = db_trans([$sql, "SELECT LAST_INSERT_ID() as 'last_id'"],[$qdatas]);
			$data["id"] = $res[0]["last_id"];
		}else{
			db_query($sql,$qdatas);
		}
		return $data;
	} catch (PDOException $e) {
		error_log($sql);
		return false;
	}
}
function db_schema($schemaName){
	$schemaDef = cache_get("SCHEMA_$schemaName", function($key){
		$schemaname = str_replace("SCHEMA_","",$key);
		$schemaDef = @parse_ini_file(APP_DIR.__SLASH__."conf".__SLASH__."schemas".__SLASH__.$schemaname.".ini", true);
		if(!$schemaDef){
						$schemaDef = @parse_ini_file(LIBER_DIR."common/schemas/".str_replace("liber_","",$schemaname).".ini", true);
		}
		return $schemaDef;
	},false);
	return $schemaDef;
}
function db_trans($querys,$datas){
	if(!isset($querys))
		return false;
	$db = db_conn();
	$mod = $db->getAttribute(PDO::ATTR_ERRMODE);
	$db->setAttribute( PDO::ATTR_AUTOCOMMIT, 0 );
	$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	$db->beginTransaction();
	$cnt = 0;
	$res = true;
	try{
		if(is_callable($querys)){
			$cnt = $querys($db);
		}else if(is_array($querys)){
			$i=0;
			foreach($querys as $q){
				$i++;
				if($q==="@rollback"){
					$db->rollBack();$cnt--;
				}else{
					$statement = $db->prepare($q);
					$data = isset($datas[$i-1])?$datas[$i-1]:[];
					if ($statement->execute($data) == false) {
						continue;
					}
					if(str_starts(strtolower($q), "select")){
						$res = $statement->fetchAll(PDO::FETCH_ASSOC);					}
					$cnt++;
				}
			}
		}	
	}catch(Exception $e){
		error_log("DB Transaction ERR:",$e->getMessage());
				$db->rollBack();
			}
	if($cnt>0)
		$db->commit();
	$db->setAttribute( PDO::ATTR_AUTOCOMMIT, 1 );
	$db->setAttribute(PDO::ATTR_ERRMODE, $mod);
	return $res;
}
function bson_enc($arr){
	$str = json_encode($arr);
	$str = str_replace("\\", "", $str);
	return str2hex($str);
}
function bson_dec($bson){
	if(isset($bson)){
		$json = hex2str($bson);
		return json_decode($json,true);
	}
	return false;
}
function db_v($v, $typeDef="", $bsonText=false){
	if(!isset($v))
		return "NULL";
	if(is_bool($v))
		return $v ? 1 : 0;
	if (is_array($v)){
		return $bsonText&&(isset($typeDef)&&preg_match("/text/i", $typeDef))? "'".bson_enc($v)."'"
				: "'".mysql_escape_string(json_encode($v))."'";
	}
	if(is_string($v)){
		if(preg_match("/bigint/i", $typeDef) && str_has($v, "-"))
			return strtotime($v);
		if(preg_match("/(int|byte)/i", $typeDef))
			return (int)$v;
		return "'".mysql_escape_string($v)."'";
	} 
	return $v;
}

class Model extends Observable implements Observer{
	
	private $db;
	private $pk;			private $table;			public 	$schema;		private $restricts;		private $indexes;		
	private function __construct(){}
	
	
	
	public static function factory($schemaName){
		try{
			if(!isset($schemaName))
				throw Exception("Model must have a schema name");
			$model = new Model();
	        $db = null;
	        if(str_has($schemaName, '.')){
	        	list($db, $schemaName) = explode('.', $schemaName);
	        	$model->db = $db;
	        } 
	        $model->addObserver($model);
			if(isset($schemaName) && $db==null)
				$model->loadSchema($schemaName);
			return $model;
		}catch(Exception $e){
			throw new Exception($schemaName.'Model not found');
		}
	}
	
	
	public function loadSchema($schemaname){
		try{
			$schemaDef = db_schema($schemaname);
			$general = $schemaDef["general"];
			error_log(json_encode($general));
			if(isset($general["db"])) $this->db = $general["db"];
			$this->pk	 	= $general["pk"];
			$this->table	= $general["name"];
			if(isset($general["index"])) $this->indexes = $general["index"];
			$this->schema 	= $schemaDef["schema"];
			$this->isTemp = isset($schemaDef["temp"]);
		}catch(Exception $e){
			throw new Exception($schemaName.' schema file not found');
		}
	}
	
	
	public function import($datas, $sql_dump_path=null){
		return db_import($this->table, $datas, $sql_dump_path);
	}
	
	public function find($opts=array(),$withCount=false){
		return db_find($this->table,$opts,$withCount);
	}
	
	
	
	public function fetch($id, $useCache=false, $cols="*",$pkName=null){
		try {
			if($pkName==null)	
				$pkName = $this->pk;
			if(!isset($this->table))
				return null;
			if($cols==null) $cols ="*";
			$sql = "SELECT ".$cols." FROM ".$this->table." WHERE `".$pkName."`=:$pkName LIMIT 1";
			error_log($sql);
			$res = db_query($sql,[$pkName=>$id],$useCache);
		    $data = $res[0];
		    $this->data = $data;
		    return $data;
		} catch (PDOException $e) {
			error_log($sql);
		    return null;
		}		
	}
	public function find1st($opts=array()){
		return db_find1st($this->table,$opts);
	}
	
	public function exists($pk){
		return db_exists($this-table,$pk);
	}
	
	public function del($optsOrPk){
		if(!isset($this->table) || !isset($this->pk) || !isset($optsOrPk))
			return false;
		if(is_array($optsOrPk)){
			$ops = db_make_query($this->table, $optsOrPk, ["order","fields"]);
			$sql = "DELETE FROM ".$this->table." ".$ops[1];
			return db_query($sql, $ops[2]);
		}else{
			$sql = "DELETE FROM ".$this->table." WHERE `".$this->pk."`=:".$this->pk;
			return db_query($sql, [$this->pk=>$optsOrPk]);
		}
	}
	
	
	public function save($data=null, $returnId=false){
		if (isset($data)){
			foreach ($data as $k => $v){
				$this->set($k, $v);
			}
		}
		$id = isset($this->data[$this->pk]) ? $this->data[$this->pk] : null;
		$isUpdate = isset($id);
		$d = $isUpdate?$this->changes:$this->data;
		if($isUpdate)$d[$this->pk] = $id;
		return db_save($this->table, $d, $returnId);
	}
	
	public function __set($key, $value){
		$this->$key = $value;
	}
	
	public function __get($key){
        return $this->$key;
	}
	
	
	public function update(Observable $obj, $args){
		if(isset($this->schema[$args["key"]])){
			$v = $args["val"];
			if(is_string($v) && str_has($this->schema[$args['key']], "int"))
				$v = (int) $v;
			$this->changes[$args['key']] = $v;
		}
	}
	
	public function get($keyPath){
		return hash_get($this->data, $keyPath);
	}
	
	public function setDB($db){
		$this->db= $db;
	}
	
	
}

class Render {
	static $path;
	private static $output_path;
	private static $ext='.html';
	
	private $layout = '_layout';
	private $data = [];
	private $contents = [];
	
	private function __construct(){}
	
	static function factory($path){
		self::$path = $path.__SLASH__;
		self::$output_path = APP_DIR.__SLASH__.'tmp'.__SLASH__;
		$render = new Render();
		return $render;
	}
	
	function assign($key, $value){
		$this->data[$key] = $value;	
	}
	
	
	function render($file,$data=[],$layout=null){
		$template = isset($layout)? $layout : $this->layout;
		$s = cache_get('template-'.$template, function($f){
			return file_get_contents(Render::$path.str_replace('template-','',$f));
		},false);
		list($before, $after) = explode('__CONTENTS__', $s);
		echo $before;
				if(!empty($data))
		foreach ($data as $k=>$v)
			$this->data[$k] = $v;
		extract($this->data);
		if(is_string($file)){
			$r = $this->render_file($file,$data);
			if($r) include($r);
		}else if(is_array($file))
			foreach ($file as $f){
				$r = $this->render_file($f,$data);
				if($r) include($r);
				flush();
			}
				echo $after;
		unset($this->data);
		unset($data);
	}
	
	
	function render_file($file,$data=[]){
		$filepath = self::$path.$file;
		if(!file_exists($filepath)) return false;
		$outpath = self::$output_path.'template-'.str_replace(self::$ext,'.php',$file);
		if(!file_exists($outpath)
			||(Consts::$template_check && filemtime($filepath) > filemtime($outpath))){
			$code = $this->compile($filepath);
			if(isset($code) && $code!=""){
				file_put_contents($outpath,$code);
				unset($code);
			}
		}
		return $outpath;
	}
	function compile($file){
		$rows = preg_split('/(\{[^\{^\}]*\})/', file_get_contents($file), -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY ) ;
		$phpcode = '';
		$indent = 0;
		$ignore = false;
		while($code = array_shift($rows)){
			$matched = false;
			preg_match_all('/\{(?P<close>\/*)(?P<tag>(%|=|if|elseif|else|break|ignore|for|var|include){0,1})\s*(?P<val>.*)\}/', $code, $matches);
			
			if(empty($matches[0])){
				$phpcode .= $code;
			}else{
				list($close, $tag, $val) = [$matches['close'][0]=="/"?true:false, $matches['tag'][0], 
						preg_replace('/\.([a-zA-Z0-9_]+)/', "['$1']",(trim($matches['val'][0])))];
				
				if($tag=='' || $tag=='=')$tag='echo';
				if($tag=='%')$tag='text';
				if($close){
					if($tag=='if'||$tag=='for')$indent --;
					if($tag=='ignore'){
						$ignore = false;
					}else{
						$phpcode .= '<?php } ?>';
					}
				}else if($ignore){
					$phpcode .= $code;
				}else{
					switch($tag){
						case 'for':
							$parts = preg_split('/\s*,\s*/',$val,-1,PREG_SPLIT_NO_EMPTY );
							$len = count($parts);
							$indent ++;
							switch($len){
								case 1:$phpcode .= '<?php foreach('.$parts[0].' as $key=>$value) { ?>';break;
								case 2:$phpcode .= '<?php foreach('.$parts[0].' as $key=>'.$parts[1].') { ?>';break;
								default :$phpcode .= '<?php foreach('.$parts[0].' as '.$parts[1].'=>'.$parts[2].') { ?>';break;
							}
							break;
						case 'if':
							$indent ++;
							$phpcode .= '<?php if('.$val.'){ ?>';break;
						case 'elseif':
							$phpcode .= '<?php }else if('.$val.'){ ?>';break;
						case 'else':
							$phpcode .= '<?php }else{ ?>';break;
						case 'break':
							$phpcode .= '<?php break; ?>';break;
						case 'echo':
							$phpcode .= '<?= '.$val.' ?>';break;
						case 'text':
							$phpcode .= '<?= T("'.$val.'"); ?>';break;
						case 'var':
							$phpcode .= '<?php '.$val.'; ?>';break;
						case 'include':
							$val = preg_replace_all('/\'"/',"",$val);
							$phpcode .= '<?php include_template("'.$val.'"); ?>';break;
						case 'ignore':
							$ignore = true;
							break;
						default:break;
					}				}			}
		}
		return $phpcode;
	}
	
}

?>