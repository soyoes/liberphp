<?php

/**
 *	@file: cache.inc	
 *	@author: Soyoes 2011/04/27
 *	@uses: Interface of variable NoSQL
 *	@example: 
 *
 *	TODO change to memecached
 *
 *****************************************************************************/

class Cache {

	static $_singleton;
	
	private static function init(){
		if(!isset(self::$_singleton)){
			self::$_singleton = __Cache::factory();
		}
	}
	
	/**
	 * $nullHandler : function. if not $key exists, call this function with $key
	 * */
	static function get($key, $nullHandler=null){
		self::init();
		$res = self::$_singleton->get($key);
		if($res===false && $nullHandler!=null){
			try{
				$res = $nullHandler($key);
				self::set($key, $res);
			}catch(Exception $e){
				return false;
			}
		}
		return $res;
	}
	
	static function set($key, $value, $span=3600, $sync=false){
		self::init();
		try{
			self::$_singleton->set($key, $value, $span);
			if($sync===true){
				$v = is_array($value) ? json_encode($value):$value;
				self::call("127.0.0.1", "set", $key, $v, $span);
			}
		}catch(Exception $e){
			error_log($e->getTrace());
		}
	}
	
	static function incr($key,$amount=1){
		self::init();
		self::$_singleton->incr($key,$count);
	}
	
	static function decr($key,$amount=1){
		self::init();
		self::$_singleton->decr($key,$count);
	}
	
	static function del($key){
		self::init();
		self::$_singleton->del($key);
	}
	
	static function flush(){
		self::init();
		memcache_flush(self::$_singleton);
	}
	
	public static function __callStatic($name, $arguments){
		self::init();
		//call_user_func_array([self::$_singleton,$name],$arguments);
		self::$_singleton->$name($arguments);
		//call_user_func_array(self::$_singleton->$name,$arguments);
		return true;
		if(method_exists(self::$_singleton, $name)){
			call_user_func_array([self::$_singleton,$name],$arguments);
			return true;
		}else{
			return false;
		}
	}
	
	static public function call($host, $cmd, $key, $value=null, $span=null){
		/*
		$url = "http://$host/__cache.php?cmd=$cmd&key=$key";
		if($value)$url.="&value=$value";
		if($span)$url.="&span=$span";
		error_log("cache.call $url");
		$res = file_get_contents($url);
		try{
			//error_log("cache.call = ".$res);
			$res = json_decode($res, true);
			return (isset($res["result"])) ? $res["result"]:$res;
		}catch(Exception $e){
			return $res;
		}*/
		$m = new Memcache;
		$m->addServer($host, 11211);
		error_log($m->get($key));
		switch($cmd){
			case "get":
				$res = $m->get($key);
				if(strpos($res,"[")===0 || strpos($res,"{")===0)
					return json_decode($res,true);
				else 
					return $res;
			case "set":
				return $m->set($key,$value,isset($span)?$span:3600);
			default:
				return $m->$cmd($key);
		}
	}
}

abstract class __Cache {
	protected $_conf;
	
	abstract public function getConn();
	abstract public function get($key);
	abstract public function set($key, $value, $span=3600);
	abstract public function del($key);
	abstract public function incr($key,$amount=1);
	abstract public function decr($key,$amount=1);
	abstract public function isConnecting();
	public function dump($keys){}
	public function recover(){}
	public function add($key, $value, $time){}
	private function __construct(){}
	static public function factory($engine=null){
		if($engine==null)
			$engine = defined(CACHE_ENGINE)?CACHE_ENGINE:"apc";
		$classname = ucfirst($engine).'Cache';
		if (require_once $classname.'.inc'){
			$cache = new $classname;
			$cache->_conf = [];
			if(defined(CACHE_HOSTS))
				$cache->_conf["hosts"] = CACHE_HOSTS;
			if(defined(CACHE_PORT))
				$cache->_conf["port"] = CACHE_PORT;
			return $cache;
		}
		return null;
	}
	/**
	 * 
	 * get value
	 * @param String $key
	 * @param Cache $cache
	 * @param       $defaultValue
	 */
	static public function getValue($key, $cache=null, $defaultValue) {
		if($cache == null)
			$cache = Cache::factory();

		$value = $cache->get($key);
		if($value == false)
			return $defaultValue;
		else
			return $value;
	}
	
	/**
	 * get int vlaue
	 * 
	 * @param $key
	 * @param $cache
	 * @param $defaultInitValue
	 */
	static public function getInt($key, $cache=null, $defaultValue=0) {
		return self::getValue($key, $cache, $defaultValue);
	}
	
	/**
	 * incr int vlaue
	 * 
	 * @param $key
	 * @param $cache
	 * @param $defaultInitValue
	 */
	static public function incrInt($key, $cache=null, $incrValue=1, $defaultInitValue=0){
		if($cache == null)
			$cache = Cache::factory();
		
		$value = $cache->get($key);
		if($value == false)
			$cache->set($key, $defaultInitValue);// init value

		$cache->incr($key, $incrValue);
	}
}

?>