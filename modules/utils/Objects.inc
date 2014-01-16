<?php


class Objects{
	/**
	 * get node from a deep object
	 * @param array $arr
	 * @param string $keypath parent.child1.attr1.xxxx ...
	 * @return mixed	
	 */
	static function getNode($obj, $keyPath){
		$paths = explode(".", $keyPath);
		$o = $obj;
		$current_path = "";
		while (count($paths)>=1){
			$path = array_shift($paths);
			if (!isset($o->$path))
				return null;
			$o = $o->$path;
		}
		//$path = array_pop($paths);
		//return $o->$paths;
		return $o;
	}
	
	
	static function dumpMethods($obj){
		$class = get_class($obj);
		$methods = get_class_methods($obj);
		foreach ($methods as $m){
			$r = new ReflectionMethod($class, $m);
			$params = $r->getParameters();
			$ps = array();
			foreach ($params as $param) {
			    //$param is an instance of ReflectionParameter
			    $ps[]= $param->getName();
			}
			$psStr = "$".join(", $",$ps);
			$str = $m."(".$psStr.")";
			echo $str."<br>";
		}
		
	}
	
	static function getParamNames($className, $methodName){
		$r = new ReflectionMethod($className, $methodName);
		$params = $r->getParameters();
		$names = array();
		foreach($params as $p)
			$names [] = $p->getName();
		return $names;
	}
	
}

?>