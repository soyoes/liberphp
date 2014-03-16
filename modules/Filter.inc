<?php

abstract class Filter{
	private function __construct(){}
	abstract public function before($params);
	abstract public function after($params);
	public static function factory($type){
		if (include APP_DIR.__SLASH__.'filters'.__SLASH__. $type. '.inc') {
            $classname = strtoupper($type[0]).substr($type,1).'Filter';
            return new $classname;
        } else {
            throw new Exception('filter not found');
        }
	}
}

?>