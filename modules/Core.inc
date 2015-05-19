<?php
/**
 *	@file: Core.inc
 *	@author: Soyoes 2014/01/09
 *	@uses: Core functions of dispatching, rendering.
 *****************************************************************************/

/**
 * consts 
 */
class Consts extends Conf{
	
	static $db_regexp_op = ['mysql'=>'REGEXP','postgres'=>'~'];
	
	static $db_query_filters;
	
	static $arr_query_filters;
	
	static $query_filter_names = [
		'eq' 	=> '=',
		'ne' 	=> '!',
		'lt' 	=> '<',
		'gt'	=> '>',
		'le' 	=> '<=',
		'ge'	=> '>=',
		'in'	=> '[]',
		'nin' 	=> '![]',
		'bt' 	=> '()',
		'nb' 	=> '!()',
		'l' 	=> '?',
		'nl' 	=> '!?',
		'm' 	=> '~',
		'nm' 	=> '!~',
		'mi' 	=> '~~',
		'nmi' 	=> '!~~'
	];
	
	static $error_codes = [
		'200'=>'OK',
		'201'=>'Created',
		'202'=>'Accepted',
		'204'=>'No Content',
		'301'=>'Moved Permanently',
		'302'=>'Found',
		'400'=>'Bad Request',
		'401'=>'Unauthorized',
		'403'=>'Forbidden',
		'404'=>'Not Found',
		'413'=>'Request Entity Too Large',
		'414'=>'Request-URI Too Large',
		'415'=>'Unsupported Media Type',
		'419'=>'Authentication Timeout',
		'500'=>'Internal Server Error',
		'501'=>'Not Implemented'];
	
	static function init(){
		self::$db_engine = strtolower(self::$db_engine);
		if(empty(self::$default_action)) self::$default_action = strtolower($_SERVER['REQUEST_METHOD']);
	}
}

Consts::init();

/**
 * Assign values to render.
 * @param string $key
 * @param unknown_type $value
 */
function assign($key, $value){
	$render = REQ::getInstance()->getRender();
	$render->assign($key, $value);
}

/**
 * Set render layout
 * @param string $file
 * @example
 *   render_layout("_mylayout.html");
 */
function render_layout($file){
	$req = REQ::getInstance();
	$req->setRenderLayout($file);
}

function render($arg1=false, $arg2=false){
	switch(REQ::getInstance()->getFormat()){
		case 'json':return render_json($arg1);
		case 'text':return render_text($arg1);
		default:return render_html($arg1,$arg2);			
	}
}

function render_html($templateName=null, $datas=array()){
	$req = REQ::getInstance();
	list($render,$render_layout) = [$req->getRender(), $req->getRenderLayout()];
	$appName = str_has($req->getURI(),'/'.APP_NAME.'/')?'/'.APP_NAME:'';
	$render->assign('TITLE',APP_NAME);
	header('Content-type: text/html; charset=UTF-8');
	if($templateName&&empty($datas)&&is_array($templateName)){
		$datas = $templateName;
		$templateName=null;
	}
	if (!$templateName)
		$templateName = $req->getController().'_'.$req->getAction().'.html';
	if(!str_ends($templateName, '.html'))
		$templateName .= '.html';
	$render->render($templateName,$datas,$render_layout);
	$req->setResponseBody('true');
	if(isset($_REQUEST['after_wrapper']))
		after_wrapper($req->getParams());
}

function render_json($data){
	$body = json_encode($data);
	header('Content-type: application/json; charset=UTF-8');
	REQ::getInstance()->setResponseBody(json_encode($data));
	REQ::write($body,'json');
}

function render_text($text){
	header('Content-type: text/plain; charset=UTF-8');
	REQ::getInstance()->setResponseBody($text);
	REQ::write($text,'text');
}

