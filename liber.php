<?php
/**
 *	@file: liber.php
 *	@author: Soyoes 2014/01/28
 *****************************************************************************/
require 'conf/conf.inc';
const LIBER_DIR = '/Users/soyoes/Dropbox/Develop/php/liberPHP2';
const __SLASH__ = DIRECTORY_SEPARATOR;
define('APP_DIR', dirname(__FILE__));
define('IMAGE_DIR', APP_DIR . __SLASH__ . "webroot" . __SLASH__ . "images" . __SLASH__);
define('APP_NAME', end(explode("/", APP_DIR)));

set_include_path(
get_include_path(). PATH_SEPARATOR
. LIBER_DIR .__SLASH__. 'modules'.__SLASH__.'utils'. PATH_SEPARATOR
. APP_DIR.__SLASH__.'delegate'.PATH_SEPARATOR
. APP_DIR.__SLASH__.'modules'.__SLASH__ 
);
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
function assign($key, $value){
	$render = REQ::getInstance()->getRender();
	$render->assign($key, $value);
}
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
	if(file_exists($path.'/'.$template_file)){		render_html($template_file);
	}else{
				error(400,'json','action does not exist.');
	}
	REQ::quit();
}
function keygen($len,$chars=false){
	if(!isset($len))$len=16;
	if(!$chars) $chars = 'abcdefghijklmnopqrstuvwxyz0123456789_.;,-$%()!@';
	$key='';$clen=strlen($chars);
	for($i=0;$i<$len;$i++){
		$key.=$chars[rand(0,$clen-1)];
	}
	return $key;
}
function error($code, $contentType, $reason=''){
	if(empty($reason)&&!empty($contentType)&&!in_array($contentType, ['html','json','text'])){
		$reason=$contentType;$contentType='json';
	}
	$msg = Consts::$error_codes[''.$code];
	header('HTTP/1.1 '.$code.' '.$msg, FALSE);
	$req = REQ::getInstance();
	$src = REQ::load_resources();
	$type = REQ::getClientType();
	$hasHtml = in_array($type.'/error_'.$code, $src['views']);
	if(isset($contentType)&&!in_array($contentType, ['html','json','text'])){
		if(empty($reason)) {
			$reason=$contentType;
			$contentType=null;
		}
	}	
	if(!$contentType)
		$contentType=$hasHtml?'html':'text';
	switch($contentType){
		case 'json':
			header('Content-type: application/json; charset=utf-8');
			echo '{"error":"'."$code $msg. $reason".'"}';
			break;
		case "html":
			header('Content-type: text/html; charset=utf-8');
			if($hasHtml)
				render_html('error_$code.html',['code'=>$code,'msg'=>$msg,'reason'=>$reason]);
			else
				echo "<HTML><BODY><h1>$code ERROR</h1></BODY></HTML>";
			break;
		default:			header('Content-type: text/plain; charset=utf-8');
			echo "$code ERROR: $msg. $reason";
			break;
	}
	REQ::quit();
}
function is_https() {
    if(!empty($_SERVER['HTTP_X_FORWARDED_PROTO']))
        return strtoupper($_SERVER['HTTP_X_FORWARDED_PROTO']) == "HTTPS";
    else
        return (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') || $_SERVER['SERVER_PORT'] == 443;
}
function redirect($url,$clientRedirect=false) {
	$appName = str_has($_SERVER['REQUEST_URI'],APP_NAME.'/')?
	APP_NAME.'/':'';
	$redirectUrl = str_starts($url, 'http:') || str_starts($url, 'https:') ?
	   $url : (is_https() ? 'https' : 'http') . '://'.$_SERVER['HTTP_HOST'].'/'.$appName . $url;
		if(!$clientRedirect){
		header('HTTP/1.1 301 Moved Permanently');
		header('Location: '.$redirectUrl);
	}else{
		header('Content-type: text/html; charset=utf-8');
		echo '<script type="text/javascript">window.location="'.$redirectUrl.'";</script>';
	}
	REQ::quit();
}
function call($url, $method, $data = [], $header = [], $options = []) {
    $method = strtoupper($method);
    $defaults = $method == 'POST' || $method == 'PUT' ? [
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
function async($msg,$func){
	ob_end_clean();
	header('Connection: close');
	ignore_user_abort(true); 	ob_start();
	echo $msg;
	$size = ob_get_length();
	header("Content-Length: $size");
	ob_end_flush(); 
	flush();
	$args = array_slice(func_get_args(), 2);
	call_user_func_array ($func, $args);
}
function T($key){
	$filename = null;
	$lang = isset($_REQUEST['@lang'])?$_REQUEST['@lang']:
					(!empty($_SESSION['lang']) ? $_SESSION['lang']:
						(isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])?
							substr($_SERVER['HTTP_ACCEPT_LANGUAGE'], 0, 2):
							Consts::$lang));
	$text_func = function($fn){
		$file = join(__SLASH__,[APP_DIR,'conf','text.csv']);
		if (file_exists($file)){
			$lines = preg_split('/[\r\n]+/',file_get_contents($file));
			$idx = 0;
			$res = [];$langs = [];
			if (($handle = fopen($file, 'r')) !== FALSE) {
				$max_len = 0; 				$delimiter = ',';
				try {
					while (($cols = fgetcsv($handle, $max_len, $delimiter)) !== FALSE) {
						if($idx++==0){
							if($cols[0]!='id'){
								error_log('Language File Error: the first column of text.csv must have a name of "id" ');
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
					error_log('Language File Error: '.$e->getMessage());
				}
			}
			fclose($handle);
			return $res;
		}return [];
	};
	$texts = Consts::$mode=='Developing'?$text_func('__TEXTS__'):cache_get('__TEXTS__', $text_func);
	$lang = isset($texts[$key][$lang]) ? $lang : (isset($texts[$key][Consts::$lang]) ? Consts::$lang : false);
	if($lang){
		$text = $texts[$key][$lang];
		if(str_has($text,'%')){
			$args = array_slice(func_get_args(), 1);
			$enc = mb_detect_encoding($text);
			return $lang=='en'? vsprintf($text, $args) : 
				mb_convert_encoding(vsprintf(mb_convert_encoding($text,'UTF-8',$enc), $args),$enc,'UTF-8');
		}else
			return $text;
	}else{
		error_log("__ERR_WORD_NOT_EXISTS_($key,$lang)__, please check your /conf/text.csv");
		return null;
	}
}
function parse_uri($uri, $method, &$params=[], $ua=""){
	$host =$_SERVER['SERVER_NAME'];
	if(empty($uri)) $uri = $_SERVER['REQUEST_URI'];
	if(empty($method)) $method = $_SERVER['REQUEST_METHOD'];
	if(empty($ua)) $ua = $_SERVER['HTTP_USER_AGENT'];
		if(!empty(Consts::$path_prefix)) 
		$uri =  preg_replace('/'.preg_quote(Consts::$path_prefix, '/').'/', '', $uri, 1);
		$uri = PathDelegate::rewriteURI($uri);
	$uri = htmlEntities($uri, ENT_QUOTES|ENT_HTML401);
	$uri = preg_replace(['/\sHTTP.*/','/(\/)+/','/\/$/','/^[a-zA-Z0-9]/'], ['','/','',"/$1"], $uri);
	$parts = parse_url('http://'.$host.$uri);
	$uri = $parts['path'];
	$fmts = ['json','bson','text','html','csv'];
	if(isset($parts['query']))
		parse_str(str_replace('&amp;', '&', $parts['query']),$params);
		if(($host=='localhost'||$host=="127.0.0.1") && (str_has($uri,'liber.php')||str_has($uri,'index.php')) && isset($params['__URL__']) ){
		$uri = $params['__URL__'];
		unset($params['__URL__']);
	}
	list($uri, $ext) = explode('.', $uri);
	$specifiedFmt = in_array($ext,$fmts);
	if($ext==1||$ext==""||$specifiedFmt){		preg_match_all('/\/(?P<digit>\d+)\/*/', $uri, $matches);
		if(!empty($matches['digit'])){
			$params['@id'] = intval($matches['digit'][0]);
			$uri = preg_replace('/\/\d+\/*/', '/', $uri);
		}
		$rest = parse_rest_uri($uri, $method, $params);
		return ['uri'=>$uri, 'method'=>$method, 'params'=>$params, 'format'=>($specifiedFmt)?$format:false] + $rest;
	}else{		$uri = '/webroot'.$uri.'.'.$ext;
		return ['uri'=>$uri, 'method'=>$method, 'params'=>$params, 'static'=>true];
	}
}
function parse_rest_uri($uri, $method, &$params){
	$uri = preg_replace('/(^\/)|(\/$)/','',$uri);
	$uparts =ds_remove(explode('/',$uri), '');
	$method = strtolower($method);
	if ($method == 'put' || $method == 'delete') {
        parse_str(file_get_contents('php://input'), $input);
        $params = array_merge($params, $input);
	}
	$target =($method=='post'||$method=='put')?$_POST: $_GET;
	foreach($target as $k=>$v)
		$params[$k] = $v;
	unset($params['__URL__']);
	$fmts = ['json','bson','text','html','csv'];
	$res = [];
	foreach($params as $k=>$v){
		if($k=='@format' && in_array($v, $fmts)) 
			$res['format'] = $v;
		$params[$k] = htmlEntities($v); 	}
	unset($params['@format']);
	if($params['@test_mode']) $_REQUEST['@test_mode']=1;
 	unset($params['@test_mode']);
	$resources = REQ::load_resources();
	list($namespace, $controller, $action) =
		['',Consts::$default_controller,Consts::$default_action];
	$len = count($uparts);
	if(empty($uparts)){$uparts=[$controller,$action];}
	if(count($uparts)==1)$uparts[]=$action;
	if($uri==''){
		$res['uri'] = $controller;
	}else if(in_array($uri,$resources['namespaces'])){		$namespace = $uri;
	}else if(in_array($uri,$resources['controllers'])){		$namespace = join('/',array_slice($uparts, 0 , $len-1));
		$controller = $uparts[$len-1];
        $action = $method;
	}else if(in_array(join('/',array_slice($uparts, 0 , $len-1)),$resources['controllers'])){		$namespace = join('/',array_slice($uparts, 0 , $len-2));
		$controller = $uparts[$len-2];
		$action = $uparts[$len-1];
	}else{		if(str_starts($uri,'@')){			$uri = substr($uparts[0],1);
						if(in_array($uri,$resources['schemas'])){
				$controller = '@REST';
				$res['schema_name'] = $uri;
				$schemaDef =db_schema($uri);
				$res['schema_def'] = $schemaDef;
				$action = $method;
			}
		}else if(in_array(REQ::getClientType().'/'.join('_',$uparts), $resources['views'])){			$res['static'] = join('_',$uparts).'.html';
		}else error(400,$uri);
	}
	if(in_array($action,['get','post','put','delete']) && $method!=$action)
		error(400,'Action Name Permission Error : you can not access the action name with different http method. ');
	$res['namespace'] = $namespace;
	$res['controller'] = $controller;
	$res['action'] = $action;
	$res['params'] = $params;
	return $res;
}
function parse_user_agent($ua=""){
	if(empty($ua)) $ua = $_SERVER['HTTP_USER_AGENT'];
		$type = 'pc';
	if(preg_match('/(curl|wget|ApacheBench)\//i',$ua))
		$type = 'cmd';
	else if(preg_match('/(iPhone|iPod|(Android.*Mobile)|BlackBerry|IEMobile)/i',$ua))
		$type = 'sm';
	else if(preg_match('/(iPad|MSIE.*Touch|Android)/',$ua))
		$type = 'pad';
		if(preg_match('/Googlebot|bingbot|msnbot|Yahoo|Y\!J|Yeti|Baiduspider|BaiduMobaider|ichiro|hotpage\.fr|Feedfetcher|ia_archiver|Tumblr|Jeeves\/Teoma|BlogCrawler/i',$ua))
		$bot = 'bot';
	else if(preg_match('/Googlebot-Image|msnbot-media/i',$ua))
		$bot = 'ibot';
	else 
		$bot = false;
	return ['type'=>$type,'bot'=>$bot];
}
class REQ {
	private static $resources = null;
	private static $instances = [];
	private static $db = null;
	private static $token = null;
	private static $client_type = 'pc';
	private static $template_type = 'pc';
	private static $client_bot = false;
	private static $test_mode = false;
	private $data = [];
	private $dispatched = false;
	private $interrupted = false;
	private $redirecting = null;
	private $is_thread = false;
	private $render = null;
	private $render_path = null;
	private $render_layout = '_layout.html';
	var $params = [];
	private $response_body = null;
	private function __construct(){}
	static function getDB(){return self::$db;}
	static function setDB($dbh){if(isset($dbh) && $dbh instanceof PDO)self::$db=$dbh;}
	static function getTemplateType(){return self::$template_type;}
	static function getClientType(){return self::$client_type;}
	static function getInstance($idx=0){$idx= $idx<0?count(self::$instances)+$idx:$idx;return self::$instances[$idx];}
	static function isTestMode(){return self::$test_mode;}
	static function stackSize(){return count(self::$instances);}
	static function dispatch($uri, $method, $params=[], $ua=''){
				if(strtoupper($_SERVER['REQUEST_METHOD'])=='OPTIONS' && strlen(Consts::$cross_domain_methods)>0){
			header('HTTP/1.1 200 OK');
			header('Content-type: application/json');
			header('Access-Control-Allow-Origin: *');
			header('Access-Control-Allow-Methods: '.preg_replace('/\s*,\s*/', ', ', Consts::$cross_domain_methods));
			exit;
		}
		$req = new REQ();
		self::$instances[]=$req; 		$ua = parse_user_agent($ua);
		self::$client_type = $ua['type'];
		self::$client_bot = $ua['bot'];
		$req->data = parse_uri($uri, $method, $params, $ua);
		$req->params = $req->data['params'];
		if(Consts::$session_enable && !isset($_SESSION))
			Session::start();
		if(count(self::$instances)>1){
			$req->is_thread = true;
		}
		if(!empty($req->data['static']))
			return render_html($req->data['static']);
		return $req->process();
	}
	static function load_resources(){
		if(self::$resources)
			return self::$resources;
		self::$resources = cache_get('APP_RESOURCES', function($key){
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
				'version'		=> $ver,
				'namespaces' 	=> ds_remove(array_unique($namespaces), ''),
				'controllers' 	=> ds_remove(array_unique($controllers), ''),
				'schemas'		=> ds_remove($schemas, ''),
				'views'			=> ds_remove($views, ['','pc','sm','bot','ibot','pad','mail']),
				'view_types'	=> array_map(function($e){
					return strtolower(preg_replace(['/^'.str_replace('/','\/',APP_DIR.__SLASH__.'views'.__SLASH__).'/','/^\//'],['',''], $e));
				},$view_types)
			];
		},false);
		return self::$resources;
	}
	static function quit(){
		$last = array_pop(self::$instances);
		if($last)
			$last->interrupted = true;
		if(empty(self::$instances)){
			self::$db = null;
			exit;
		}
	}
	static function write($text, $format){
		if((self::$test_mode||$_REQUEST['@test_mode']) && $format=='json'){
			Tests::writeJSON($text);
		}else
			echo $text;
	}
	function getRender($path=null){
		if(!isset($this->render)){
			$src = self::load_resources();
			$vtypes = $src['view_types'];
			self::$template_type = in_array(self::$client_type,$vtypes)? self::$client_type:'pc';
			$data = $this->data;
			if ($path==null){
				$path = APP_DIR.__SLASH__.'views'.__SLASH__.self::$template_type;
				$path = $data['namespace']==''? $path:$path.'/'.$data['namespace'];
			}
			$render = Render::factory($path);
			$render->assign('CLIENT_TYPE',self::$template_type);
			$render->assign('controller',$data['controller']);
			$render->assign('action',$data['action']);
			$render->assign('APP_VER',$src['version']);
			$this->render_path = $path;
			$this->render = $render;
		}
		return $this->render;
	}
	function getRenderPath(){return $this->render_path;}
	function setRenderPath($path){if(isset($path) && is_string($path))$this->render_path=$path;}
	function getRenderLayout(){return $this->render_layout;}
	function setRenderLayout($path){if(isset($path) && is_string($path))$this->render_layout=$path;}
	function getNamespace(){return $this->data['namespace'];}
	function getController(){return $this->data['controller'];}
	function getAction(){return $this->data['action'];}
	function getFormat(){return $this->data['format'];}
	function getURI(){return $this->data['uri'];}
	function getMethod(){return $this->data['method'];}
	function getData($key){return empty($this->data)?null:$this->data[$key];}
	function setResponseBody($body){if(isset($body) && is_string($body))$this->response_body=$body;}
	public function process(){
		if($this->dispatched===true)return;
        if (Consts::$mode!='Product') {
            error_log('URI: ' . $this->getURI());
            error_log('METHOD: ' . $_SERVER['REQUEST_METHOD']);
            error_log('PARAMETERS: ' . json_encode($this->params));
        }
        try{
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
				if ($token == 0){					$per = permission();
					if($per != 200){
						if($per == 401)  return error(401, 'Permission ERROR : Sorry, You are not permited to do that.');
						if($per == 403)  return error(403);
					}
					if(!empty($data['schema_name']))
						$this->process_rest();
					else
						$this->process_normal();
				}else if($size>0){					$nextIdx = $token < 0 ? $size + $token : $size - $token ;
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
						if(!isset($this->response_body)){
				render_html();
			}
					}catch(Exception $e){
			error_log('exec exception');
			print $e->getMessage();
		}
				REQ::quit();
	}
	private function process_normal(){
		try {
						$ctrldir = APP_DIR.__SLASH__.'controllers'.__SLASH__;
			$data = $this->data;
			$controller_dir = !empty($data['namespace']) ? $ctrldir.$data['namespace'].'/':$ctrldir;
			$file_path = $controller_dir.$data['controller'].'.inc';
			require_once $file_path;
						$action = $data['action'];
			$exec = function($action){
								$has_wrapper =  !isset($exclude_wrappers) || !in_array($action, $exclude_wrappers);
				if (function_exists('before_wrapper') && $has_wrapper)
					before_wrapper($this->params);
				$action($this->params);
				if (function_exists('after_wrapper')  && $has_wrapper){
					if($data['format']!='html'){						after_wrapper($this->params);
					}else{
						$_REQUEST['after_wrapper'] = true;
					}
				}
			};
			if(function_exists($action)){				$exec($action);
			}else if(Consts::$mode=='Developing' && str_starts($action,'test_') && function_exists(str_replace('test_','',$action))){				self::$test_mode = true;
				return Tests::run($data['controller'],str_replace('test_','',$action));
			}else if(function_exists('__magic')){
				$this->params['@path']=$action;
				$exec('__magic');
			}else{				return render_default_template();
			}
		} catch(Exception $e) {
			echo $e->getMessage();
			throw new Exception($controllerName.',Controller not found');
		}
	}
	private function process_rest(){
		$schema = $this->data['schema_name'];
		$schemaDef =$this->data['schema_def'];
		$method = strtolower($_SERVER['REQUEST_METHOD']);
		$pk = $schemaDef['general']['pk'];
		$params = $this->params;
		if(isset($params[$pk]) && !isset($params['@id']))
			$params['@id'] = $params[$pk];
		$delegate_name = $schema.'_'.$this->data['action'];  
		if(!method_exists('RestfulDelegate', $delegate_name)){
			switch(strtolower($_SERVER['REQUEST_METHOD'])){
				case 'get'	:return $this->rest_get($schema,$params);
				case 'post'	:return $this->rest_post($schema,$params);
				case 'put'	:return $this->rest_put($schema,$params);
				case 'delete':return $this->rest_delete($schema,$params);
				default : return error(401,'RESTful ERROR : Sorry, You are not permited to do that.');
			}
		}else{
			$re = call_user_func(['RestfulDelegate', $delegate_name]);
			if(!$re) error(401, 'RESTful ERROR : Sorry, You are not permited to do that.');
		}
	}
	private function rest_get($schema, $params){
		$res = (isset($params['@id']))?
			db_find1st($schema, $params):
			db_find($schema, $params);
		render_json($res);
	}
	private function rest_post($schema, $params){
		if(isset($params['@id'])){
			error(400,'RESTful ERROR : Sorry, You can\'t use RESTful POST with @id, try PUT for update or using normal controllers');
		}else{
			return render_json(db_save($schema, $params, true));
		}
	}
	private function rest_put($schema, $params){
		if(isset($params['@id'])){
			return render_json(db_save($schema, $params));
		}else{
			error(400,'RESTful ERROR : You must specify a @id to use RESTful PUT');
		}
	}
	private function rest_delete($schema, $params){
		if(isset($params['@id'])){
			return render_json(db_delete($schema, $params));
		}else{
			error(400,'RESTful ERROR : You must specify a @id to use RESTful DELETE');
		}
	}
}
function permission(){
	$req = REQ::getInstance();
	$uri = $req->getURI();
	$schemaDef = $req->getData('schema_def');
	$group = AuthDelegate::group();
	$permission = '';
	if(!empty($schemaDef)){		$restful =  strtolower($schemaDef['general']['restful']?$schemaDef['general']['restful']:'');
				if(!empty($restful) && $restful!='all' && !str_has($restful, $method)){ return false; }
		$permission = isset($schemaDef['general']['permission'])?$schemaDef['general']['permission']:'';
	}else{
		$ctl = $req->getController();
		$ns  = $req->getNamespace();
		$tree = cache_get('APP_PERMISSION_'.$ns.'_'.$ctl, function($key){
			$ctrldir = APP_DIR.__SLASH__.'controllers'.__SLASH__;
			$req = REQ::getInstance();
			$ns = $req->getNamespace();
			$controller_dir = !empty($ns) ? $ctrldir.$ns.'/':$ctrldir;
			$fp = $controller_dir.str_replace('APP_PERMISSION_'.$ns.'_','',$key).'.inc';
			$tree = fs_src_tree($fp);
						$permission = [];
			foreach ($tree['functions'] as $fn => $ftr){
				$permission[$fn] = $ftr['annotations']['permission'];
			}
			return $permission; 
		},false);
		$act = $req->getAction();
				$permission = isset($tree[$act])?$tree[$act]:'F';
	}
	$bits = isset($permission)&&isset($permission[$group])?$permission[$group]:($group==0?'8':'F');
	if($bits=='0') return $group==0? 401 : 403;
		$bits = base_convert($bits, 16, 2); 
		$bitIdx = array_search($req->getMethod(), ['get','post','put','delete']);
	if($bits[$bitIdx]!='1') return $group==0? 401 : 403;
	return 200;
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
function is_json($str){
	return is_object(json_decode($str));
}
function hash_incr($data, $key, $amount){
	$v = self::get($data,$key,true,0);
	$v += $amount;
	return self::set($data, $key, $v);
}
function hash_set(&$data, $keyPath, $val){
	$paths = explode('.', $keyPath);
	$o = &$data;
	$current_path = '';
	$path_size = count($paths);
	$key = $paths[0];
	$org = isset($data[$key])? $data[$key]: null;
	for ($i=0; $i<$path_size; $i++){
		$path = $paths[$i];
		if (is_string($o) && (str_starts($o, '{') || Strings::startsWith($o, '[')))
			$o = json_decode($o,true);
		if ($i == $path_size-1){
			$o[$path] = $val;
		}else{
			if (!isset($o[$path]))
				$o[$path] = [];
			$o = &$o[$path];
		}
	}
	return ['key'=>$key, 'val'=>$data[$key], 'org'=>$org];
}
function hash_get(&$data, $keyPath, $autoCreate=true, $defaultValue=null){
	if (empty($data)) {
		if($autoCreate){
			hase_set($data, $keyPath, $defaultValue);
		}else
			return $defaultValue;
	}
	$paths = explode('.', $keyPath);
	$o = $data;
	$current_path = '';
	while (count($paths)>1){
		$path = array_shift($paths);
		if (is_string($o) && (str_starts($o, '{') || Strings::startsWith($o, '[')))
			$o = json_decode($o,true);
		if (!isset($o[$path])){
			return $defaultValue;
		}
		$o = $o[$path];
	}
	if (is_string($o) && (str_starts($o, '{') || Strings::startsWith($o, '[')))
		$o = json_decode($o,true);
	$key = array_pop($paths);
	if(!isset($o[$key]))
		return $defaultValue;
	return $o[$key];
}
function arr2hash($arr, $keyName, $valueName=null){
	$hash = [];
	foreach ($arr as $e){
		$hash[''.$e[$keyName]] = $valueName==null ? $e : $e[$valueName];
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
		'=' 	=> function($o,$k,$v){return $o[$k]===$v;},
		'!' 	=> function($o,$k,$v){return $o[$k]!==$v;},
		'<' 	=> function($o,$k,$v){return $o[$k]<$v;},
		'>' 	=> function($o,$k,$v){return $o[$k]>$v;},
		'<=' 	=> function($o,$k,$v){return $o[$k]<=$v;},
		'>=' 	=> function($o,$k,$v){return $o[$k]>=$v;},
		'[]' 	=> function($o,$k,$v){return is_array($v)&&in_array($o[$k],$v);},
		'![]' 	=> function($o,$k,$v){return is_array($v)?!in_array($o[$k],$v):true;},
		'()' 	=> function($o,$k,$v){return is_array($v) && count($v)==2 && $o[$k]>=min($v[0],$v[1]) && $o[$k]<=max($v[0],$v[1]);},
		'!()' 	=> function($o,$k,$v){return !is_array($v) || count($v)<2 || $o[$k]<min($v[0],$v[1]) || $o[$k]>max($v[0],$v[1]);},
		'?'  	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && str_has($o[$k], $v); },
		'!?'  	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !str_has($o[$k], $v); },
		'~' 	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && preg_match('/'.$v.'/', $o[$k]);},
		'!~'	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !preg_match('/'.$v.'/', $o[$k]);},
		'~~' 	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && preg_match('/'.$v.'/i', $o[$k]);},
		'!~~'	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !preg_match('/'.$v.'/i', $o[$k]);},
	];
	if(empty($opts))return false; 
	$res = [];
	foreach ($arr as $a){
		$match = true;
		foreach ($opts as $k=>$v){
			$cmd = strstr($k, '@');
			$cmd = !$cmd ? "=":substr($k, $cmd);
			$func = Consts::$arr_query_filters[$cmd];
			if ($func && !$func($a,$k,$v)){
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
			$cmp = create_function("$a, $b", $code);
			usort($arr, $cmp);
		}else
			usort($arr, $comparator);
		return $arr;
	}else{
		asort($arr);
		return $arr;
	}
}
function ms(){
    list($usec, $sec) = explode(' ', microtime());
    return ((int)((float)$usec*1000) + (int)$sec*1000);
}
function fs_put_ini($file, array $options){
	$tmp = '';
	foreach($options as $section => $values){
		$tmp .= "[$section]\n";
		foreach($values as $key => $val){
			if(is_array($val)){
				foreach($val as $k =>$v)
					$tmp .= "{$key}[$k] = \'$v\'\n";
			}else
				$tmp .= "$key = \'$val\'\n";
		}
		$tmp .= '\n';
	}
	file_put_contents($file, $tmp);
	unset($tmp);
}
function fs_archived_path ($id, $tokenLength=1000){
	$arch =  (int)$id % (int)$tokenLength;
	return "$arch/$id";
}
function fs_mkdir($out){
	$folder = (str_has($out,'.'))? preg_replace('/[^\/]*\.[^\/]*$/','',$out):$out;
	if(!file_exists($folder))
		mkdir($folder, 0775, TRUE);
}
function fs_xml2arr($xmlString){
	return json_decode(json_encode((array)simplexml_load_string($xmlString, 'SimpleXMLElement', LIBXML_NOCDATA)), TRUE);
}
function fs_annotations($comm){
	$comm = explode("\n",preg_replace(['/\/\*+\s*/m','/\s*\*+\/\s*/m'],'',$comm));
	$anno = [];
	$rows = count($comm);
	$tag = null; $value=[]; $attr= null;
	for($i=0;$i<=$rows;$i++){
		$cm = trim(preg_replace('/^[\s\*]*/','',$i<$rows?$comm[$i]:''));
		preg_match_all('/^@(?P<tag>[a-zA-Z]+)\s*(?P<attr>[^:^=]*)\s*[:=]*\s*(?P<value>.*)/i',$cm,$matches);
				if(!empty($matches['tag']) || $i==$rows){
			if(empty($tag))$tag = 'desc';
			if(empty($anno[$tag]))
				$anno[$tag] = [];
			$anno[$tag] []= ['value'=>join("\n", $value),'attr'=>$attr];
			$tag = null; $value=[]; $attr = null;
		}
				if(!empty($matches['tag'])){
			$tag = trim(strtolower($matches['tag'][0]));
			$value []= preg_replace('/^[:\s]*/','',trim($matches['value'][0]));
			$attr = preg_replace('/^[:\s]*/','',$matches['attr'][0]);
		}else if(!empty($cm)){
			$value []= $cm;
		}
	}
	foreach ($anno as $key=>$vs){
		if(count($vs)==1) $anno[$key] = $vs[0]['value'];
	}
	return $anno;
}
function fs_src_tree($phpfile){
	$src = file_get_contents($phpfile);
	require_once $phpfile;
		preg_match_all('/<\?php\s*\/\*+\s*(?P<comment>.*?)\*\/\s*/sm', $src, $fdef);
	$comment = $fdef['comment'][0];
		preg_match_all('/^(abstract)*\s*(class|trait)\s+(?P<cls>[\w\d]+)\s*/mi', $src, $ma);
	$classes = [];
	if(!empty($ma['cls'])){
		foreach ($ma['cls'] as $cls){
			$classes[$cls] =[];
			$cr = new ReflectionClass($cls);
			$classes[$cls]['name'] = $cls;
						$parent = $cr->getParentClass();
			if($parent) $classes[$cls]['parent']=$parent->getName();
						$classes[$cls]['interfaces']=$cr->getInterfaceNames();
						$classes[$cls]['abstract']=$cr->isAbstract();
						$classes[$cls]['trait']=$cr->isTrait();
						$comm = $cr->getDocComment();
			if($comm==$comment) $comment='';
			$classes[$cls]['annotations']=fs_annotations($comm);
						$methods = $cr->getMethods();
			foreach ($methods as $mr){
				$args = array_map(function($e){return $e->name;}, $mr->getParameters());
				$anno = fs_annotations($mr->getDocComment());
				$classes[$cls]['methods'][$mr->getName()] = [
				'name'	=> $mr->getName(),
				'classname'=>$cls,
				'annotations'=>$anno, 'params'=>$args,
				'abstract' => $mr->isAbstract(),
				'constructor' => $mr->isConstructor(),
				'destructor' => $mr->isDestructor(),
				'final' => $mr->isFinal(),
				'visibility' => $mr->isPrivate()?'private':($mr->isProtected()?'protected':'public'),
				'static' => $mr->isStatic()
				];
			}
						$props = $cr->getProperties();
			foreach ($props as $pr){
				$classes[$cls]['properties'][$pr->getName()] = [
				'visibility' => $pr->isPrivate()?'private':($pr->isProtected()?'protected':'public'),
				'static' => $pr->isStatic()
				];
			}
		}
	}
		preg_match_all('/^function\s+(?P<func>[\w\d_]+)\s*\(/mi', $src, $ma);
	$funcs = [];
	if(!empty($ma['func'])){
		foreach ($ma['func'] as $fn){
			$ref = new ReflectionFunction($fn);
			$args = array_map(function($e){return $e->name;}, $ref->getParameters());
			$comm = $ref->getDocComment();
			if($comm==$comment) $comment='';
			$anno = fs_annotations($comm);
			$funcs[$fn] = ['annotations'=>$anno, 'params'=>$args, 'name'=>$fn];
		}
	}
	return ['annotations' => empty($comment)?[]:fs_annotations($comment),'functions' => $funcs,'classes' => $classes];
}

