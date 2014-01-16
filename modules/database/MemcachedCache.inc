<?php

/**
 *	@file: MemcacheCache.inc	
 *	@author: Soyoes 2011/04/27
 *	@uses:
 *	@example: 
 *****************************************************************************/

	
class MemcachedCache extends __Cache{
	
	static protected $conn;
	
	public function getConn(){
		if(!isset(self::$conn)){
			self::$conn = new Memcache();
			$servers = explode(",",$this->_conf['hosts']);
			if(count($servers) ==0)
				throw new Exception("No cache server");
				//return;
			foreach ($servers as $server)
				self::$conn->addServer($server, $this->_conf["port"]);
			//self::$conn->connect($this->_conf['hosts'], $this->_conf['port']);
		}
		return self::$conn;
	}
	
	public function get($key){
		$conn = $this->getConn();
		return $conn->get($key);	
	}
	
	public function set($key, $value, $span=3600){
		$conn = $this->getConn();
		$conn->set($key, $value, 0, $span);
	}
	
	public function del($key){
		$conn = $this->getConn();
		$conn->delete($key);
	}
	
	public function incr($key,$amount=1){
		$conn = $this->getConn();
		$conn->increment($key,$amount);
	}
	
	public function decr($key,$amount=1){
		$conn = $this->getConn();
		$conn->decrement($key,$amount);
	}
	
	public function isConnecting(){
		return (isset(self::$conn));
	}
	
}