function render_default_template(){
	$path = APP_DIR.__SLASH__.'views'.__SLASH__.REQ::getTemplateType();
	$req = REQ::getInstance();
	$ns = $req->getNamespace();
	if($ns!=''){
		$path .= '/'.str_replace('.','/',$ns);
	}
	$template_file = $req->getController().'_'.$req->getAction().'.html';
	//error_log($path."/".$template_file);
	if(file_exists($path.'/'.$template_file)){//FIXME check APC instead
		render_html($template_file);
	}else{
		//show error page?
		error(400,'json','action does not exist.');
	}
	REQ::quit();
}

/**
 * generate key string with specified length, and character set.
 * @param  $len: length of the key
 * @param  $chars: charactor set string
 * @return a key string
 * @example
 *  keygen(64, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.;,-$%()!@")
 */
function keygen($len,$chars=false){
	if(!isset($len))$len=16;
	if(!$chars) $chars = "abcdefghijklmnopqrstuvwxyz0123456789_.;,-$%()!@";
	$key="";$clen=strlen($chars);
	for($i=0;$i<$len;$i++){
		$key.=$chars[rand(0,$clen-1)];
	}
	return $key;
}

/**
 * send error code + message and quit
 * @param $code: HTTP error code. http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 * @param $contentType[OPTIONAL]: json | text[default] | html
 * @param $reason: messages to send back to the client.
 */
function error($code, $contentType, $reason=""){
	$msg = Consts::$error_codes["".$code];
	header("HTTP/1.1 $code $msg", FALSE);
	$req = REQ::getInstance();
	$src = $req->load_resources();
	$type = $req->getClientType();
	$hasHtml = in_array("$type/error_$code", $src["views"]);
	if(isset($contentType)&&!in_array($contentType, ["html","json","text"])){
		if(empty($reason)) {
			$reason=$contentType;
			$contentType=null;
		}
	}	
	if(!$contentType)
		$contentType=$hasHtml?"html":"text";
	
	switch($contentType){
		case "json":
			header("Content-type: application/json; charset=utf-8");
			echo '{"error":"'."$code $msg. $reason".'"}';
			break;
		case "html":
			header("Content-type: text/html; charset=utf-8");
			if($hasHtml)
				render_html("error_$code.html",['code'=>$code,'msg'=>$msg,'reason'=>$reason]);
			else
				echo "<HTML><BODY><h1>$code ERROR</h1></BODY></HTML>";
			break;
		default://text
			header("Content-type: text/plain; charset=utf-8");
			echo "$code ERROR: $msg. $reason";
			break;
	}
	REQ::quit();
}