class Session {
	static function start(){
		session_start();
		$ua = $_SERVER['HTTP_USER_AGENT'];
		$time = time();
		if(!isset($_COOKIE['sid'])){
			setcookie('sid', md5($_SERVER['REMOTE_ADDR'].'|'.$ua), $time+86400*30, '/');
			$_SESSION['IP'] = $_SERVER['REMOTE_ADDR'];
			$_SESSION['UA'] = $ua;
			$_SESSION['ISSUED_HOST'] = $_SERVER['SERVER_NAME'];
			$_SESSION['ISSUED_AT'] = $time;
			$_SESSION['CSRF_NOUNCE'] = md5(uniqid(rand(), TRUE));			setcookie('sidsecr', sha1($_SESSION['CSRF_NOUNCE']), $time+86400*30, '/');
		}else{
			if($_COOKIE['sid']!=md5($_SESSION['IP'].'|'.$_SESSION['UA'])
				|| $time-(isset($_SESSION['ISSUED_AT'])?$_SESSION['ISSUED_AT']:0)>=Consts::$session_lifetime){
								self::clear();
				return self::start();
			}else if($_COOKIE['sidsecr']!=sha1($_SESSION['CSRF_NOUNCE'])){
				self::clear();
				return error(400); 			}
		}
	}
	private static function clear(){
		setcookie('sid', '', 1);
		setcookie('sidsecr', '', 1);
		unset($_COOKIE['sid']);
		unset($_COOKIE['sidsecr']);
		session_unset();
		session_destroy();
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
function mc_get($key, $nullHandler=null){
	$conn = mc_conn();
	$k = APP_NAME."::".$key;
	if($conn){
		$v = $conn->get($k);
		if(!$v && isset($nullHandler)){
			$r = $nullHandler($key);
			if(isset($r)&&$r!=false){
				mc_set($key, $r);
			}
			return $r;
		}
		return $v;
	}
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

function db_conn($opts=null, $pdoOpts=null){
	$db = REQ::getDB();
	if(!isset($db)){
		$db = pdo_conn($opts,$pdoOpts);
		REQ::setDB($db);
	}
	return $db;
}
function pdo_conn($opts=null, $pdoOpts=null){
	$opts = $opts ? $opts: [
		'engine'=>Consts::$db_engine,
		'host'	=>Consts::$db_host,
		'port'	=>Consts::$db_port,
		'db'	=>Consts::$db_name,
		'user'	=>Consts::$db_user,
		'pass'	=>Consts::$db_pass,
	];
	if (ini_get('mysqlnd_ms.enable')) {
		$host = APP_NAME.'_'.Consts::$mode;
		$conn_str = $opts['engine'].':host='.$host.';dbname='.$opts['db'].';charset=utf8';
	} else {
		$conn_str = $opts['engine'].':host='.$opts['host'].';port='.$opts['port'].';dbname='.$opts['db'].';charset=utf8';
	}
	$pdoOpts = $pdoOpts ? $pdoOpts :[PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,PDO::ATTR_PERSISTENT => true];
	return new PDO($conn_str,$opts['user'],$opts['pass'],$pdoOpts);
}
function pdo_query($pdo, $sql, $datas=[], $pdoOpt=null) {
	if(!$pdo || empty($sql))return false;
	error_log($sql);
	error_log(json_encode($datas));	
	if($pdoOpt==null)$pdoOpt=PDO::FETCH_ASSOC;
	$isQeury = str_starts(strtolower(trim($sql)), 'select');
	$statement = $pdo->prepare($sql);
	if ($statement->execute ($datas) == FALSE) {
				error_log("DB ERR:".json_encode($datas));	
		return false;
	}
	return $isQeury? $statement->fetchAll($pdoOpt):true;
}
function pdo_count($pdo, $sql, $datas=[], $col=0){
	if(!$pdo || empty($sql))return false;
	$statement = $pdo->prepare($sql);
	if ($statement->execute ($datas) == FALSE) {
		return false;
	}
	$res =  $statement->fetchColumn();
	return intval($res);
}
function pdo_import($pdo, $table, $datas, $regName='regAt', $updName='updAt'){
	if(!isset($pdo) ||!isset($table) || count($datas)==0)
		return false;
	list($table,$schemaname) = explode('@',$table);
	if(empty($schemaname)) $schemaname = $table;
	$schema = db_schema($schemaname)['schema'];
	$cols = [];
	foreach ($datas as $d){
		$cols = array_unique(array_merge($cols,array_keys($d)));
	}
	$cls = $cols;$cols=[];$schema_cols =array_keys($schema);
	foreach($cls as $c){
		if(in_array($c, $schema_cols)){
			$cols[]=$c;
		}
	}
	$hasRegStamp = !empty($regName) && array_key_exists($regName,$schema);
	if($hasRegStamp && !in_array($regName, $cols)) $cols[] = $regName;
	$hasTimestamp = !empty($updName) && array_key_exists($updName,$schema);
	if($hasTimestamp && !in_array($updName, $cols)) $cols[] = $updName;
	$sql = 'INSERT IGNORE INTO '.$table.' (`'.join('`,`', $cols).'`) VALUES ';
	$time = time();
	foreach ($datas as $d){
		if($hasRegStamp && empty($d[$regName])){$d[$regName]=$time;}
		if($hasTimestamp && empty($d[$updName])){$d[$updName]=$time;}
		$vals = [];
		foreach ($cols as $c){
			$v = array_key_exists($c, $d) ? $d[$c] : null;
			$vals[]=db_v($v, $schema[$c]);
		}
		$sql.=' ('.join(',', $vals).'), ';
	}
	$sql = substr($sql, 0, strlen($sql)-2);
	$pdo->setAttribute(PDO::ATTR_TIMEOUT, 1000);
	try{
		pdo_query($pdo, $sql);
		$pdo->setAttribute(PDO::ATTR_TIMEOUT, 10);
	}catch(Exception $e){
		$pdo->setAttribute(PDO::ATTR_TIMEOUT, 10);
		$file = '/tmp/'.$table.'_imp.sql';
		file_put_contents($file,$sql);
		return false;
	}
}
function pdo_save($pdo, $table, $data, $returnId=false, $bson=false){
	if(!isset($pdo) || !isset($table) || !is_hash($data) || empty($data))return false;
	$regName = Consts::$schema_reg;
	$updName = Consts::$schema_upd;
	list($table,$schemaname) = explode('@',$table);
	if(empty($schemaname)) $schemaname = $table;
	$schema_def = db_schema($schemaname);
	$schema = $schema_def['schema'];
	$pk = $schema_def['general']['pk'];
	$pks=[];$qo=null;$isUpdate=false;
	if (str_has($pk, '|')||str_has($pk, '+')||str_has($pk, ',')){
		$pks = preg_split('/[\|\+,]/', $pk);
		$qo = [];
		foreach ($pks as $p){
			if(empty($data[$p])){
				return false;
			}else 
				$qo[$p] = $data[$p];
		}
		try{
			$ext = pdo_find($pdo, $table.'@'.$schemaname, $qo);
		}catch(Exception $e){
			error_log($e->getMessage().'\n');
		}
		$isUpdate = !empty($ext);
	} else{
		$id = isset($data[$pk]) ? $data[$pk] : null;
		$isUpdate = isset($id) && pdo_exists($pdo, $table.'@'.$schemaname, $id);
	}
	$sql = '';
	if(array_key_exists($updName,$schema) && !isset($data[$updName])){
		$data[$updName] = ms();
	}
	$qdatas = [];
	if ($isUpdate){
		if($id)cache_del($table.'_'.$id);
		foreach ($data as $col => $val){
			if(str_ends($col,'+')) {
				$opr = substr($col,-1);
				$col = substr($col,0,-1); 
			}
			if($col==$pk || in_array($col, $pks) || !isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ',';
			$colStmt .= $opr? '`'.$col.'`=`'.$col.'` + :'.$col.' ' : '`'.$col.'`=:'.$col.' ';
			$qdatas[$col]= is_array($val)?json_encode($val):$val;		}
		if(empty($pks)){
			$sql = 'UPDATE `'.$table.'` SET '.$colStmt.' WHERE `'.$pk.'`='.db_v($id).';';
		}else{
			$table = $table.'@'.$schemaname;
            list($colStr,$optStr,$qrdatas) = db_make_query($table, $qo);
			foreach ($qrdatas as $qk=>$qv){
				$qdatas[$qk]= $qv;
			}
			$sql ='UPDATE `'.$table.'` SET '.$colStmt.' '.$optStr; 
		}
	}else{								
		if(array_key_exists($regName,$schema) && !isset($data[$regName]))
			$data[$regName] = ms();
		foreach ($data as $col => $val){
			if(str_ends($col,'+')) {
				$opr = substr($col,-1);
				$col = substr($col,0,-1);
			}
			if(!isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ',';
			if(!empty($valStmt))$valStmt .= ',';
			$colStmt .= '`'.$col.'`';
			$valStmt .= $opr? '`'.$col.'` + :'.$col.' ' : ':'.$col;
			$qdatas[$col] = is_array($val)?json_encode($val):$val ;		}
		$sql = 'INSERT '.$ignore.' INTO `'.$table.'` ('.$colStmt.') VALUES('.$valStmt.')';
					}
	try {
		if($returnId==true && !$isUpdate) {
			if(!$pdo->inTransaction()) {
				$res = pdo_trans($pdo,[$sql, 'SELECT LAST_INSERT_ID() as \'last_id\''],[$qdatas]);
				$data['id'] = $res[0]['last_id'];
			}else{
				pdo_query($pdo, $sql,$qdatas);
				$res = pdo_query($pdo, 'SELECT LAST_INSERT_ID() as \'last_id\'', []);
				$data['id'] = $res[0]['last_id'];
			}
		}else{
			pdo_query($pdo, $sql,$qdatas);
		}
		return $data;
	} catch (Exception $e) {
		error_log('ERROR '.$e->getMessage());
		error_log($sql);
		return false;
	}
}
function pdo_find($pdo, $table, $opts=[], $withCount=false, $pdoOpt=null){
	if(!$pdo || !$table)return false;
	list($colStr, $optStr,$datas,$conns) = db_make_query($table, $opts);
	$sql = 'SELECT '.$colStr.' FROM '.$table.$optStr;
	$res = pdo_query($pdo, $sql, $datas, $pdoOpt);
	if(!empty($conns) && !empty($res)){
		$ds = [];
		$extras = [];
		foreach ($conns as $conn => $def) {
			$col = $def['column'];
			if(!isset($ds[$col]))
				$ds[$col] = array_map(function($e) use($col){return $e[$col];}, $res);
			$condition = empty($def['query'])?[]:$def['query'];
			$condition['fields'] = $def['fields'];
			$tc = $def['target_column'];
			if(count($ds[$col])>1){
				$condition[$tc.'@in']=join(',',$ds[$col]);
			}else
				$condition[$tc]=$ds[$col][0];
			$re = pdo_find($pdo,$def['table'],$condition, false, $pdoOpt);
			$extras[$conn]=[];
			foreach ($re as $r) {
				$k = $r[$tc];
				if(!isset($extras[$conn][$k])) 
					$extras[$conn][$k]=[];	
				$extras[$conn][$k][] = $r;
			}
					}
		foreach ($res as &$r) {
			foreach ($conns as $conn => $def) {
				$tc = $def['target_column'];
				$r[$conn] = $extras[$conn][''.$r[$def['column']]];
				if($def['fields']!='*' && !in_array($tc, $def['fields']))
					unset($r[$conn][$tc]);
			}
		}
	}
	if($withCount){
				$sql = 'SELECT count(*) FROM '.$table.preg_replace(['/ORDER\s+BY.*/i','/LIMIT\s.*/i'], '',$optStr);
		$cnt = pdo_count($pdo,$sql, $datas, $opts['useCache']);
		$key_cnt = property_exists('Consts', 'schema_total')? Consts::$schema_total:'count';
		$key_res = property_exists('Consts', 'schema_result')? Consts::$schema_result:'result';
		return [$key_cnt=>$cnt,$key_res=>$res];
	}else{
		return $res;
	}
}
function pdo_exists($pdo, $table, $id){
	if(!isset($pdo) ||!isset($table) || !isset($id))
		return false;
	list($table,$schemaname) = explode('@',$table);
	if(empty($schemaname)) $schemaname = $table;
	$pk = db_schema($schemaname)['general']['pk'];
	$entity =pdo_count($pdo, "select count(*) from $table where `$pk`=:$pk",[$pk=>$id]);
	return $entity>0;
}
function pdo_trans($pdo,$querys,$datas,$pdoOpt){
	if(!isset($pdo)||!isset($querys))
		return false;
	if($pdoOpt==null)$pdoOpt=PDO::FETCH_ASSOC;
	$mod = $pdo->getAttribute(PDO::ATTR_ERRMODE);
	$pdo->setAttribute(PDO::ATTR_AUTOCOMMIT, 0 );
	$pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	$cnt = 0;
	$res = true;
	try{
		$pdo->beginTransaction();
		if(is_callable($querys)){
			$cnt = $querys($pdo);
		}else if(is_array($querys)){
			$i=0;
			foreach($querys as $q){
				$i++;
				if($q==='@rollback'){
					$pdo->rollBack();$cnt--;
				}else{
					$statement = $pdo->prepare($q);
					if(!$statement){
						error_log("PDO TRANS Failed : ".$pdo->errorInfo());
						continue;
					}
					$data = isset($datas[$i-1])?$datas[$i-1]:[];
					if ($statement->execute($data) == false) {
						error_log("PDO TRANS Failed : ".$pdo->errorInfo());
						continue;
					}
					if(str_starts(strtolower($q), 'select')){
						$res = $statement->fetchAll($pdoOpt);					
					}
					$cnt++;
				}
			}
		}
		if($cnt>0)
			$pdo->commit();
		else
			$pdo->rollBack();
	}catch(Exception $e){
		error_log('DB Transaction ERR:'.$e->getMessage());
		$pdo->rollBack();
        $res = false;
	}
	$pdo->setAttribute(PDO::ATTR_AUTOCOMMIT, 1);
	$pdo->setAttribute(PDO::ATTR_ERRMODE, $mod);
	return $res;
}
function db_query($sql, $datas=[], $useCache = false, $pdoOpt=null) {
	try {
		$key = $sql;
		$isQeury = str_starts(strtolower($sql), 'select');
		if ($useCache && $isQeury) {
			$key = $sql.'-'.json_encode($datas);
			$value = cache_get($key);
			if (isset ( $value ) && $value != false)
				return $value;
		}
		$db = db_conn();
		$res = pdo_query($db, $sql, $datas, $pdoOpt);
		if ($useCache && $isQeury && $res != null) {
			cache_set($key, $res);
		}
		return $res;
	} catch ( PDOException $e ) {
		error_log ('DB ERR :'. $e->getMessage() );
		error_log ('DB ERR SQL:'. $sql );
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
		$res = pdo_count($db, $sql, $datas);
		if($useCache && $res){
			cache_set($sql, $res);
		}
		return $res;
	} catch (PDOException $e) {
		error_log($sql);
		return -1;
	}
}
function db_attr($attr, $val){
	$db = db_conn();
	$db->setAttribute($attr, $val);
}
function db_find($table, $opts=[], $withCount=false, $pdoOpt=null){
	return pdo_find(db_conn(), $table, $opts, $withCount, $pdoOpt);
}
function db_find1st($table, $opts=[], $pdoOpt=null){
	$opts['limit']=1;
	$res = db_find($table,$opts,false,$pdoOpt);
	return isset($res)&&$res!=false ? $res[0]:false;
}
function db_import($table, $datas){
	return pdo_import(db_conn(), $table, $datas, Consts::$schema_reg, Consts::$schema_upd);
}
function db_make_query(&$table, $opts=[], $omit=[], $colPrefix=false){
	db_init_filters();
	if(!isset($table))return false;
		list($table,$schemaname) = explode('@',$table);
	if(empty($schemaname)) $schemaname = $table;
	$colStr = '*'; 	$schemas = db_schema();
	$schemaDef = $schemas[$schemaname];
	$pk = $schemaDef['general']['pk'];
	$schema = $schemaDef['schema'];
	$connect = $schemaDef['connect'];
	$connNames = !empty($connect) ?array_keys($connect):[];
	if($colPrefix)$colPrefix.=".";
	$data = [];
	$conns = [];
		if(is_hash($opts) && !empty($opts['fields']) && 
		(preg_match('/[\{\}\.]+/',$opts['fields'])) || (!empty($connNames)&&preg_match('/\b('.join('|',$connNames).')\b/', $opts['fields'])) ){ 				preg_match_all('/\b(?P<tbl>[\w\d_]+)\{(?P<cols>[^\}]+)\}/', $opts['fields'], $ma);
		if(!empty($ma['tbl'])){
			$i=0;
			foreach ($ma['tbl'] as $tbl) {
				if(!isset($connect[$tbl]))continue;				if(!isset($conns[$tbl])) $conns[$tbl] = ['fields'=>[$connect[$tbl]['target_column']]]+$connect[$tbl];
				$conns[$tbl]['fields'] = array_merge($conns[$tbl]['fields'],explode(',',$ma['cols'][$i++])) ;
			}
			$opts['fields'] = preg_replace(['/\b(?P<tbl>[\w\d_]+)\{(?P<cols>[^\}]+)\}/','/^,/','/,$/'], '', $opts['fields']);
		}
				$cols =  explode(',',$opts['fields']);
		$ncols = [];
		foreach ($cols as $f) {
			$f = trim($f);
			if(in_array($f, $connNames)){
				$conns[$f] = ['fields'=>'*']+$connect[$f];
			}else if(str_has($f, '.')){
				list($tbl, $col) = explode('.', $f);
				if(in_array($tbl, $connNames)){
					if(!isset($conns[$tbl])) $conns[$tbl] = ['fields'=>[$connect[$tbl]['target_column']]]+$connect[$tbl];
					$conns[$tbl]['fields'][] = $col;	
				}
			}else{
				if($f=='*' || array_key_exists($f, $schema))
					$ncols[]=$f;
			}
		}
		$connFields = array_keys($conns);
		foreach ($connFields as $cf) {
			if($opts['fields']!='*' && !preg_match('/\b'.$connect[$cf]['target_column'].'\b/i',!$opts['fields'])){
				$ncols []= $connect[$cf]['column'];
			}	
		}
		$colStr = '`'.join('`,`',$ncols).'`';
	}else{
		if(!empty($opts['fields']) && $opts['fields']!='*'){
			$colStr = is_string($opts['fields'])? explode(',',preg_replace('/[`\s]/','',$opts['fields'])):$opts['fields'];
			$colStr = array_filter($colStr, function($e) use($schema, $omit){return array_key_exists($e, $schema) && !in_array($e,$omit);});
			$colStr = $colPrefix? $colPrefix.'`'.join('`,'.$colPrefix.'`', $colStr).'`':'`'.join('`,`', $colStr).'`';
		}else if($colStr=='*' && !empty($schemaDef['general']['fields'])){
			$colStr = $colPrefix? $colPrefix.'`'.str_replace(',', '`,'.$colPrefix.'`', $schemaDef['general']['fields']).'`':'`'.str_replace(',', '`,`', $schemaDef['general']['fields']).'`';
		}
	}
	if(is_hash($opts)){
		$optStr = [];
				if(array_key_exists('@id', $opts) && array_key_exists('id', $opts)) {
			unset($opts['@id']);unset($opts['limit']);
		}
		foreach ($opts as $k => $v){
			preg_match_all('/^(?<tbl>[\w\d_]+)\./i',$k,$ma);
			if(!empty($ma['tbl'])){				$tbl = $ma['tbl'][0];
				$col = substr($k, strlen($tbl)+1);
				if(empty($conns[$tbl])) continue;
				if(!isset($conns[$tbl]['query']))
					$conns[$tbl]['query'] = [];
				$conns[$tbl]['query'][$col] = $v;
			}else{
				if($k=='@id')$k=$pk;
				list($k,$cmd) = explode('@',$k);
				$keys = array_filter(preg_split('/\|/',$k), function($k) use($schema, $omit){
					return array_key_exists($k, $schema) && !in_array($k,$omit);
				});
				if(!empty($keys)){
					$cmd = !isset($cmd)||$cmd=='' ? '=':$cmd;
					$cmd = strpbrk($cmd, 'begilmnqt') !==false? Consts::$query_filter_names[$cmd]:$cmd;
					$func = Consts::$db_query_filters[$cmd];
					$vStr = $func(join('|', $keys), $v, $data);
					if($vStr) $optStr []= $vStr;
				}	
			}
		}
		$optStr =  empty($optStr) ? '': ' WHERE '.join(' AND ', $optStr);
		if(!in_array('order',$omit) && !empty($opts['order']))
			$optStr .= ' ORDER BY '.$opts['order'];
		if(!in_array('limit',$omit) && !empty($opts['limit']))
			$optStr .= ' LIMIT '.$opts['limit'];
	}else {
		$optStr = !empty($opts)? ' WHERE '. $opts : '';
	}
	return [$colStr,$optStr,$data,$conns];
}
function db_exists($table, $id){
	return pdo_exists(db_conn(), $table, $id);
}
function db_delete($table, $opts){
	if(empty($opts))return false;
	list($cs,$optStr,$data) = db_make_query($table, $opts,['order','limit','fields']);
	$sql = 'DELETE FROM '.$table.' '.$optStr;
	return db_query($sql,$data);
}
function db_update($table, $data, $opts=[]){
	if(!isset($table) || empty($data) || !is_hash($data))
		return false;
	$vStrs = [];
	list($table,$schemaname) = explode('@',$table);
	if(empty($schemaname)) $schemaname = $table;
	$schema = db_schema($schemaname)['schema'];
	foreach($data as $k=>$v){
		$vStrs[]='`'.$k.'`='.db_v($v, $schema[$k]);
	} 
	$vStrs = join(',',$vStrs);
	list($cs,$optStr,$data) = db_make_query($table, $opts,['order','limit','fields']);
	$sql = 'UPDATE '.$table.' SET '.$vStrs.' '.$optStr;
	return db_query($sql,$data);
}
function db_migrate($schemaName, $tableName=null){
	$isCLI = (php_sapi_name() === 'cli');
	$dbn = Consts::$db_name;
	if(empty($tableName))$tableName=$schemaName;
	$sql = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='$dbn' AND table_name='$tableName';";
	$res = db_count($sql,[], false);
	if ($res<=0 ){		$schema_def = db_schema($schemaName);
		$schema = $schema_def['schema'];
		$pk = $schema_def['general']['pk'];
				$engine = $schema_def['general']['engine'];
		if(empty($engine)) $engine='InnoDB';
		$colStmt = '';
		foreach ($schema as $col => $type){
			$colStmt .= '`'.$col.'` '.$type.', ';
		}
		$incStmt = '';
		$auto_increment = $schema_def['general']['auto_increment'];
		if($auto_increment) $incStmt .= 'auto_increment='.$auto_increment;
		$sql = '';
		if (str_has($pk, '|')||str_has($pk, '+')||str_has($pk, ',')){
			$parts = preg_split('/[\|\+,]/', $pk);
			$pkName = join('_',$parts);
			$keys = '`'.join('`,`',$parts).'`';
			$sql = "CREATE TABLE `$dbn`.`$tableName` ( $colStmt CONSTRAINT $pkName PRIMARY KEY ($keys)) ENGINE=$engine $incStmt DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;";
		}else{
			$sql = "CREATE TABLE `$dbn`.`$tableName` ( $colStmt PRIMARY KEY (`$pk`)) ENGINE=$engine $incStmt DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;";
		}
		$res = db_query($sql);
				$index = $schema_def['index'];
		if(!empty($index)){
			foreach ($index as $cols => $desc){
				$name = 'idx_'.preg_replace('/[\+\.\|]/', '_',$cols);
				$cols = preg_replace('/[\+\.\|]/', '`,`',$cols);
				$desc = $desc=='unique'?'UNIQUE ':' ';
				$indextype = ($engine == 'MEMORY' && strtolower($desc)=='hash') ? ' USING HASH' : ' ';
				$sql = "CREATE ${desc}INDEX $name ON `$tableName` (`$cols`) $indextype";
				if($isCLI) echo $sql.'\n';
				db_query($sql);
			}
		}
	}
	if($isCLI)
		echo 'Created '.$tableName.'</br>\n';
	else return true;
}
function db_save($table, $data, $returnId=false, $bson=false){
	return pdo_save(db_conn(), $table, $data, $returnId);
}
function db_schema($schemaName=null){
	$schemas = cache_get('DB_SCHEMAS', function($key){ 
		$dir = APP_DIR.__SLASH__.'conf'.__SLASH__.'schemas';
		$files = glob($dir."/*.ini");
		$schemas = [];
		$conns = [];
		foreach ($files as $f) {
			$n = str_replace([$dir.'/','.ini'], '', $f);
			$s = parse_ini_file($f, true);
			if(!empty($s['connect'])){
				$conns = [];
				foreach ($s['connect'] as $ck => $cv) {
					preg_match_all('/(?P<col>[\w\d_]+)\s*=\s*(?P<tbl>[\w\d_]+)\.(?P<tarCol>[\w\d_]+)/', $cv, $mc);
					if(!empty($mc['col'])&&!empty($mc['tbl'])&&!empty($mc['tarCol'])){
						$conns[$ck] = [
							'column' 		=> $mc['col'][0],
							'table' 		=> $mc['tbl'][0],
							'target_column' => $mc['tarCol'][0],
						];
					}else{
						throw "DB ERR: wrong format in $f.ini [connect], should be [MAPPING_NAME = 'COLUMN_NAME = TABLE_NAME.COLUMN_NAME']";
					}
				}
				$s['connect'] = $conns;
			}
			$schemas[$n] = $s;
		}
		return $schemas;
	},false);
	return isset($schemaName)? $schemas[$schemaName]:$schemas;
}
function db_trans($querys,$datas){
	return pdo_trans(db_conn(), $querys,$datas);
}
function bson_enc($arr){
	$str = json_encode($arr);
	$str = str_replace('\\', '', $str);
	return str2hex($str);
}
function bson_dec($bson){
	if(isset($bson)){
		$json = hex2str($bson);
		return json_decode($json,true);
	}
	return false;
}
function db_v($v, $typeDef='', $bsonText=false){
	if(!isset($v))
		return 'NULL';
	if(is_bool($v))
		return $v ? 1 : 0;
	if (is_array($v)){
		return $bsonText&&(isset($typeDef)&&preg_match('/text/i', $typeDef))? '\''.bson_enc($v).'\''
				: '\''.mysql_escape_string(json_encode($v)).'\'';
	}
	if(is_string($v)){
		if(preg_match('/bigint/i', $typeDef) && str_has($v, '-'))
			return strtotime($v);
		if(preg_match('/(int|byte)/i', $typeDef))
			return intval($v);
		return "'".mysql_escape_string($v)."'";
	} 
	return $v;
}
function db_make_filters($k,$k_operator, $v, $v_operator, &$o, $func_make) {
	$keys = is_array($k)? $k : preg_split('/\|/',$k);
	$values = is_array($v)? $v : preg_split('/\|/',$v);
	$conditions =[]; $idx=0;
	foreach($keys as $_k) {
		$sub_cond = [];
		foreach($values as $_v) {
			$sql = $func_make($_k, $_v, $o, $idx);
			if($sql) $sub_cond[] = $sql;
			$idx++;
		}
		if(!empty($sub_cond))
			$conditions[] = count($values)>1 ? '('. join(' '.$v_operator.' ', $sub_cond) .')' : join(' '.$v_operator.' ', $sub_cond);
	}
	if(count($conditions) <=0 ) return false;
	return count($conditions)>1 ? '('. join(' '.$k_operator.' ', $conditions) .')' : join(' '.$k_operator.' ', $conditions);	
}
function db_init_filters(){
	if(empty(Consts::$db_query_filters))
		Consts::$db_query_filters = [
		'=' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'or', $v, 'or', $o, function($k,$v, &$o, $idx){
				if($v==='NULL') return '`'.$k.'` IS NULL'; else { $_k = $k.'_'.$idx; $o[$_k]=$v;return '`'.$k.'`=:'.$_k; }
			});
		},
		'!' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				if($v==='NULL') return '`'.$k.'` IS NOT NULL'; else { $_k = $k.'_'.$idx;$o[$_k]=$v;return '`'.$k.'`!=:'.$_k.''; }
			});
		},
		'<' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				$_k = $k.'_'.$idx; $o[$_k]=$v;return '`'.$k.'`<:'.$_k.'';
			});
		},
		'>' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				$_k = $k.'_'.$idx; $o[$_k]=$v;return '`'.$k.'`>:'.$_k.'';
			});
		},
		'<=' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				$_k = $k.'_'.$idx; $o[$_k]=$v;return '`'.$k.'`<=:'.$_k.'';
			});
		},
		'>=' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				$_k = $k.'_'.$idx; $o[$_k]=$v;return '`'.$k.'`>=:'.$_k.'';
			});
		},
		'[]' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)==0)return false;$vs=array_map(function($e){return db_v($e);},$v);return '`'.$k.'` IN ('.join(',',$vs).')';
			});
		},
		'![]' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return '`'.$k.'` NOT IN ('.join(',',$vs).')';
			});
		},
		'()' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)!=2)return false; return '(`'.$k.'` BETWEEN '.min($v[0],$v[1]).' AND '.max($v[0],$v[1]).')';
			});
		},
		'!()' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)!=2)return false; return '(`'.$k.'` NOT BETWEEN '.min($v[0],$v[1]).' AND '.max($v[0],$v[1]).')';
			});
		},
		'?'  	=> function($k,$v,&$o){
			return db_make_filters($k, 'or', $v, 'or', $o, function($k,$v, &$o, $idx){
				if(!str_has($v,'%'))$v='%'.$v.'%';return '`'.$k.'` LIKE \''.preg_replace('/[\+\s]+/','%',$v).'\'';
			});
		},
		'!?'  	=> function($k,$v,&$o){
			return db_make_filters($k, 'and', $v, 'and', $o, function($k,$v, &$o, $idx){
				if(!str_has($v,'%'))$v='%'.$v.'%';return '`'.$k.'` NOT LIKE \''.preg_replace('/[\+\s]+/','%',$v).'\'';
			});
		},
		'~' 	=> function($k,$v,&$o){
			return db_make_filters($k, 'or', $v, 'or', $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return '`'.$k.'` '.$op.' \''.mysql_escape_string(preg_replace('/^\/|\/$/','',$v)).'\'';
			});
		},
		'!~'	=> function($k,$v,&$o){
			return db_make_filters($k, 'or', $v, 'or', $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return '`'.$k.'` NOT '.$op.' \''.mysql_escape_string(preg_replace('/^\/|\/$/','',$v)).'\'';
			});
		},
		'~~'	=> function($k,$v,&$o){
			return db_make_filters($k, 'or', $v, 'or', $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return 'LOWER(`'.$k.'`) '.$op.' \''.mysql_escape_string(preg_replace('/^\/|\/$/','',$v)).'\'';
			});
		},
		'!~~'	=> function($k,$v,&$o){
			return db_make_filters($k, 'or', $v, 'or', $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return 'LOWER(`'.$k.'`) NOT '.$op.' \''.mysql_escape_string(preg_replace('/^\/|\/$/','',$v)).'\'';
			});
		},
			];
}
class Render {
	static $path;
	private static $var_prefix = 'LBR_';
	private static $output_path;
	private static $ext = '.html';
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
	function render($file,$data=[],$layout=null,$renderOnly=false){
		$req = REQ::getInstance();
		$template = isset($layout)? $layout : $this->layout;
		$ns = $req->getNamespace();
		$ns = empty($ns)?"":$ns."-";
		$key = 'template-'. REQ::getTemplateType()."-".$ns.$template;
		$wrapper_code = cache_get($key, function($f){
			$req = REQ::getInstance();
			$ns = $req->getNamespace();
			$ns = empty($ns)?"":$ns."-";
			$key_prefix = 'template-'.REQ::getTemplateType()."-".$ns;
			return file_get_contents(Render::$path.str_replace($key_prefix,'',$f));
		},false);
				if(!empty($data))
		foreach ($data as $k=>$v)
			$this->data[$k] = $v;
		$this->data['__render'] = $this;
		$req = REQ::getInstance();
		if(!$this->data['__controller']) $this->data['__controller'] = $req->getController();
		if(!$this->data['__namespace']) $this->data['__namespace'] = $req->getNamespace();
		if(!$this->data['__action']) $this->data['__action'] = $req->getAction();
		if(!$this->data['__params']) $this->data['__params'] = $req->params;
		$_REQUEST[self::$var_prefix."TMP_DATA"] = $this->data;
		extract($this->data, EXTR_PREFIX_ALL, self::$var_prefix);
		$r = $this->render_file($file,$wrapper_code);
		$output = null;
		if($r){
			if($renderOnly){
				ob_start(); 
				include($r);
				$output = ob_get_contents();
								ob_end_clean();
							}else{
				include($r);
			}
		}
		unset($this->data);
		unset($data);
		if($output)
			return $output;
	}
	function render_file($file,$template_code){
		$prefix  = 'template-' . REQ::getTemplateType() . '-';
		if(!empty($this->data['__namespace']))
			$prefix .= $this->data['__namespace']."-";
		$filepath = self::$path.$file;
		if(!file_exists($filepath)) return false;
		$outpath = self::$output_path. $prefix .str_replace(self::$ext,'.php',$file);
		if(!file_exists($outpath)
						||Consts::$mode=="Developing"){
			$code = $this->compile($filepath,$template_code);
			if(isset($code) && $code!=""){
				file_put_contents($outpath,$code);
				unset($code);
			}
		}
		return $outpath;
	}
	function compile($file,$wrapper){
		list($before, $after) = $wrapper?explode('__CONTENTS__', $wrapper):["",""];
		$src = $before.file_get_contents($file).$after;
		$rows = preg_split('/(\{[^\{^\}]*\})/', $src, -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY ) ;
		$phpcode = '';
		$indent = 0;
		$ignore = false;
				$delegate_methods = get_class_methods('RenderDelegate');
		$custom_tags = [];
		foreach ($delegate_methods as $m) 
			if(str_starts($m,'tag_'))
				$custom_tags []= preg_replace('/^tag_/','',$m);
				$tags_regexp = (!empty($custom_tags)) ?
			'(%|=|if|elseif|else|break|ignore|for|var|include|'.join('|',$custom_tags).')':
			'(%|=|if|elseif|else|break|ignore|for|var|include)';
		while($code = array_shift($rows)){
			$matched = false;
			preg_match_all('/\{(?P<close>\/*)(?P<tag>'.$tags_regexp.'{0,1})\s*(?P<val>.*)\}/', $code, $matches);
			if(empty($matches[0])){
				$phpcode .= $code;
			}else{
				list($close, $tag, $val) =  [$matches['close'][0]=="/"?true:false, $matches['tag'][0], trim($matches['val'][0])];
				if($tag=='' || $tag=='=')$tag='echo';
				if($tag=='%')$tag='text';
				$val = $tag=="text"?$val: preg_replace('/\.([a-zA-Z0-9_]+)/', "['$1']",$val);
				if(!preg_match('/\$(_GET|_POST|_REQUEST|_SESSION)/', $val))
					$val = preg_replace('/\$/','$'.self::$var_prefix."_",$val);
				if($close){
					if($tag=='if'||$tag=='for')$indent --;
					if($tag=='ignore'){
						$ignore = false;
					}else{
						$phpcode .= '<?php } ?>';
					}
				}else if($ignore){
					$phpcode .= $code;
				}else if(!empty($custom_tags)&&in_array($tag, $custom_tags)){
										$phpcode .= "<?php echo RenderDelegate::tag_{$tag}(".(empty($val)?'""':'"'.$val.'"').", \$_REQUEST['".self::$var_prefix."TMP_DATA']); ?>";
				}else{
					switch($tag){
						case 'for':
							$parts = preg_split('/\s*,\s*/',$val,-1,PREG_SPLIT_NO_EMPTY );
							$len = count($parts);
							$indent ++;
							switch($len){
								case 1:$phpcode .= '<?php foreach('.$parts[0]." as $".self::$var_prefix."_key=>$".self::$var_prefix."_value) { ?>";break;
								case 2:$phpcode .= '<?php foreach('.$parts[0]." as $".self::$var_prefix."_key=>".$parts[1].") { ?>";break;
								default :
									if((preg_match('/^\d+$/', $parts[1])) || (preg_match('/^\$/', $parts[1])) && (preg_match('/^\d+$/', $parts[2]))|| (preg_match('/^\$/', $parts[2]))){
										$phpcode .= '<?php for($'.$parts[0].'='.$parts[1].';$'.$parts[0].'<'.$parts[2].';$'.$parts[0].'++) { ?>';
									}else
										$phpcode .= '<?php foreach('.$parts[0].' as '.$parts[1].'=>'.$parts[2].') { ?>';break;
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
							$vstr = preg_split('/,+/', trim($val));
							if(count($vstr)>1){
								$vstr = array_map(function($e){return trim($e);}, $vstr);
								$phpcode .= '<?= T("'.join('","',$vstr).'"); ?>';break;
							}else
								$phpcode .= '<?= T("'.$val.'"); ?>';break;
						case 'var':
							$phpcode .= '<?php '.$val.'; ?>';break;
						case 'include':
							$phpcode .= '<?php $__render->include_template("'.preg_replace('/\'"/',"",$val).'"); ?>';break;
						case 'ignore':
							$ignore = true;
							break;
						default:
							break;
					}				}			}
		}
		return $phpcode;
	}
	function include_template($f){
		$r = $this->render_file($f.'.html');
		$output = '';
		if($r) {
			ob_start(); include($r);
			$output = ob_get_contents();
			ob_end_clean();
		};
		echo $output;
		flush();
	}
	static function paginate($page,$total,$opts=['perPage'=>20]){
		$pp = ($opts['perPage']>0)? $opts['perPage']: 20;
		$pi = $opts['items']?$opts['items']:9; 
		$ptt = ceil($total/$pp); $half=floor($pi/2);
		$pages = []; $begin = max(1,$page-$half); $end = min($ptt, $page+$half);
		$cursor = 0;
		for($i=$begin;$i<=$end;$i++){
			$pages[]=$i;
			if($i<$page) $cursor++;
		}
		if($begin>2) {
			$b = $begin;
			while(count($pages)<$pi-2 && $b>2){
				array_unshift($pages,--$b);$cursor++;
			}
			if(count($pages)>$pi-2)$pages =[1,0]+$pages;
			else{$pages = array_merge([1,0],$pages); $cursor+=2;}
		}
		if($begin==2) {array_unshift($pages, 1); $cursor++;}
		if($ptt-$end>2) {
			while(count($pages)>$pi-2){
				$p = array_pop($pages);
				if($p<$page)$cursor--;
			}
			if(count($pages)<$pi-2){
				$e = $end;
				while(count($pages)<$pi-2 && $e<$ptt-2){array_push($pages, ++$e);}
			}
			$pages = array_merge($pages,[0,$ptt]);
		}
		if($ptt-$end==2) $pages []= $ptt;
		return ['pages'=>$pages,'cursor'=>$cursor];
	}
}

abstract class Filter{
	private function __construct(){}
	abstract public function before(&$params, $authRequired);
	abstract public function after($params, $authRequired);
	public static function factory($type){
		if (include APP_DIR.__SLASH__.'filters'.__SLASH__. $type. '.inc') {
            $classname = strtoupper($type[0]).substr($type,1).'Filter';
            return new $classname;
        } else {
            throw new Exception('filter not found');
        }
	}
}

spl_autoload_register(function($class){
	if(!include_once $class.'.inc')
		include_once $class.'.php';
});
try{
	$cli_args = array_slice($argv, 1);
	$cli_cmd = array_shift($cli_args);
	if(php_sapi_name() == 'cli' || PHP_SAPI == 'cli'){
		$cli_cmd="cli_".$cli_cmd;
		if(function_exists($cli_cmd)){$cli_cmd();}
	}else{
		REQ::dispatch();
	}
}catch(Exception $e){
	error_log($e->getMessage());
	exit;
}
function cli_script(){
	global $cli_args;
	$f = array_shift($cli_args);
	if(!empty($f)){
		$pwd=dirname(__FILE__);
		$f = $pwd."/scripts/$f.php";
		include $f;
	}
	exit;
}
function cli_migrate(){
	try{
		$pwd=dirname(__FILE__);
		$schemas = glob($pwd."/conf/schemas/*.ini") ;
		foreach ($schemas as $file){
			echo $file."\n";
			$parts = explode("/",$file);
			$file = $parts[count($parts)-1];
			$parts = explode(".", $file);
			$schema = $parts[0];
			echo $schema."\n";
			db_migrate($schema);
		}
		echo "DONE\n";
	}catch(Exception $e){
		echo "FAILED\n";
	}
	exit;
}

?>
