<?php
/**
 *	@file: Arrays.inc
 *	@author: Soyoes 2011/03/20
 *	@uses: array methods
 *	@example:
 *	
 *	TODO split arrays and hash
 *
 *****************************************************************************/

class Arrays{

	/**
	 * [{k1=>v1, k2=>v2},{k1=v, k2=>v}] find assoc-array from list
	 * @param $arr: the target 
	 * @param $conditions:
	 * @param $firstOnly:TRUE=find the 1st only
	 * @param $operator: "eq", "lt", "gt", "le", "ge", "ne"
	 * @return $firstOnly==TRUE? array(hash) / array | null
	 * 
	 * @example : hash_array_search($myArr, array("id"=>10)); //find all elements with id==10 from list $myArr
	 * @example : hash_array_search($myArr, array("type"=>"card")); //find all elements with "type"=="card" from list $myArr
	 * 
	 */
	static function search($arr, $conditions, $firstOnly=FALSE, $operator="eq"){
		$operators = array(
			"lt" => '<',
			"le" => '<=',
			"eq" => '==',
			"ge" => '>=',
			"gt" => '>',
			"ne" => '!='
		); 
		if(!isset($conditions)||empty($conditions))
			return null; 
		$res = null;
		$comparator = create_function('$a, $b', 'return $a '.$operators[$operator].' $b;');
		foreach ($arr as $el){
			$match = TRUE;
			foreach ($conditions as $k=>$v){
				if (!isset($el[$k]) || !$comparator($el[$k],$v)){
					$match = FALSE;
					break;
				}
			}
			if ($match)
				if ($firstOnly)
					return $el;
				else
					$res[] = $el;
		}
		return $res;
	}
	
	/**
	 * [{k1=>v1, k2=>v2},{k1=v, k2=>v}] 
	 * @param:$arr : the target
	 * @param:$conditions
	 * @return:boolean
	 * 
	 * @example: in_hash_array($myArr, array("id"=>10)); //tell if element with id==10 exists
	 * @example: in_hash_array($myArr, array("type"=>"card")); //tell if element with "type"=="card" exists
	 * 
	 **/
	static function exists($arr, $conditions){
		return hash_array_search($arr, $conditions)!=null;
	}
	
	/**
	 * 
	 * @param:$arr: target arr
	 * @param:$sortKey:use to sort array like [{k1=>v1, k2=>v2},{k1=v, k2=>v}].
	 * @param:$sortOrder:1=ASC、-1=DESC
	 * @return:sorted array
	 * @uses: only numerice | string values can be sorted
	 * @example: hash_array_sort($myArr, "id"); //sort myArr by key id with ASC order.
	 * 
	 **/
	static function sort($arr, $sortKey=null, $sortOrder=1, $comparator=null){
		if(isset($sortKey)){
			if($comparator==null){
				$cfmt = '$av=$a["%s"];if(!isset($av))$av=0;$bv=$b["%s"];if(!isset($bv))$bv=0;if($av==$bv){return 0;} return is_string($av)?strcmp($av,$bv)*%d:($av>$bv)?-1*%d:1*%d;';
				$code = sprintf($cfmt, $sortKey, $sortKey, $sortOrder, $sortOrder,$sortOrder);
				$cmp = create_function('$a, $b', $code);
				usort($arr, $cmp);
			}else{
				usort($arr, $comparator);
			}
			return $arr;
		}else{
			asort($arr);
			return $arr;
		}
	}

	/**
	 * sort by comparator
	 * */
	static function fsort($arr, $comparator){
		usort($arr, $comparator);
		return $arr;
	}
	