/**
*   detecting HTTPS request
*/
function is_https() {
    if(!empty($_SERVER['HTTP_X_FORWARDED_PROTO']))
        return strtoupper($_SERVER['HTTP_X_FORWARDED_PROTO']) == "HTTPS";
    else
        return (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') || $_SERVER['SERVER_PORT'] == 443;
}

/**
 * redirect to url
 * @param $url: url to redirect
 * @param $clientRedirect: if let client browser to do the redirect job. (for some reason, header("Location :xxx") does not work well sometime.) 
 * 
 */
function redirect($url,$clientRedirect=false) {
	$appName = str_has($_SERVER['REQUEST_URI'],APP_NAME."/")?
	APP_NAME."/":"";

	$redirectUrl = str_starts($url, "http:") || str_starts($url, "https:") ?
	   $url : (is_https() ? "https" : "http") . "://".$_SERVER["HTTP_HOST"]."/".$appName . $url;
	
	// for some reason this sometimes does not works well !!
	if(!$clientRedirect){
		header("HTTP/1.1 301 Moved Permanently");
		header("Location: ".$redirectUrl);
	}else{
		header("Content-type: text/html; charset=utf-8");
		echo "<script type='text/javascript'>window.location='".$redirectUrl."';</script>";
	}
	REQ::quit();
}

/**
 * shortcut version of cURL
 * @param $url : url to access
 * @param $method : get|post|put|delete
 * @param $data : query parameters or data to POST/PUT/DELETE 
 * @param $options : PHP curl options, check http://www.php.net/manual/en/function.curl-setopt.php
 */
function call($url, $method, $data = [], $header = [], $options = []) {
    $method = strtoupper($method);
    $defaults = $method == "POST" || $method == "PUT" ? [
        CURLOPT_URL            => $url,
        CURLOPT_POST           => 1,
        CURLOPT_HEADER         => 0,
        CURLOPT_RETURNTRANSFER => 1,
        CURLOPT_POSTFIELDS     => http_build_query($data)
    ]:[
        CURLOPT_URL            => $url. (strpos($url, '?') === FALSE ? '?' : ''). http_build_query($data),
        CURLOPT_HEADER         => 0,
        CURLOPT_RETURNTRANSFER => 1
    ];
    if ($header) {
        $defaults[CURLOPT_HTTPHEADER] = $header;
    }

    $ch = curl_init();
    curl_setopt_array($ch, $options + $defaults);
    if( ! $result = curl_exec($ch)){
        trigger_error(curl_error($ch));
    }
    curl_close($ch);
    
    return $result;
}

/**
 * exec an asynced backend task after send msg to the client.
 * @param $msg : msg/html/json to send to the client.
 * @param $func : the task to be exec background.
 * @example:
 	async("1", function($a,$b){sleep($a);error_log($b);}, 3, "Sleeped 3 seconds");
 */
function async($msg,$func){
	ob_end_clean();
	header("Connection: close");
	ignore_user_abort(true); // just to be safe
	ob_start();

	echo $msg;

	$size = ob_get_length();
	header("Content-Length: $size");
	ob_end_flush(); 
	flush();

	$args = array_slice(func_get_args(), 2);
	call_user_func_array ($func, $args);
}

/**
 * language support . get text by key from /conf/text.csv
 * @param $key : id, the first column of text.csv
 * @param $args : other params to format the text
 * @example:
 *  Consts::$lang = "jp"; // set this in your liber.php	
 *  $txt = T("tasks.task_name"); 
 *  
 *  $txt = T("welcome_message", "YOUR NAME");  
 *  	//given that welcome_message="Welcome %s", then the result will be "Welcome YOUR NAME"
 * 	
 * 	// target file : /conf/text.csv
 * 	// id : tasks.task_name
 */
function T($key){
	$filename = null;
	$lang = isset($_REQUEST["@lang"])?$_REQUEST["@lang"]:
					(empty($_SESSION['lang']) ? $_SESSION['lang']:
						(isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])?
							substr($_SERVER['HTTP_ACCEPT_LANGUAGE'], 0, 2):
							Consts::$lang));
	$text_func = function($fn){
		$file = join(__SLASH__,[APP_DIR,"conf","text.csv"]);
		if (file_exists($file)){
			$lines = preg_split('/[\r\n]+/',file_get_contents($file));
			$idx = 0;
			$res = [];$langs = [];

			if (($handle = fopen($file, "r")) !== FALSE) {
				$max_len = 0; // unlimited
				$delimiter = ",";
				try {
					while (($cols = fgetcsv($handle, $max_len, $delimiter)) !== FALSE) {
						if($idx++==0){
							if($cols[0]!='id'){
								error_log("Language File Error: the first column of text.csv must have a name of 'id' ");
								return [];
							}
							$langs = $cols;
							array_shift($langs);
							continue;
						}else{
							$c = 1;
							$id = $cols[0];
							$res[$id] = [];
							foreach ($langs as $l) 
								$res[$id][$l] = $cols[$c]?$cols[$c++]:"";
						}
					}
				} catch(Exception $e) {
					error_log("Language File Error: " . $e->getMessage());
				}
			}
			fclose($handle);
			return $res;
		}return [];
	};
	$texts = Consts::$mode=="Developing"?$text_func("__TEXTS__"):cache_get("__TEXTS__", $text_func);
	$lang = isset($texts[$key][$lang]) ? $lang : (isset($texts[$key][Consts::$lang]) ? Consts::$lang : false);
	if($lang){
		$text = $texts[$key][$lang];
		if(str_has($text,"%")){
			$args = array_slice(func_get_args(), 1);
			$enc = mb_detect_encoding($text);
			return $lang=="en"? vsprintf($text, $args) : 
				mb_convert_encoding(vsprintf(mb_convert_encoding($text,"UTF-8",$enc), $args),$enc,"UTF-8");
		}else
			return $text;
	}else{
		error_log("__ERR_WORD_NOT_EXISTS_($key,$lang)__, please check your /conf/text.csv");
		return null;
	}
}

