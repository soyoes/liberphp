<?php
/**
 *	@file: Core.inc
 *	@author: Soyoes 2014/01/09
 *	@uses: Core functions of dispatching, rendering.
 *****************************************************************************/
require_once LIBER_DIR.'modules'.__SLASH__."Lang.inc";
require_once LIBER_DIR.'modules'.__SLASH__."DB.inc";
require_once LIBER_DIR.'modules'.__SLASH__."Caches.inc";
class Consts{
	/*------------ APP Configuration Begin ---------------------------------*/
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
	
	//set this to false when you deploy.
	static $template_check		= true;		
	
	//set this to true when you deploy.
	static $csrf_check			= false;
	static $csrf_lifetime		= 300;
	
	static $session_enable		= true;
	
	static $filters				= [];
	
	static $schema_reg			= "regAt";
	static $schema_upd			= "updAt";
	
	/*------------ APP Configuration End ----------------------------------*/
	
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

/**
 * @param string $key
 * @param unknown_type $value
 */
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
	//error_log($path."/".$template_file);
	if(file_exists($path."/".$template_file)){//FIXME check APC instead
		render_html($template_file);
	}else{
		//show error page?
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
			//TODO
			break;
		default://text
			header("Content-type: text/plain; charset=utf-8");
			$reason = $contentType=="text" ? $reason:$contentType;
			echo "$code ERROR: $msg. $reason";
			break;
	}
	exit;
}


