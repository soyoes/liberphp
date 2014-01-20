<?php
/**
 *	@file: Caches.inc
 *	@author: Soyoes 2013/08/14
 *	@uses: APC Cache & Memcache functions
 *	@example:
 *****************************************************************************/



/**
 * Get value by key from APC->Memcached 
 * if apc does not contain key, it will try to ask memcached by default.
 * @param $key: 
 * @param $nullHandler: callback function, 
 * 			if hash key does not exist get value from this function
 * @param $sync: whether use memcache.default=true
 * @return : mixed | false
 * */
function cache_get($key, $nullHandler=null,$sync=true){
	$res = apc_fetch($key);
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

/**
 * Set value with key to APC -> Memcached
 * @param $key:
 * @param $value:
 * @param $time: default=3600(1 hour)
 * @param $sync: whether sync to memcache. default=true
 * @return : boolean
 */
function cache_set($key, $value, $time=3600, $sync=true){
	$s = apc_store($key, $value, $time);
	return ($time && $sync)?mc_set($key,$value,$time):$s;
}

/**
 * Delete cache with key
 * @param $key:
 * @param $sync: whether del memcache also. default=true
 * @return : boolean
 */
function cache_del($key,$sync=true){
	$r = apc_delete($key);
	return ($sync)?mc_del($key):$r;
}

/**
 * Increase value by key
 * @param $key:
 * @param $amout: default = 1
 * @param $sync: whether sync to memcache. default=true
 * @return : int new amount
 */
function cache_inc($key,$amount=1,$sync=false){
	$res = apc_inc($key, $amount);
	return ($sync)?mc_inc($key,$amount):$res;
}

/**
 * Dump APC cache to file.
 * The file path will be PROJECT_PATH/tmp/apc.data
 * @return : The number of bytes written | false for failure
 */
function cache_dump(){
	return @apc_bin_dumpfile([],null, TMP_DIR."apc.data");
}

/**
 * Load bin file to APC cache.
 * The file path will be PROJECT_PATH/tmp/apc.data
 * @return : boolean
 */
function cache_load(){
	return @apc_bin_loadfile(TMP_DIR."apc.data");
}

/**
 * Get memcached connection.
 * Edit [Cache]hosts=yourhostlist in conf/conf.ini to change the settings. 
 * @return false | Memcached object
 */
function mc_conn(){
	$conn = new Memcached(APP_NAME);
	if(!defined("CACHE_HOSTS")){
		return false;
	}else{
		$hosts=explode(",", CACHE_HOSTS);
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

/**
 * Get value by key from memcached
 * @param $key:
 * @return : if failed return false
 */
function mc_get($key){
	$conn = mc_conn();
	if($conn)
		return $conn->get($key);
	return false;
}

/**
 * Get multiple values by multiple keys from memcached
 * @param $keys: array
 * @return : mixed | if failed return false
 */
function mc_gets($keys){
	$conn = mc_conn();
	if($conn)
		return $conn->getMulti($keys);
	return false;
}

/**
 * Set value with key to memcached
 * @param $key:
 * @param $value:
 * @param $time: default 3600(1 hour)
 * @return : boolean
 */
function mc_set($key, $value, $time=3600){
	$conn = mc_conn();
	return ($conn)? $conn->set($key,$value,$time):false;
}

/**
 * Set value with key to memcached
 * @param $key:
 * @param $value:
 * @param $time: default 3600(1 hour)
 * @return : boolean
 */
function mc_sets($datas,$time=3600){
	$conn = mc_conn();
	return ($conn)? $conn->setMulti($datas,$time):false;
}

/**
 * Delete value from memcached with key
 * @param $key:
 * @return : boolean
 * 
 */
function mc_del($key){
	$conn = mc_conn();
	return ($conn)? $conn->delete($key,$time):false;
}

/**
 * Increase value by key in memcached
 * @param $key:
 * @param $amount:default 1
 * @return : new amount (int) | false if failure
 */
function mc_inc($key,$amount=1){
	$conn = mc_conn();
	return ($conn)? $conn->increment($key,$amount):false;
}