/**
 * 
 * @result : [
 * 	uri => "/mycontroller/myaction?a=1&b=2", //relative path
	namespace => "",			//folder name under /controllers
	controller => "mycontroller",
	action => "myaction",
	format => "html",			//html|csv|json|bson|text
	schema_name => null,		//restful request only, e.g. "tasks"
	static => "mycontroller_myaction.html" //static template only (no action or controller file)
	params => [
		a=>1,
		b=>2
	]
 * ]
 * */
function parse_uri($uri, $method, &$params=[], $ua=""){
	$host =$_SERVER["SERVER_NAME"];
	if(empty($uri)) $uri = $_SERVER["REQUEST_URI"];
	if(empty($method)) $method = $_SERVER["REQUEST_METHOD"];
	if(empty($ua)) $ua = $_SERVER['HTTP_USER_AGENT'];
	
	$uri = htmlEntities($uri, ENT_QUOTES|ENT_HTML401);
	$uri = preg_replace(['/\sHTTP.*/','/(\/)+/','/\/$/','/^[a-zA-Z0-9]/'], ['',"/","","/$1"], $uri);
	$parts = parse_url("http://".$host.$uri);
	$uri = $parts["path"];
	
	//check prefix
	$prefix = empty(Consts::$path_prefix)?"":"/".preg_replace('/(^\/|\/$)/','',Consts::$path_prefix);
	if(!empty($prefix)){
		if($uri!=$prefix && !str_starts($uri, $prefix."/")){
			//it is a customized path or path not exists
			//$uri = file_exists("PathDelegate")? PathDelegate::rewriteURI($uri) : false;
			$uri = PathDelegate::rewriteURI($uri);
			if(!$uri || ($uri!=$prefix && !str_starts($uri, $prefix."/"))) return error(400);
		}
		$uri = preg_replace('/^\\'.$prefix.'/','', $uri); 	
	}
	
	$fmts = ['json','bson','text','html','csv'];
	if(isset($parts["query"]))
		parse_str(str_replace('&amp;', '&', $parts['query']),$params);

	if(($host=="localhost"||$host=="127.0.0.1") && (str_has($uri,"liber.php")||str_has($uri,"index.php")) && isset($params["__URL__"]) ){
		$uri = $params["__URL__"];
		unset($params["__URL__"]);
	}
	list($uri, $ext) = explode(".", $uri);
	$specifiedFmt = in_array($ext,$fmts);
	if($ext==1||$ext==""||$specifiedFmt){//action
		preg_match_all('/\/(?P<digit>\d+)\/*/', $uri, $matches);
		if(!empty($matches["digit"])){
			$params["@id"] = (int)$matches["digit"][0];
			$uri = preg_replace("/\/\d+\/*/", "/", $uri);
		}
		$params = $params;
		$rest = parse_rest_uri($uri, $method, $params);
		return ['uri'=>$uri, 'method'=>$method, 'params'=>$params, 'format'=>($specifiedFmt)?$format:false] + $rest;
	}else{//static
		$uri = "/webroot".$uri.".$ext";
		return ['uri'=>$uri, 'method'=>$method, 'params'=>$params, 'static'=>true];
	}
}

