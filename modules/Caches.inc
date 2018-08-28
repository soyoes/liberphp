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

/**
 * Set value with key to APC -> Memcached
 * @param $key:
 * @param $value:
 * @param $time: default=3600(1 hour)
 * @param $sync: whether sync to memcache. default=true
 * @return : boolean
 */
function cache_set($key, $value, $time=3600, $sync=true){
	$k = APP_NAME."::".$key;
	$s = apc_store($k, $value, $time);
	return ($time && $sync)?mc_set($key,$value,$time):$s;
}

/**
 * Delete cache with key
 * @param $key:
 * @param $sync: whether del memcache also. default=true
 * @return : boolean
 */
function cache_del($key,$sync=true){
	$k = APP_NAME."::".$key;
	$r = apc_delete($k);
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
	$k = APP_NAME."::".$key;
	$res = apc_inc($k, $amount);
	return ($sync)?mc_inc($key,$amount):$res;
}

/**
 * Dump APC cache to file.
 * The file path will be PROJECT_PATH/tmp/apc.data
 * @return : The number of bytes written | false for failure
 */
function cache_dump(){
	return @apc_bin_dumpfile([],null, APP_DIR.__SLASH__."tmp".__SLASH__."apc.data");
}

/**
 * Load bin file to APC cache.
 * The file path will be PROJECT_PATH/tmp/apc.data
 * @return : boolean
 */
function cache_load(){
	return @apc_bin_loadfile(APP_DIR.__SLASH__."tmp".__SLASH__."apc.data");
}

/**
 * Get memcached connection.
 * Edit [Cache]hosts=yourhostlist in conf/conf.ini to change the settings. 
 * @return false | Memcached object
 */
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

/**
 * Get value by key from memcached
 * @param $key:
 * @return : if failed return false
 */
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

/**
 * Get multiple values by multiple keys from memcached
 * @param $keys: array
 * @return : mixed | if failed return false
 */
function mc_gets($keys){
	$conn = mc_conn();
	$ks = array_map(function($e){return APP_NAME."::".$k;}, $keys);
	if($conn)
		return $conn->getMulti($ks);
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
	$k = APP_NAME."::".$key;
	$conn = mc_conn();
	return ($conn)? $conn->set($k,$value,$time):false;
}

/**
 * Set value with key to memcached
 * @param $key:
 * @param $value:
 * @param $time: default 3600(1 hour)
 * @return : boolean
 */
function mc_sets($datas,$time=3600){
	$ds = [];
	foreach($datas as $k=>$v){
		$ds[APP_NAME."::".$k] = $v;		
	}
	$conn = mc_conn();
	return ($conn)? $conn->setMulti($ds,$time):false;
}

/**
 * Delete value from memcached with key
 * @param $key:
 * @return : boolean
 * 
 */
function mc_del($key){
	$k = APP_NAME."::".$key;
	$conn = mc_conn();
	return ($conn)? $conn->delete($k,$time):false;
}

/**
 * Increase value by key in memcached
 * @param $key:
 * @param $amount:default 1
 * @return : new amount (int) | false if failure
 */
function mc_inc($key,$amount=1){
	$k = APP_NAME."::".$key;
	$conn = mc_conn();
	return ($conn)? $conn->increment($k,$amount):false;
}


//redis_servers : 'host:7000', 'host:7001', 'host:7003'
function redis_conn(){
	$svs = Conf::$redis_servers;
	if(empty($svs))return false;
	if(count($svs)>1){
		$cluster = new RedisCluster(NULL, $svs);
		if(Conf::$redis_pass)
			$cluster->auth(Conf::$redis_pass);
//		$cluster->setOption(Redis::OPT_PREFIX, APP_NAME.'::');
		return $cluster;
	}else{
		$redis = new Redis();
		$redis->connect($svs[0]);
		if(Conf::$redis_pass)
			$redis->auth(Conf::$redis_pass);
//		$redis->setOption(Redis::OPT_PREFIX, APP_NAME.'::');
		return $redis;
	}
}

// key-value
/**
 * Get value by key from redis
 * @param $key:
 * @return : if failed return false
 */
