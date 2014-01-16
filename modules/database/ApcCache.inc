<?php

/**
 *	@file: ApcCache.inc	
 *	@author: Soyoes 2013/08/13
 *	@uses:
 *	@example: 
 *****************************************************************************/

	
class ApcCache extends __Cache{
	
	public function get($key){
		return apc_fetch($key);	
	}
	
	public function set($key, $value, $span=3600){
		return apc_store($key, $value, $span);
	}
	
	public function del($key){
		return apc_delete($key);
	}
	
	public function incr($key,$amount=1){
		return apc_inc($key, $amount);
	}
	
	public function decr($key,$amount=1){
		return apc_dec($key, $amount);
	}
	
	public function isConnecting(){
		return true;
	}
	
	public function add($key,$value, $time=3600){
		return apc_add($key,$value, $time);
	}
	
	public function dump($keys){
		error_log("apc.dump");
		return @apc_bin_dumpfile([],$keys, TMP_DIR."apc.data");
	}
	
	public function recover(){
		return @apc_bin_loadfile(TMP_DIR."apc.data");
	}
	
	public function getConn(){
		return true;
	}
	
}