function parse_rest_uri($uri, $method, &$params){
	$uri = preg_replace("/(^\/)|(\/$)/","",$uri);
	$uparts =ds_remove(explode("/",$uri), "");
	$method = strtolower($method);
	
	if ($method == "put" || $method == "delete") {
        parse_str(file_get_contents('php://input'), $input);
        $params = array_merge($params, $input);
	}
	$target =($method=="post"||$method=="put")?$_POST: $_GET;
	foreach($target as $k=>$v)
		$params[$k] = $v;
	unset($params["__URL__"]);
	$fmts = ['json','bson','text','html','csv'];
	$res = [];
	foreach($params as $k=>$v){
		if($k=="@format" && in_array($v, $fmts)) 
			$res["format"] = $v;
		$params[$k] = htmlEntities($v); //XSS
	}
	unset($params["@format"]);
	if($params["@test_mode"]) $_REQUEST["@test_mode"]=1;
 	unset($params["@test_mode"]);

	$resources = REQ::load_resources();

	list($namespace, $controller, $action) =
		["",Consts::$default_controller,Consts::$default_action];
	$len = count($uparts);
	if(empty($uparts)){$uparts=[$controller,$action];}
	if(count($uparts)==1)$uparts[]=$action;

	if($uri==""){
		$res["uri"] = $controller;
	}else if(in_array($uri,$resources["namespaces"])){//default controller
		$namespace = $uri;
	}else if(in_array($uri,$resources["controllers"])){//controller exist with default action
		$namespace = join("/",array_slice($uparts, 0 , $len-1));
		$controller = $uparts[$len-1];
        $action = $method;
	}else if(in_array(join("/",array_slice($uparts, 0 , $len-1)),$resources["controllers"])){//controller exist with action
		$namespace = join("/",array_slice($uparts, 0 , $len-2));
		$controller = $uparts[$len-2];
		$action = $uparts[$len-1];
	}else{//controller not exist
		if(str_starts($uri,"@")){//check RESTful request with no controller.
			$uri = substr($uparts[0],1);
			if(in_array($uri,$resources["schemas"])){
				$controller = "@REST";
				$res["schema_name"] = $uri;
				$schemaDef =db_schema($uri);
				$res["schema_def"] = $schemaDef;
				$action = $method;
			}
		}else if(in_array(REQ::getClientType()."/".join("_",$uparts), $resources["views"])){//has view template file
			$res["static"] = join("_",$uparts).".html";
		}else error(400,"$uri");
	}

	if(in_array($action,["get","post","put","delete"]) && $method!=$action)
		error(400,'Action Name Permission Error : you can not access the action name with different http method. ');

	$res["namespace"] = $namespace;
	$res["controller"] = $controller;
	$res["action"] = $action;
	$res["params"] = $params;
	//error_log("ns=$namespace, ctrl=$controller, act=$action");
	return $res;
}

function parse_user_agent($ua=""){
	if(empty($ua)) $ua = $_SERVER['HTTP_USER_AGENT'];
	//$br=get_browser($ua); //never use this, it will kill performance !!!
	$type = "pc";
	if(preg_match('/(curl|wget|ApacheBench)\//i',$ua))
		$type = "cmd";
	else if(preg_match('/(iPad|MSIE.*Touch|Android)/',$ua)) //FIXME Android problems later.
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
		$bot = "bot";
	else if(preg_match('/Googlebot-Image|msnbot-media/i',$ua))
		$bot = "ibot";
	else 
		$bot = false;
	
	return ["type"=>$type,"bot"=>$bot];
}

/**
 * Encapsulate request variables for security reason
 */
class REQ {
	
	/** app level properties **/
	private static $resources = null;
	//private static $conf = null;
	
	/** connection level properties **/
	private static $instances = [];
	private static $db = null;
	private static $token = null;
	private static $client_type = "pc";
	private static $template_type = "pc";
	private static $client_bot = false;
	private static $test_mode = false;
	
	/** request level properties **/
	private $data = [];
	private $dispatched = false;
	private $interrupted = false;
	private $redirecting = null;
	private $is_thread = false;
	
	private $render = null;
	private $render_path = null;
	private $render_layout = "_layout.html";