function redis_get($key, $nullHandler=null){
	$conn = redis_conn();
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

/**
 * Set value with key to redis
 * @param $key:
 * @param $value:
 * @param $time: default 3600(1 hour)
 * @return : boolean
 */
function redis_set($key, $value, $time){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if(!$conn)return false;
	if($time>0)
		$conn->setEx($k,$time,$value);
	else
		$conn->set($k,$value);
}

/**
 * Delete value from redist with key
 * @param $key:
 * @return : boolean
 * 
 */
function redis_del($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	return ($conn)? $conn->del($k):false;
}

// Array

/**
 * Set Array with key to redis
 * @param $key:
 * @param $value:
 * @return : boolean
 */
function redis_sAdd($key, $value){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if(!$conn)return false;
	$conn->sAdd($k,$value);
}

/**
 * Sort Array with key to redis
 * @param $key:
 * @param $sort:default asc
 * @return : if failed return false
 */
function redis_arraySort($key, $sort="asc"){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->sort($k, array("sort" => $sort));
		return $v;
	}
	return false;
	
}

// List

/**
 * Adds the string value to the head (left/right) of the list
 * @param $key:
 * @param $head:default right(left or other)
 * @param $value:right
 * @return : If the key exists and is not a list, FALSE is returned.
 */
function redis_push($key, $head,$value){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		if ($head == 'left') {
			$v = $conn->lPush($k, $value);
			return ($v)? $v:false;
		} else {
			$v = $conn->rPush($k, $value);
			return ($v)? $v:false;
		}		
	}
	return false;
}

/**
 * Returns the elements of the list stored in the range [start, end]. 
 * @param $key:
 * @param $start:start default 0
 * @param $end:end default -1
 * @return : Array containing the values in specified range.
 */
function redis_lRange($key, $start = 0,$end = -1){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->lRange($k, $start, $end);
		return $v;
	}
	return false;
}

/**
 * Returns the size of a list identified by Key. 
 * @param $key:
 * @param $start:start default 0
 * @param $end:end default -1
 * @return : If the list didn't exist or is empty, the command returns 0.
 *           If the data type identified by Key is not a list, the command return FALSE.
 */
function redis_lLen($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->lLen($k);
		return $v;
	}
	return false;
}

// Hash
/**
 * Adds a value to the hash stored at key. 
 * @param $key:
 * @param $hKey:hash key
 * @param $value:hash value
 * @param $owFlg:overwrite flag default is 1(overwrite)
 * @return : overwrite mode(hSet)): LONG 1 if value didn't exist and was added successfully, 
 *           0 if the value was already present and was replaced, 
 *           FALSE if there was an error.
 *           the other(hSetNx): BOOL TRUE if the field was set, FALSE if it was already present.
 */
function redis_hSet($key, $hKey,$value,$owFlg=1){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		if ($owFlg == 1) {
			$v = $conn->hSet($k, $hKey, $value);
			return $v;
		} else {
			$v = $conn->hSetNx($k, $hKey, $value);
			return $v;
		}	
	}
	return false;
}

/**
 * Gets a value from the hash stored at key.  
 * @param $key:
 * @param $hKey:hash key
 * @return : If the hash table doesn't exist, or the key doesn't exist, FALSE is returned.
 */
function redis_hGet($key, $hKey){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hGet($k, $hKey);
		return ($v)? $v:false;
	}
	return false;
}

/**
 * Returns the length of a hash, in number of items. 
 * @param $key:
 * @return : LONG the number of items in a hash, FALSE if the key doesn't exist or isn't a hash.
 */
function redis_hLen($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hLen($k);
		return ($v)? $v:false;
	}
	return false;
}
/**
 * Removes a value from the hash stored at key. 
 * @param $key:
 * @param $hKey: hashKey1 hashKey2
 * @return : LONG the number of deleted keys, 0 if the key doesn't exist, FALSE if the key isn't a hash.
 */
function redis_hDel($key,$hKey){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hDel($k,$hKey);
		return ($v)? $v:false;
	}
	return false;
}

/**
 * Returns the keys in a hash, as an array of strings. 
 * @param $key:
 * @return : An array of elements, the keys of the hash. This works like PHP's array_keys().
 */
function redis_hKeys($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hKeys($k);
		return $v;
	}
	return false;
}

/**
 * Returns the values in a hash, as an array of strings. 
 * @param $key:
 * @return : An array of elements, the values of the hash. This works like PHP's array_values().
 */
function redis_hVals($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hVals($k);
		return $v;
	}
	return false;
}

/**
 * Returns the whole hash, as an array of strings indexed by strings. 
 * @param $key:
 * @return : An array of elements, the contents of the hash.
 */