	/**
	 * remove elements from [{k1=>v1, k2=>v2},{k1=v, k2=>v}] while K==V
	 * @param:$arr
	 * @param:$conditions:
	 * @param:$firstOnly:TRUE|FALSE
	 * @return:0=OK, 1=Nothing removed
	 * @example: 
	 * 		self::remove($myArr, array("id"=1)); //remove elements with id=1 from myArr
	 * 		self::remove($myArr, ["a","b"]); //remove elements which inside ["a","b"];
	 * 		self::remove($myArr, "a"); //remove elements which equals "a";
	 * 		self::remove($myArr, function($e){return $e==null;}); //remove elements which equals "";
	 * 
	 **/
	static function remove(&$arr, $conditions, $firstOnly=FALSE){
		if(!isset($conditions)||(is_array($conditions)&&count($conditions)==0))
			return $arr; 
		$res = array();
		$found = false;
		foreach ($arr as $el){
			$match = TRUE;
			if($firstOnly && $found){
				$match = FALSE;
			}else{
				if(Arrays::isHash($conditions)){
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
	
	/**
	 * remove empty
	 * */
	static function trim($arr){
		//return array_filter($arr, create_function('$e', 'return isset($e) && $e!="" && !empty($e);'));
		$a = array();
		foreach ($arr as $e){
			if(isset($e) && $e!="" && !empty($e))
				array_push($a, $e);
		}
		return $a;
		//return array_filter($arr);
	}
	
	/**
	 * count elements with specified conditions from arr.
	 * @param array $arr
	 * @param array $conditions
	 * @return number
	 */
	static function count($arr, $conditions){
		$res = hash_array_search($arr, $conditions, FALSE);
		return $res == null ? 0 : count($res);
	}
	
	
	/**
	 * change list [{},{},{}] to hash {k=>v, k=>v, k=>v}
	 * @param array $arr
	 * @param unknown_type $keyName specify field of list element and use it as the key to render hash-table
	 * @param unknown_type $valueName, NULL? {k=>ListElement} : {k=>ListElement[valueName]} 
	 * @return Ambigous <multitype:, unknown>
	 * @example
	 		$arr = [{a=>1, b=4},{a=2, b=3}]
	 * 		Arrays::toHash($arr, 'a', 'b'); result :{1=>4, 2=>3}
	 * 		Arrays::toHash($arr, 'a'); result : {1=>{a=>1, b=4},2=>{a=2, b=3}}
	 * 
	 */
	static function toHash($arr, $keyName, $valueName=null){
		$hash = array();
		foreach ($arr as $e){
			$hash["".$e[$keyName]] = $valueName==null ? $e : $e[$valueName];
		}
		return $hash;
	}
	
	static function toSerializedHash($arr){
		$hash = array();
		$size = count($arr);
		for ($i = 0; $i<$size; $i+=2){
			if($i+1<$size){
				$hash[$arr[$i]]=$arr[$i+1];
			}
		}
		return $hash;
	}
	
	static function copyFields($from, $to, $fields){
		foreach($fields as $f){
			if(isset($from[$f]))
				$to[$f] = $from[$f];
		}
		return $to;
	}
	
	/**
	 * First only
	 * @param $list
	 * @param $key
	 * @param $val
	 * @return mixed | NULL
	 */
	static function fetch($list, $key, $val){
		if (isset($list) && !empty($list)){
			if (is_string($val)){
				foreach ($list as $item){
					if(isset($item[$key]) && strcmp($item[$key], $val)==0){
						return $item;
					}
				}
			}elseif (is_int($val) || is_float($val)){
				foreach ($list as $item){
					if(isset($item[$key]) && $item[$key] === $val){
						return $item;
					}
				}
			}
		}
		return null;
	}
	
	/**
	 * array to string
	 * @param array $array
	 * @param string $inner_glue, use to join elements' fields
	 * @param string $outer_glue, use to join elements
	 * @return string
	 */
	static function toString($array, $inner_glue, $outer_glue) {
       $output = array();
       foreach( $array as $key => $item )
               $output[] = $key . $inner_glue . $item;
       return implode($outer_glue, $output);
	}
	
	static public function increase($data, $key, $amount){
		$v = self::get($data,$key,true,0);
		$v += $amount;
		return self::set($data, $key, $v);
	}
	
	static public function decrease($data, $key, $amount){
		$v = self::get($data,$key,true,0);
		$v -= $amount;
		return self::set($data, $key, $v);
	}
	
	/**
	 * set keyPath=>v data to assoc array 
	 * @param assoc array $data :{$k=>$v}
	 * @param string $keyPath : "property1.subProperty1"
	 * @param unknown_type $val
	 * @return assoc array: return changes of target array. use to tell Observer; 
	 */
	static public function set(&$data, $keyPath, $val){
		$paths = explode(".", $keyPath);
		$o = &$data;
		$current_path = "";
		$path_size = count($paths);
		$key = $paths[0];
		$org = isset($data[$key])? $data[$key]: null;
		for ($i=0; $i<$path_size; $i++){
			$path = $paths[$i];
			if (is_string($o) && (Strings::startsWith($o, "{") || Strings::startsWith($o, "[")))
				$o = json_decode($o,true);
			if ($i == $path_size-1){
				$o[$path] = $val;
			}else{
				if (!isset($o[$path]))
					$o[$path] = array();
				$o = &$o[$path];
			}
		}
		return array("key"=>$key, "val"=>$data[$key], "org"=>$org);
	}
	
	
	/**
	 * get value from assoc array by keyPath
	 * @param assoc array $data
	 * @param string $keyPath
	 * @param boolean $autoCreate, create if not exists
	 * @param mixed $defaultValue, return this value if not exists
	 * @return NULL|unknown|$defaultValue
	 */
	static public function get(&$data, $keyPath, $autoCreate=true, $defaultValue=null){
		if (!isset($data) || empty($data)) {
			if($autoCreate){
				Arrays::set($data, $keyPath, $defaultValue);		
			}else
				return $defaultValue;
		}
		$paths = explode(".", $keyPath);
		$o = $data;
		$current_path = "";
		while (count($paths)>1){
			$path = array_shift($paths);
			if (is_string($o) && (Strings::startsWith($o, "{") || Strings::startsWith($o, "[")))
				$o = json_decode($o,true);
			if (!isset($o[$path])){
				return $defaultValue;
			}
			$o = $o[$path];
		}
		if (is_string($o) && (Strings::startsWith($o, "{") || Strings::startsWith($o, "[")))
			$o = json_decode($o,true);
		$key = array_pop($paths);
		if(!isset($o[$key]))
			return $defaultValue;
		return $o[$key];
	}
	
	static public function del(&$arr, $keyPath){
		$keys =explode(".", $keyPath);
		$a = &$arr;
		$size = count($keys);
		for($i=0;$i<$size;$i++){
			$k = $keys[$i];
			if($i==$size-1){
				unset($a[$k]);
				return;
			}else{
				$a = &$a[$k];
				if(Arrays::isEmpty($a))
					return;
			}
		}
		return $arr;
	}
	
	static public function createIfNull(&$arr,$key=null,$defaultValue=array()){
		if(self::isEmpty($arr))
			$arr = array();
		if(isset($key)){
			if(!isset($arr[$key]))
				$arr[$key]=$defaultValue;
		}
		return $arr;
	}
	
	static public function isEmpty($arr){
		return (!isset($arr) || empty($arr) || $arr=false);
	}
	
	static public function minAndMax($arr, $field){
		$a = self::sort($arr,$field,1);
		return array($a[0][$field], $a[count($a)-1][$field]);
	}
	static public function min($arr, $field){
		$a = self::sort($arr,$field,1);
		return $a[0][$field];
	}
	static public function max($arr, $field){
		$a = self::sort($arr,$field,-1);
		return $a[0][$field];
	}
	static function isHash($arr){
		if(!is_array($arr))
			return false;
    	return array_keys($arr) !== range(0, count($arr) - 1);
	}
	
	static function arrayType($arr){
		if(self::isHash($arr))
			return 2;
		if(is_array($arr))
			return 1;
		return 0;
	}
	
	/**
	 * the 2nd param $pojoClassName must be a POJO class.
	 * witch has set/get methods for all properties
	 * */
	static function toObject($hash, $pojoClassName=null){
		if(!isset($pojoClassName))
			return (object)$hash;
		$o = new $pojoClassName();
		foreach ($hash as $k=>$v){
			$method = "set".ucfirst($k);
			$o->$method($v);
		}
		return $o;
	}
	
	static function hashMerge($h1,$h2){
		$r = $h1;
		foreach ($h2 as $k=>$v2){
			if(isset($r[$k])){
				$v1 = $r[$k];
				$pa1 = self::arrayType($v1);
				$pa2 = self::arrayType($v2);
				$pattern = $pa1*10+$pa2;
				switch ($pattern){
					case 22:
						$r[$k] = self::hashMerge($v1, $v2);
						break;
					case 21:
					case 12:
					case 11:
						$r[$k] = array_unique(array_merge($v1,$v2));
						break;
					case 10:
						if(!in_array($v2, $v1))
							$r[$k][] = $v2;
						break;
					case 1:
						if(!in_array($v1, $v2)){
							$v2[] = $v1;
							$r[$k] = $v2;
						}
						break;
					case 0:
						$r[$k] = $v1!=$v2? array($v1,$v2):$v1;
						break;
					default:
						break;
				}
			}else{
				$r[$k]=$v2;
			}
		}
		return $r;
	}

}
?>