	var $params = [];
	private $response_body = null;
	
	private function __construct(){}
	
	static function getDB(){return self::$db;}
	static function setDB($dbh){if(isset($dbh) && $dbh instanceof PDO)self::$db=$dbh;}
	static function getTemplateType(){return self::$template_type;}
	static function getClientType(){return self::$client_type;}
	/**
	 * get REQ instance from running stack. this is NOT factory method.
	 * @param $idx: get Nth of of instance, -1 means the last one. $idx<0 means get from the end.  
	 * @return : default is root REQ
	 */
	static function getInstance($idx=0){$idx= $idx<0?count(self::$instances)+$idx:$idx;return self::$instances[$idx];}
	static function isTestMode(){return self::$test_mode;}
	static function stackSize(){return count(self::$instances);}

	/**
	 * Dispatch the URI / UserAgent / Params from client. 
	 * create new REQ instance and push to running stack. 
	 * Don't use this directly!
	 * @param $uri : relative url starts with /
	 * @param $method : get|post|put|delete 
	 * @param $params : http request params (hash)
	 * @param $ua : userAgent
	 */
	static function dispatch($uri, $method, $params=[], $ua=""){
		//check cross domain request first.
		if(strtoupper($_SERVER['REQUEST_METHOD'])=="OPTIONS" && strlen(Consts::$cross_domain_methods)>0){
			header("HTTP/1.1 200 OK");
			header('Content-type: application/json');
			header('Access-Control-Allow-Origin: *');
			header('Access-Control-Allow-Methods: '.preg_replace('/\s*,\s*/', ', ', Consts::$cross_domain_methods));
			exit;
		}

		$req = new REQ();
		$ua = parse_user_agent($ua);
		self::$client_type = $ua["type"];
		self::$client_bot = $ua["bot"];
		$req->data = parse_uri($uri, $method, $params, $ua);
		$req->params = $req->data["params"];

		if(Consts::$session_enable && !isset($_SESSION))
			Session::start();
		//var_dump($req->data);
	
		self::$instances[]=$req; // push to running stack
		if(count(self::$instances)>1){
			$req->is_thread = true;
		}
		
		if(!empty($req->data["static"]))
			return render_html($req->data["static"]);
	
		return $req->process();
	}
	