function redis_hGetAll($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hGetAll($k);
		return $v;
	}
	return false;
}

/**
 * Verify if the specified member exists in a key. 
 * @param $key:
 * @param $hKey: memberKey
 * @return : BOOL: If the member exists in the hash table, return TRUE, otherwise return FALSE.
 */
function redis_hExists($key,$hKey){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hExists($k,$hKey);
		return $v;
	}
	return false;
}

/**
 * Fills in a whole hash. Non-string values are converted to string, 
 * using the standard (string) cast. NULL values are stored as empty strings.
 * @param $key:
 * @param $hKeyVals: members: key → value array
 * @return : BOOL
 */
function redis_hMSet($key,$hKeyVals){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hMSet($k,$hKeyVals);
		return $v;
	}
	return false;
}


/**
 * Retrieve the values associated to the specified fields in the hash, 
 * @param $key:
 * @param $hKeys: memberKeys Array
 * @return : Array An array of elements, the values of the specified fields in the hash, 
 *           with the hash keys as array keys.
 */
function redis_hMGet($key,$hKeys){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->hMSet($k,$hMGet);
		return $v;
	}
	return false;
}

// Sort set

/**
 * Add one or more members to a sorted set or update its score if it already exists 
 * @param $key:
 * @param $score: long
 * @param $value: 
 * @return : Long 1 if the element is added. 0 otherwise.
 */
function redis_zAdd($key,$score,$value){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zAdd($k,$score,$value);
		return $v;
	}
	return false;
}


/**
 * Returns the cardinality of an ordered set 
 * @param $key:
 * @return : Long, the set's cardinality.
 */
function redis_zCard($key){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zCard($k);
		return $v;
	}
	return false;
}


/**
 * Returns the number of elements of the sorted set stored at the specified key 
 * which have scores in the range [start,end]. 
 * Adding a parenthesis before start or end excludes it from the range. 
 * +inf and -inf are also valid limits.
 * @param $key:
 * @param $start:string
 * @param $end:string
 * @return : LONG the size of a corresponding zRangeByScore.
 */
function redis_zCount($key,$start='0',$end='-1'){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zCount($k,$start,$end);
		return $v;
	}
	return false;
}

/**
 * Returns a range of elements from the ordered set stored at the specified key, 
 * with values in the range [start, end]. 
 * @param $key:
 * @param $start:long
 * @param $end:long
 * @param $withS:withscores: bool = false
 * @return : Array containing the values in specified range.
 */
function redis_zRange($key,$start='0',$end='-1',$withS=false){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zRange($k,$start,$end,$withS);
		return $v;
	}
	return false;
}


/**
 * Returns the rank of a given member in the specified sorted set, 
 * starting at 0 for the item with the smallest score. 
 * zRevRank starts at 0 for the item with the largest score.
 * @param $key:
 * @param $member:
 * @return : Long, the item's score.
 */
function redis_zRank($key,$member){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zRank($k,$member);
		return $v;
	}
	return false;
}


/**
 * Returns the rank of a given member in the specified sorted set, 
 * starting at 0 for the item with the smallest score. 
 * zRevRank starts at 0 for the item with the largest score.
 * @param $key:
 * @param $member:
 * @return : Long, the item's score.
 */
function redis_zRevRank($key,$member){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zRevRank($k,$member);
		return $v;
	}
	return false;
}


/**
 * Deletes a specified member from the ordered set.
 * @param $key:
 * @param $member:
 * @return : LONG 1 on success, 0 on failure.
 */
function redis_zDelete($key,$member){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zDelete($k,$member);
		return $v;
	}
	return false;
}

/**
 * Returns the elements of the sorted set stored at the specified key in the range [start, end] in reverse order.
 * @param $key:
 * @param $start:long
 * @param $end:long
 * @param $withS:withscores: bool = false
 * @return : Array containing the values in specified range.
 */
function redis_zRevRange($key,$start='0',$end='-1',$withS=false){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zRevRange($k,$start,$end,$withS);
		return $v;
	}
	return false;
}


/**
 * Returns the score of a given member in the specified sorted set.
 * @param $key:
 * @param $member:
 * @return : Double.
 */
function redis_zScore($key,$member){
	$k = APP_NAME."::".$key;
	$conn = redis_conn();
	if($conn){
		$v = $conn->zScore($k,$member);
		return $v;
	}
	return false;
}

