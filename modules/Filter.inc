<?php

abstract class Filter{

	/*
	 * request type
	 * common, admin, plugin
	 */ 
	var $type;
	
	var $req;
	
	public static $FILTERS;
	
	public static function init(){
		//$CONF
		Filter::$FILTERS = FILTERS_COMMON;//FIXME
	}
	
	public function __construct(){
		if (!isset(Filter::$FILTERS)){
			Filter::init();
		}
		$this->req = get_request();
	}
	
 	public function doFilter(){
		Debugs::dump("Filter.filter");
	}
	abstract public function before();
	abstract public function after();
	
	
	public static function factory($type){
		if (require_once 'filters'.__SLASH__. ucfirst($type) . 'Filter.inc') {
            $classname = strtoupper($type[0]).substr($type,1).'Filter';
            return new $classname;
        } else {
            throw new Exception('filter not found');
        }
	}
	
} 

?>