/*
 function log($msg){error_log($msg);}
*/
function redirect($url,$method="GET",$data=array()) {
	$appName = str_has($_SERVER['REQUEST_URI'],APP_NAME."/")?
	APP_NAME."/":"";

	$redirectUrl = str_starts($url, "http:") ||str_starts($url, "https:")?
	$url: "http://".$_SERVER["HTTP_HOST"]."/".$appName . $url;
	/*	//for some reason this does not works well
	 header("HTTP/1.1 301 Moved Permanently");
	header("Location: ".$redirectUrl);
	*/
	if("GET"==$method){
		echo "<html><script type='text/javascript'>window.location='".$redirectUrl."';</script></html>";
	}else{
		//FIXME!!
		$render = new Smarty();
		$clientType = REQ::getClientTemplateType();
		$vdir = APP_DIR.__SLASH__."views".__SLASH__;
		$path = $vdir.'templates'.__SLASH__."pc".$adminSurfix;
		$render->setTemplateDir($path);
		$render->setCompileDir($vdir.'templates_c');
		$render->setCacheDir($vdir.'cache');
		$render->setConfigDir($vdir.'configs');

		if(!str_starts($url,'http') && !Strings::startsWith($url,'https')){
			$url = self::getURL($url);//FIXME
		}
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

/**
 * get text by key
 * @key = filename.key
 * */
function T($key,$replace=array(),$lang="jp"){
	$filename = null;
	if(str_has($key, ".")){
		list($filename,$key) = explode(".", $key, 2);
	}else{
		$filename = strtolower(REQ::getController());
	}
	global $TEXTS;//FIXME : to APC Cache and push to REQ
	if(!isset($TEXTS) || !isset($TEXTS[$filename])){
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

/**
 * used in _page.html
 * @total = total records
 * @page = current page number
 *
 * */
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
		//shift right
		if($left<1)
			return paginate($page,$total,$perPage,$center-$left+1);
		//shift left
		if($right>$max)
			return paginate($page,$total,$perPage,$center-($right-$max));

		//draw center parts
		for($i=$left;$i<=$right;$i++){
			$pages[]=array("label"=>$i,"page"=>$i);;
		}

		//draw right
		switch ($max-$right){
			case 0:	//draw nothing;
				break;
			case 1:	//draw $max;
				$pages[]=array("label"=>$max,"page"=>$max);
				break;
			default:
				$pages[]=array("label"=>"...","page"=>$right+1);
				$pages[]=array("label"=>$max,"page"=>$max);
				break;
		}

		//draw left
		switch ($left-1){
			case 0://draw nothing;
				break;
			case 1://draw 1st page
				array_unshift($pages, array("label"=>1,"page"=>1));
				break;
			default:
				array_unshift($pages, array("label"=>"...","page"=>2));
				array_unshift($pages, array("label"=>1,"page"=>1));
				break;
		}
	}
	return $pages;
}


/**
 * Encapsulate request variables for security reason
 *
 */
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
	
	//TODO remove this
	private static $response_body = null;
	
	private function __construct(){}
	
	/**
	 * Get conf from conf.ini with keypath
	 * 
	 */
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
	
	
	/**
	 * Core function, all requests URLs are dispatched here.
	 */
	public static function dispatch(){
		if(self::$dispatched===true)return;
		self::parse_uri();
		try{
			$filters = $_REQUEST["filters"];
			$size = count($filters);
			$filterCls = [];
			if(Consts::$session_enable)
				Session::start();
			//check CSRF
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
				if ($token == 0){//Exec controller->actions
					if(isset(self::$rest_schema)){
						self::rest_process();
					}else{
						self::process();
					}
				}else if($size>0){//Exec filters
					$nextIdx = $token < 0 ? $size + $token : $size - $token ;
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
			//show
			if(!isset(self::$response_body)){
				render_html();
			}
			//echo $_REQUEST["RESPONSE_BODY"];
		}catch(Exception $e){
			error_log("exec exception");
			print $e->getMessage();
		}
		//echo $response;
		exit;
	}
	
	
	private static function parse_uri(){
		$host =$_SERVER["SERVER_NAME"];
		$uri = htmlEntities($_SERVER["REQUEST_URI"], ENT_QUOTES|ENT_HTML401);
		$uri = preg_replace(['/\sHTTP.*/','/(\/)+/','/\/$/','/^[a-zA-Z0-9]/'], ['',"/","","/$1"], $uri);
		//error_log($_SERVER["REQUEST_URI"]." host=$host url=$uri");
		$parts = parse_url("http://".$host.$uri);
		$uri = $parts["path"];
		$fmts = ['json','bson','text','html','csv'];
		//$params = ["@format"=>in_array($_REQUEST["@format"], $fmts)?$_REQUEST["@format"]:"html"];
		$params = [];
		
		if(isset($parts["query"]))
			parse_str($parts["query"],$params);
		list($uri, $ext) = explode(".", $uri);
		$specifiedFmt = in_array($ext,$fmts);
		if($ext==1||$ext==""||$specifiedFmt){//action
			preg_match_all('/\/(?P<digit>\d+)\/*/', $uri, $matches);
			if(!empty($matches["digit"])){
				$params["@id"] = (int)$matches["digit"][0];
				$uri = preg_replace("/\/\d+\/*/", "", $uri);
			}
			self::$uri = $uri;
			self::$params = $params;
			if($specifiedFmt) self::$format = $ext;
			self::parse_user_agent();
			self::parse_rest_uri();
		}else{//static
			//TODO redirect ?
			self::$uri = "/webroot".$uri.".$ext";
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
			self::$params[$k] = htmlEntities($v); //XSS
		}
		unset(self::$params["@format"]);
		unset(self::$params["@token"]);
	
		$resources = self::load_resources();
	
		list($namespace, $controller, $action) =
		["",Consts::$default_controller,Consts::$default_action];
		$len = count($uparts);
		/*
		 error_log("uri=$uri,ns=$namespace;ctrl=$controller;act=$action");
		*/
		//echo(json_encode($resources))."\n";
		if(empty($uparts)){$uparts=[$controller,$action];}
		if(count($uparts)==1)$uparts[]=$action;
	
		if($uri==""){
			self::$uri = $controller;
		}else if(in_array($uri,$resources["namespaces"])){//default controller
			$namespace = $uri;
		}else if(in_array($uri,$resources["controllers"])){//controller exist with default action
			$namespace = join("/",array_slice($uparts, 0 , $len-1));
			$controller = $uparts[$len-1];
			//error_log("CTRL : ns=$namespace;ctrl=$controller;act=$action");
		}else if(in_array(join("/",array_slice($uparts, 0 , $len-1)),$resources["controllers"])){//controller exist with action
			$namespace = join("/",array_slice($uparts, 0 , $len-2));
			$controller = $uparts[$len-2];
			$action = $uparts[$len-1];
			//error_log("ctrl with act",$namespace,"-",$controller,"-",$action);
		}else{//controller not exist
			if(str_starts($uri,"@")){//check RESTful request with no controller.
				$uri = substr($uparts[0],1);
				if(in_array($uri,$resources["schemas"])){
					$controller = "@REST";
					self::$rest_schema = $uri;
					$action = $method;
				}
			}else if(in_array(self::$client_type."/".join("_",$uparts),$resources["views"])){//has view template file
				//error_log($_REQUEST["client_type"]."/".join("_",$uparts).".html");
				render_html(join("_",$uparts).".html");
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
		//$br=get_browser($ua); //never use this, it will kill performance !!!
		$type = "pc";
		if(preg_match('/(curl|wget|ApacheBench)\//i',$ua))
			$type = "cmd";
		else if(preg_match('/(iPad|MSIE.*Touch|Android)/',$ua)) //FIX Android problems later.
			$type = "pad";
		else if(preg_match('/(iPhone|iPod|(Android.*Mobile)|BlackBerry|IEMobile)/i',$ua))
			$type = "sm";
		/*
		 * check search engine bots.
		* Yeti : naver.com /Korea:6
		* ia_archiver : alexa
		* ichiro : goo.ne.jp
		* Y!J-BRJ/YATS : listing.yahoo.co.jp
		* Jeeves\/Teoma|BlogCrawler : ask.com / ask.jp
		* */
		//text search bot
		if(preg_match('/Googlebot|bingbot|msnbot|Yahoo|Y\!J|Yeti|Baiduspider|BaiduMobaider|ichiro|hotpage\.fr|Feedfetcher|ia_archiver|Tumblr|Jeeves\/Teoma|BlogCrawler/i',$ua))
			$type = "bot";
		else if(preg_match('/Googlebot-Image|msnbot-media/i',$ua))
			$type = "ibot";
		self::$client_type = $type;
		
		$vtypes = self::load_resources()["view_types"];
		self::$client_template_type = in_array($type,$vtypes)? $type:"pc";//TODO:empty dir, with no file
	}
	
	/**
	 * Preload all schemas, controllers with APC
	 */
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
	
	/**
	 * Controller function, Process normal requests.
	 * Obey the rules of /...PATH/CONTROLLER_NAME/ACTION_NAME?parameters
	 */
	private static function process($useSession=true){
		try {
			//FIXME add namespace
			$ctrldir = APP_DIR.__SLASH__."controllers".__SLASH__;
			$controller_dir = isset(self::$namespace) && self::$namespace!="" ? $ctrldir.self::$namespace."/":$ctrldir;
			$file_path = $controller_dir.self::$controller.".inc";
			require_once $file_path;
			//auth check
			if(isset($auth_actions)){
				$auth_actions = str_replace(" ","",$auth_actions);
				if($auth_actions=="*" || str_has(",".$auth_actions.",", ",$actionName,")){
					$res = Auth::check();
					if($res===false){
						return error("401","json");
					}
				}
			}
			//process
			$action = self::$action;
			$exec = function($action){
				$has_wrapper =  !isset($exclude_wrappers) || !in_array($action, $exclude_wrappers);
				if (function_exists("before_wrapper") && $has_wrapper)
					before_wrapper(self::$params);
				$action(self::$params);
				if (function_exists("after_wrapper")  && $has_wrapper){
					if(self::$format!="html"){//use only on json or text, html should use in smarty.
						after_wrapper(self::$params);
					}else{
						$_REQUEST["after_wrapper"] = true;
					}
				}
			};
			if(function_exists($action)){//normal request
				$exec($action);
			}else if(str_starts($action,"test_") && function_exists(str_replace("test_","",$action))){//unit test
				$action = str_replace("test_","",$action);
				$file = APP_DIR.__SLASH__."test".__SLASH__.self::$controller.".json";
				if (file_exists($file)){
					$content = file_get_contents($file);
					$content = str_replace("\n","",str_replace("\t", "", (string)$content));//Notice: ¥n ¥t will not be trimed anyway :(
					$cases = json_decode($content, true);
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
			}else{//no action
				return render_default_template();
			}
		} catch(Exception $e) {
			echo $e->getMessage();
			throw new Exception($controllerName.',Controller not found');
		}
	}
	
	static function clean(){
		unset(self::$params);
	}
	
	/**
	 * Controller function, Process RESTful requests (without controller definations).
	 * Obey the rules of /@SCHEMA_NAME/{ID}?parameters
	 */
	private static function rest_process(){
		$schema = self::$rest_schema;
		//check filter
		require_once APP_DIR.__SLASH__."restful".__SLASH__.$schema.".inc";
		//if($_REQUEST["test_method"])$_REQUEST["action"] =$_REQUEST["test_method"];
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


?>