	/**
	 * Preload all schemas, controllers, view files ... and save to APC
	 */
	static function load_resources(){
		if(self::$resources)
			return self::$resources;

		self::$resources = cache_get("APP_RESOURCES", function($key){
			$ver = exec("cd ".APP_DIR."/; git log -1 | head -n 1 | awk '{print \$2}' ");
			if(empty($ver)){
				$lastTS = exec("cd ".APP_DIR."/;ls -lt | head -n 2 | tail -n 1 | awk '{print \$6,\$7,\$8}' ");
				$ver = strtotime($lastTS);
			}
			
			$ctrldir = APP_DIR.__SLASH__."controllers";
			exec("find $ctrldir",$res);
			$namespaces = []; $controllers = [];
			foreach($res as $f){
				$namespaces []= strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."controllers")."/",'/\/(.*)\.inc$/',"/^\//"],["","",""], $f));
				if(str_ends($f,".inc")){
					$ctl = strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."controllers")."/",'/\.inc$/',"/^\//"],["","",""], $f));
					$controllers[]= $ctl;
				}
			}
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
			return [
			"version"		=> $ver,
			"namespaces" 	=> ds_remove(array_unique($namespaces), ""),
			"controllers" 	=> ds_remove(array_unique($controllers), ""),
			//"permission"	=> $permission, permission will be loaded in Auth::checkPermission
			"schemas"		=> ds_remove($schemas, ""),
			"views"			=> ds_remove($views, ["","pc","sm","bot","ibot","pad","mail"]),
			"view_types"	=> array_map(function($e){
				return strtolower(preg_replace(["/^".str_replace("/","\/",APP_DIR.__SLASH__."views".__SLASH__)."/","/^\//"],["",""], $e));
			},$view_types)
			];
		},false);
		return self::$resources;
	}
	
	/**
	 * finish current HTTP connection and do after jobs like close db. 
	 */
	static function quit(){
		$last = array_pop(self::$instances);
		if($last)
			$last->interrupted = true;
		if(empty(self::$instances)){
			self::$db = null;
			exit;
		}
	}

	/**
	* write to response.
	* @param text: things to output. 
	* @param format: text | json | html
	*/
	static function write($text, $format){
		if((self::$test_mode||$_REQUEST["@test_mode"]) && $format=="json"){
			Tests::writeJSON($text);
		}else
			echo $text;
	}
	
	function getRender($path=null){
		if(!isset($this->render)){
			$src = self::load_resources();
			$vtypes = $src["view_types"];
			self::$template_type = in_array(self::$client_type,$vtypes)? self::$client_type:"pc";
			
			$data = $this->data;
			if ($path==null){
				$path = APP_DIR.__SLASH__."views".__SLASH__.self::$template_type;
				$path = $data["namespace"]==""? $path:$path."/".$data["namespace"];
			}
			$render = Render::factory($path);
			$render->assign('CLIENT_TYPE',self::$template_type);
			$render->assign('controller',$data["controller"]);
			$render->assign('action',$data["action"]);
			$render->assign('APP_VER',$src["version"]);
			$this->render_path = $path;
			$this->render = $render;
		}
		return $this->render;
	}
	function getRenderPath(){return $this->render_path;}
	function setRenderPath($path){if(isset($path) && is_string($path))$this->render_path=$path;}
	function getRenderLayout(){return $this->render_layout;}
	function setRenderLayout($path){if(isset($path) && is_string($path))$this->render_layout=$path;}
	
	function getNamespace(){return $this->data["namespace"];}
	function getController(){return $this->data["controller"];}
	function getAction(){return $this->data["action"];}
	function getFormat(){return $this->data["format"];}
	function getURI(){return $this->data["uri"];}
	function getMethod(){return $this->data["method"];}
	function getData($key){return empty($this->data)?null:$this->data[$key];}
	
	
	function setResponseBody($body){if(isset($body) && is_string($body))$this->response_body=$body;}
	
	/**
	 * Core function, all requests URLs are dispatched here.
	 */
	public function process(){
		if($this->dispatched===true)return;
        if (Consts::$mode=="Developing") {
            error_log("URI: " . $this->getURI());
            error_log("METHOD: " . $_SERVER['REQUEST_METHOD']);
            error_log("PARAMETERS: " . json_encode($this->params));
        }

        try{
        	$auth = Auth::checkPermission($this);

			if($auth != 200){
				if($auth == 401)  return error(401, "Permission ERROR : Sorry, You are not permited to do that.");
				if($auth == 403)  return error(403);
			}
			
			$data = $this->data;
			$filterNames = [];$filterCls = [];
			foreach (Consts::$filters as $fn => $pt) {
				if($pt=='*' || preg_match($pt, substr($this->data['uri'],1)))
					$filterNames[]=$fn;
			}
			$size = count($filterNames);

			for ($token=$size*(-1); $token<=$size; $token++){
				if(true===$this->interrupted)
					break;
				if ($token == 0){//Exec controller->actions
					if(!empty($data["schema_name"]))
						$this->process_rest();
					else
						$this->process_normal();
				}else if($size>0){//Exec filters
					$nextIdx = $token < 0 ? $size + $token : $size - $token ;
					$filterName = $filterNames[$nextIdx];
					if(!empty($filterName)){
						$existsFilter = array_key_exists($filterName, $filterCls);
						$filter = $existsFilter? $filterCls[$filterName]: Filter::factory($filterName);
						if(!$existsFilter){
							$filterCls[$filterName] = $filter;
						}
						($token<0) ? $filter->before($this->params, $authRequired) : $filter->after($this->params, $authRequired);
					}
				}
			}
				
			if(isset($this->redirecting)){
				redirect($this->redirecting);
			}
			//show
			if(!isset($this->response_body)){
				render_html();
			}
			//echo $_REQUEST["RESPONSE_BODY"];
		}catch(Exception $e){
			error_log("exec exception");
			print $e->getMessage();
		}
		//echo $response;
		REQ::quit();
	}
	
	
	/**
	 * Controller function, Process normal requests.
	 * Obey the rules of /...PATH/CONTROLLER_NAME/ACTION_NAME?parameters
	 */
	private function process_normal($useSession=true){
		try {
			//FIXME add namespace | customize 1st level path
			$ctrldir = APP_DIR.__SLASH__.'controllers'.__SLASH__;
			$data = $this->data;
			$controller_dir = !empty($data['namespace']) ? $ctrldir.$data['namespace'].'/':$ctrldir;
			$file_path = $controller_dir.$data['controller'].".inc";
			require_once $file_path;
			//process
			$action = $data["action"];

			$exec = function($action){
				$has_wrapper =  !isset($exclude_wrappers) || !in_array($action, $exclude_wrappers);
				if (function_exists("before_wrapper") && $has_wrapper)
					before_wrapper($this->params);
				$action($this->params);
				if (function_exists("after_wrapper")  && $has_wrapper){
					if($data["format"]!="html"){//use only on json or text, html should use in smarty.
						after_wrapper($this->params);
					}else{
						$_REQUEST["after_wrapper"] = true;
					}
				}
			};

			if(function_exists($action)){//normal request
				$exec($action);
			}else if(Consts::$mode=="Developing" && str_starts($action,"test_") && function_exists(str_replace("test_","",$action))){//unit test
				self::$test_mode = true;
				return Tests::run($data["controller"],str_replace("test_","",$action));
			}else if(function_exists("__magic")){
				$this->params["@path"]=$action;
				$exec("__magic");
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
	private function process_rest(){
		$schema = $this->data["schema_name"];
		$schemaDef =$this->data["schema_def"];
		$method = strtolower($_SERVER["REQUEST_METHOD"]);
		$pk = $schemaDef["general"]["pk"];
		$params = $this->params;
		if(isset($params[$pk]) && !isset($params["@id"]))
			$params["@id"] = $params[$pk];
		$delegate_name = $schema."_".$this->data["action"];  
		if(!method_exists("RestfulDelegate", $delegate_name)){
			switch(strtolower($_SERVER["REQUEST_METHOD"])){
				case "get"	:return $this->rest_get($schema,$params);
				case "post"	:return $this->rest_post($schema,$params);
				case "put"	:return $this->rest_put($schema,$params);
				case "delete":return $this->rest_delete($schema,$params);
				default : return error(401,"RESTful ERROR : Sorry, You are not permited to do that.");
			}
		}else{
			$re = call_user_func(["RestfulDelegate", $delegate_name]);
			if(!$re) error(401, "RESTful ERROR : Sorry, You are not permited to do that.");
		}
	}
	
	private function rest_get($schema, $params){
		$res = (isset($params["@id"]))?
			db_find1st($schema, $params):
			db_find($schema, $params);
		render_json($res);
	}
	
	private function rest_post($schema, $params){
		if(isset($params["@id"])){
			error(400,"RESTful ERROR : Sorry, You can't use RESTful POST with @id, try PUT for update or using normal controllers");
		}else{
			return render_json(db_save($schema, $params, true));
		}
	}
	private function rest_put($schema, $params){
		if(isset($params["@id"])){
			return render_json(db_save($schema, $params));
		}else{
			error(400,"RESTful ERROR : You must specify a @id to use RESTful PUT");
		}
	}
	private function rest_delete($schema, $params){
		if(isset($params["@id"])){
			return render_json(db_delete($schema, $params));
		}else{
			error(400,"RESTful ERROR : You must specify a @id to use RESTful DELETE");
		}
	}
}




?>
