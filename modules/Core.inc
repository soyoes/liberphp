<?php
/**
 *	@file: Core.inc
 *	@author: Soyoes 2014/01/09
 *	@uses: Core functions of dispatching, rendering.
 *****************************************************************************/

$db = null;
$render = null;
$render_path = null;
$render_contents = [];
$render_layout = "_layout.html";

include_once "DB.inc";
include_once "Caches.inc";

/**
 * Core function, all requests URLs are dispatched here. 
 */
function dispatch(){
	parse_uri();
	try{
		
		$req = $_REQUEST;
		$filters = $req["filters"];
		$size = count($filters);
		$filterCls = [];
		for ($token=$size*(-1); $token<=$size; $token++){
			if(true==$req["INTERRUPTED"])
				break;
			if ($token == 0){//Exec controller->actions
				if(isset($_REQUEST["@schema"])){
					rest_process();					
				}else{
					process();
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
		if(isset($_REQUEST["redirect"])){
			redirect($_REQUEST["redirect"]);
		}
		//show
		if(!isset($_REQUEST["RESPONSE_BODY"])){
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
	

function parse_uri(){
	//if(!STAND_ALONE)$_SERVER["REQUEST_URI"]=$_REQUEST["url"];
	$host =$_SERVER["SERVER_NAME"];
	$uri =$_SERVER["REQUEST_URI"];
	$uri = preg_replace(['/\sHTTP.*/','/(\/)+/','/\/$/','/^[a-zA-Z0-9]/'], ['',"/","","/$1"], $uri);
	//error_log($_SERVER["REQUEST_URI"]." host=$host url=$uri");
	$parts = parse_url("http://".$host.$uri);
	$uri = $parts["path"];
	$fmts = ['json','bson','text','html','csv'];
	$params = ["@format"=>in_array($_REQUEST["format"], $fmts)?$_REQUEST["@format"]:"html"];
	if(isset($parts["query"]))
		parse_str($parts["query"],$params);
	list($uri, $ext) = explode(".", $uri);
	$specifiedFmt = in_array($ext,$fmts);
	if($ext==1||$ext==""||$specifiedFmt){//action
		preg_match_all('/\/(?P<digit>\d+)\/*/', $uri, $matches);
		if(!Arrays::isEmpty($matches["digit"])){
			$params["@id"] = (int)$matches["digit"][0];
			$uri = preg_replace("/\/\d+\/*/", "", $uri);
		}
		$_REQUEST["uri"] = $uri;
		$_REQUEST["params"] = $params;
		if($specifiedFmt) $_REQUEST["@format"] = $ext;
		parse_user_agent();
		parse_rest_uri();
	}else{//static
		//TODO redirect ?
		$_REQUEST["uri"] = "/webroot".$uri.".$ext";
	}
	
}

function parse_rest_uri(){
	$uri = preg_replace("/^\//","",$_REQUEST["uri"]);
	$uparts =Arrays::remove(explode("/",$uri), "");
	
	$method = strtolower($_SERVER["REQUEST_METHOD"]);
	
	$params = $_REQUEST["params"];
	if ($method == "put" || $method == "delete") {
		parse_str(file_get_contents('php://input'), $params);
	}
	$target =($method=="post"||$method=="put")?$_POST: $_GET;
	foreach($target as $k=>$v)
		$params[$k] = $v;
	unset($params["__URL__"]);
	
	$_REQUEST["params"] = $params;
	$resources = load_resources();
	
	list($namespace, $controller, $action) =
		["",defined(APP_DEFAULT_CONTROLLER)?APP_DEFAULT_CONTROLLER:"top",defined(APP_DEFAULT_ACTION)?APP_DEFAULT_ACTION:$method];
	$len = count($uparts);
	/*
	error_log("uri=$uri,ns=$namespace;ctrl=$controller;act=$action");
	error_log(json_encode($resources));
	*/
	if(empty($uparts)){$uparts=[$controller,$action];}
	if(count($uparts)==1)$uparts[]=$action;
	
	if($uri==""){
		$uri=$controller;
		$_REQUEST["uri"] = $uri;
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
		if($len==1 && Strings::startsWith($uri,"@")){//check RESTful request with no controller.
			$uri = substr($uri,1);
			if(in_array($uri,$resources["schemas"])){
				$controller = "@REST";
				$_REQUEST["@schema"] = $uri;  
				$action = $method;
			}
		}else if(in_array($_REQUEST["client_type"]."/".join("_",$uparts),$resources["views"])){//has view template file
			//error_log($_REQUEST["client_type"]."/".join("_",$uparts).".html");
			render_html(join("_",$uparts).".html");
			exit;
		}else error(400);
	}
	$_REQUEST["namespace"] = $namespace;
	$_REQUEST["controller"] = $controller;
	$_REQUEST["action"] = $action;
	//error_log("ns=$namespace, ctrl=$controller, act=$action");
}

function parse_user_agent(){
	$ua = isset($_SERVER['HTTP_USER_AGENT'])?$_SERVER['HTTP_USER_AGENT']: $_REQUEST["userAgent"];
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
		$type = "imgbot";
	$_REQUEST["client_type"] = $type;
	$_REQUEST["view_template_type"] = Strings::contains(CLINET_TYPES, "|$type|") ? $type:"pc";
	//error_log("client_type = ".$_REQUEST["client_type"]);
}

/**
 * Preload all schemas, controllers with APC 
 */
function load_resources(){
	$resources = cache_get("APP_RESOURCES", function($key){
		exec("find ".CTRL_DIR,$res);
		$namespaces = array_unique(array_map(function($e){
			return strtolower(preg_replace(["/^".str_replace("/","\/",CTRL_DIR)."/",'/\/(.*)\.inc$/',"/^\//"],["","",""], $e));
		},array_slice($res,0)));
		$controllers = array_unique(array_map(function($e){
			return strtolower(preg_replace(["/^".str_replace("/","\/",CTRL_DIR)."/",'/\.inc$/',"/^\//"],["","",""], $e));
		},array_slice($res,0)));
		exec("find ".SCHEMA_DIR,$res2);
		$schemas = array_unique(array_map(function($e){
			return strtolower(preg_replace(["/^".str_replace("/","\/",SCHEMA_DIR)."/",'/\.ini$/',"/^\//"],["","",""], $e));
		},$res2));
		exec("find ".VIEW_DIR,$res3);
		$views = array_unique(array_map(function($e){
			return strtolower(preg_replace(["/^".str_replace("/","\/",VIEW_DIR)."/",'/\.html$/',"/^\//"],["","",""], $e));
		},$res3));
		return ["namespaces" => Arrays::remove($namespaces, ""), "controllers" => Arrays::remove($controllers, ""),
		 		"schemas"=>Arrays::remove($schemas, ""),"views"=>Arrays::remove($views, ["","pc","sm","bot","ibot","pad"])];
	},false);
	return $resources;
}

/**
 * Controller function, Process normal requests.
 * Obey the rules of /...PATH/CONTROLLER_NAME/ACTION_NAME?parameters
 */
function process($useSession=true){
	try {
		if($useSession)
			Session::start();
		//FIXME add namespace
		$controller_dir = isset($_REQUEST["namespace"]) && $_REQUEST["namespace"]!="" ? CTRL_DIR.$_REQUEST["namespace"]."/":CTRL_DIR;
		$file_path = $controller_dir.$_REQUEST["controller"].".inc";
		include_once $file_path;
		//auth check
		if(isset($auth_actions)){
			$auth_actions = str_replace(" ","",$auth_actions);
			if($auth_actions=="*" || Strings::contains(",".$auth_actions.",", ",$actionName,")){
				$res = Auth::check();
				if($res===false){
					return error("401","json");
				}
			}
		}
		//process
		$action = $_REQUEST["action"];
		$exec = function($action){
			$has_wrapper =  !isset($exclude_wrappers) || !in_array($action, $exclude_wrappers);
			if (function_exists("before_wrapper") && $has_wrapper)
				before_wrapper($_REQUEST["params"]);
			$action($_REQUEST["params"]);
			if (function_exists("after_wrapper")  && $has_wrapper){
				if($_REQUEST["RESPONSE_TYPE"]!="text/html"){//use only on json or text, html should use in smarty.
					after_wrapper($_REQUEST["params"]);
				}else{
					$_REQUEST["after_wrapper"] = true;
				}
			}
		};
		if(function_exists($action)){//normal request
			$exec($action);
		}else if(Strings::startsWith($action,"test_") && function_exists(str_replace("test_","",$action))){//unit test
			$action = str_replace("test_","",$action);
			$file = TEST_DIR.$_REQUEST["controller"].".json";
			if (file_exists($file)){
				$content = file_get_contents($file);
				$content = str_replace("\n","",str_replace("\t", "", (string)$content));//Notice: ¥n ¥t will not be trimed anyway :(
				$cases = json_decode($content, true);
				if(isset($cases) && isset($cases[$action])){
					$cases = $cases[$action];
					foreach ($cases as $case){
						Tests::exec($_REQUEST["controller"], $action, $case);
					}
				}
			}else{
				echo "<H1>Error:</H1><br>Test case does not exist <br> Pls add case to /test/[CONTROLLER].json<br>And specify your case by your ACTION_NAME";
				error(400,"html");
			}
			interrupt();
		}else{//no action
			return render_default_template();
		}
	} catch(Exception $e) {
		echo $e->getMessage();
		throw new Exception($controllerName.',Controller not found');
	}
}
/**
 * Controller function, Process RESTful requests (without controller definations).
 * Obey the rules of /@SCHEMA_NAME/{ID}?parameters
 */
function rest_process(){
	$schema = $_REQUEST["@schema"];
	//check filter
	include_once REST_DIR.$schema.".inc";
	//if($_REQUEST["test_method"])$_REQUEST["action"] =$_REQUEST["test_method"]; 
	$func = "rest_".$_REQUEST["action"];
	$pk = db_schema($schema)["general"]["pk"];
	$params = $_REQUEST["params"];
	if(isset($params[$pk]) && !isset($params["@id"]))
		$params["@id"] = $params[$pk];
	$delegate = "rest_".$schema."_".$_REQUEST["action"];
	$available = (function_exists($delegate)) ? $delegate($schema, $params):true;
	if($available){
		return $func($schema, $params);
	}else{
		error(401, "RESTful ERROR : Sorry, You are not permited to do that.");
	}
}

function rest_get($schema, $params){
	$res = (isset($params["@id"]))?
		db_find1st($schema, $params):
		db_find($schema, $params);
	render_json($res);
}

function rest_post($schema, $params){
	if(isset($params["@id"])){
		error(400,"RESTful ERROR : Sorry, You can't use RESTful POST with @id, try PUT for update or using normal controllers");
	}else{
		return render_json(db_save($schema, $params, true));
	}
}
function rest_put($schema, $params){
	if(isset($params["@id"])){
		return render_json(db_save($schema, $params));
	}else{
		error(400,"RESTful ERROR : You must specify a @id to use RESTful PUT");
	}
}
function rest_delete($schema, $params){
	if(isset($params["@id"])){
		return render_json(db_delete($schema, $params));
	}else{
		error(400,"RESTful ERROR : You must specify a @id to use RESTful DELETE");
	}
}



/**
 * @param string $key
 * @param unknown_type $value
 */
function assign($key, $value){
	if(!isset($render) || $render==null)
		init_render();
	$render->assign($key, $value);
}

function init_render($path=null){
	require_once SMARTY_DIR.'Smarty.class.php';
	global $render,$render_path;
	$render = new Smarty();
	$clientType = $_REQUEST['view_template_type'];
	if ($path==null){
		$path = VIEW_DIR.__SLASH__.$clientType;
		$path = $_REQUEST["namespace"]==""? $path:$path."/".$_REQUEST["namespace"];
	}
	$render_path = $path;
	$render->setTemplateDir($path);
	$render->setCompileDir(TMP_DIR.'templates_c');
	$render->setCacheDir(TMP_DIR.'cache');
	$render->setConfigDir(TMP_DIR.'configs');
	$render->assign('CLIENT_TYPE',$clientType);
	$render->assign('controller',$_REQUEST["controller"]);
	$render->assign('action',$_REQUEST["action"]);
}

function render($arg1=false, $arg2=false){
	switch($_REQUEST["@format"]){
		case "json":return render_json($arg1);
		case "text":return render_text($arg1);
		default:return render_html($arg1,$arg2);			
	}
}

function render_template($templateName=NULL, $datas=array()){
	global $render,$render_path,$render_contents;
	if(!isset($render))
		init_render();
	if (!isset($templateName)){
		$templateName = $_REQUEST["action"];
	}
	if(!empty($datas)){
		foreach ($datas as $k=>$v){
			$render->assign($k, $v);
		}
	}
	//$this->assign("controller", $this->controller);
	if(!Strings::endsWith($templateName, ".html") && !Strings::endsWith($templateName, ".html"))
		$templateName = $_REQUEST["controller"].'_'.$templateName.'.html';
	error_log($render_path."/".$templateName);
	if(file_exists($render_path."/".$templateName))
		$render_contents[] = $render->fetch($templateName);
}

function render_html($templateName=NULL, $datas=array()){
	global $render,$render_layout,$render_contents;
	if(!isset($render) || $render==null)
		init_render();
	render_template($templateName, $datas);
	$appName = Strings::contains($_REQUEST['uri'],"/".APP_NAME."/")?"/".APP_NAME:"";
	$render->assign('TITLE',APP_NAME);
	$render->assign('CONTENTS', $render_contents);

	$uid = $_REQUEST["uid"];
	/*
	 if($uid!=null && ($uid>0 || isset($_REQUEST["trial"]))){
	$user = $_REQUEST["me"];
	assign("me", $user->data);
	}*/
	//render template file
	$html = $render->fetch($render_layout);
	$_REQUEST["RESPONSE_TYPE"]="text/html";
	$_REQUEST["RESPONSE_BODY"]=$html;
	if(isset($_REQUEST["after_wrapper"]))
		after_wrapper($_REQUEST["params"]);
	echo $html;
}

function render_json($data,$code="200"){
	$_REQUEST["RESPONSE_TYPE"]="application/json";
	$_REQUEST["RESPONSE_BODY"]=json_encode($data);
	echo $_REQUEST["RESPONSE_BODY"];
}

function render_text($text,$code="200"){
	$_REQUEST["RESPONSE_TYPE"]="text/plain";
	$_REQUEST["RESPONSE_BODY"]=$text;
	echo $text;
}

function render_default_template(){
	$path = VIEW_DIR.$_REQUEST['view_template_type'];
	if($_REQUEST["namespace"]!=""){
		$path .= "/".str_replace(".","/",$_REQUEST["namespace"]);
	}
	$template_file = $_REQUEST["controller"].'_'.$_REQUEST["action"].'.html';
	error_log($template_file);
	if(file_exists($path."/".$template_file)){
		render_html($template_file);
	}else{
		//show error page?
		error(400,"json","action does not exist ");
	}
	exit;
}

function error_code($code){
	$codes = array(
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
			"500"=>"Internal Server Error",
			"501"=>"Not Implemented");
	return $codes["".$code];
}

function error($code, $contentType="text", $reason=""){
	$msg = error_code($code);
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
	$appName = Strings::contains($_SERVER['REQUEST_URI'],APP_NAME."/")?
	APP_NAME."/":"";

	$redirectUrl = Strings::startsWith($url, "http:") ||Strings::startsWith($url, "https:")?
	$url: "http://".$_SERVER["HTTP_HOST"]."/".$appName . $url;
	/*	//for some reason this does not works well
	 header("HTTP/1.1 301 Moved Permanently");
	header("Location: ".$redirectUrl);
	*/
	if("GET"==$method){
		echo "<html><script type='text/javascript'>window.location='".$redirectUrl."';</script></html>";
	}else{
		$render = new Smarty();
		$clientType = $_REQUEST['view_template_type'];
		$path = VIEW_DIR.'templates'.__SLASH__."pc".$adminSurfix;
		$render->setTemplateDir($path);
		$render->setCompileDir(VIEW_DIR.'templates_c');
		$render->setCacheDir(VIEW_DIR.'cache');
		$render->setConfigDir(VIEW_DIR.'configs');

		if(!Strings::startsWith($url,'http') && !Strings::startsWith($url,'https')){
			$url = self::getURL($url);
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
	if(Strings::contains($key, ".")){
		list($filename,$key) = explode(".", $key, 2);
	}else{
		$filename = Strings::contains($_REQUEST["controller"], "Controller")?
		strtolower(str_replace($_REQUEST["controller"],"Controller", "")):strtolower($_REQUEST["controller"]);
	}
	global $TEXTS;//FIXME : to APC Cache
	if(!isset($TEXTS) || !isset($TEXTS[$filename])){
		$file = TEXT_DIR.$lang."/".$filename.".json";
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
	if(!Arrays::isEmpty($TEXTS[$filename])){
		$text = $TEXTS[$filename][$key];
	}
	if(!empty($text) && !empty($replace)){
		if(!Arrays::isHash($replace)){
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
 * Interrupt the current process & run the next test case.
 * For unit test only !!!
 * */
function interrupt(){
	$_REQUEST["INTERRUPTED"] = true;
}


?>