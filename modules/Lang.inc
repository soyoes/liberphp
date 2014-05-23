<?php

/**
 *String contains
 * 
 */
function str_has($haystack, $needle){
	if(!is_string($haystack)||!is_string($needle))return false;
	return strpos($haystack, $needle) !== false;
}

/**
 *String starts with 
 */
function str_starts($haystack,$needle,$case=true) {
	if($case){return (strcmp(substr($haystack, 0, strlen($needle)),$needle)===0);}
	return (strcasecmp(substr($haystack, 0, strlen($needle)),$needle)===0);
}

/**
 *String ends with
 */
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

function hash_incr($data, $key, $amount){
	$v = self::get($data,$key,true,0);
	$v += $amount;
	return self::set($data, $key, $v);
}

/**
 * set keyPath=>v data to assoc array
 * @param assoc array $data :{$k=>$v}
 * @param string $keyPath : "property1.subProperty1"
 * @param unknown_type $val
 * @return assoc array: return changes of target array. use to tell Observer;
 */
function hash_set(&$data, $keyPath, $val){
	$paths = explode(".", $keyPath);
	$o = &$data;
	$current_path = "";
	$path_size = count($paths);
	$key = $paths[0];
	$org = isset($data[$key])? $data[$key]: null;
	for ($i=0; $i<$path_size; $i++){
		$path = $paths[$i];
		if (is_string($o) && (str_starts($o, "{") || Strings::startsWith($o, "[")))
			$o = json_decode($o,true);
		if ($i == $path_size-1){
			$o[$path] = $val;
		}else{
			if (!isset($o[$path]))
				$o[$path] = [];
			$o = &$o[$path];
		}
	}
	return ["key"=>$key, "val"=>$data[$key], "org"=>$org];
}


/**
 * get value from assoc array by keyPath
 * @param assoc array $data
 * @param string $keyPath
 * @param boolean $autoCreate, create if not exists
 * @param mixed $defaultValue, return this value if not exists
 * @return NULL|unknown|$defaultValue
 */
function hash_get(&$data, $keyPath, $autoCreate=true, $defaultValue=null){
	if (empty($data)) {
		if($autoCreate){
			hase_set($data, $keyPath, $defaultValue);
		}else
			return $defaultValue;
	}
	$paths = explode(".", $keyPath);
	$o = $data;
	$current_path = "";
	while (count($paths)>1){
		$path = array_shift($paths);
		if (is_string($o) && (str_starts($o, "{") || Strings::startsWith($o, "[")))
			$o = json_decode($o,true);
		if (!isset($o[$path])){
			return $defaultValue;
		}
		$o = $o[$path];
	}
	if (is_string($o) && (str_starts($o, "{") || Strings::startsWith($o, "[")))
		$o = json_decode($o,true);
	$key = array_pop($paths);
	if(!isset($o[$key]))
		return $defaultValue;
	return $o[$key];
}

/**
 * change list [{},{},{}] to hash {k=>v, k=>v, k=>v}
 * @param array $arr
 * @param unknown_type $keyName specify field of list element and use it as the key to render hash-table
 * @param unknown_type $valueName, NULL? {k=>ListElement} : {k=>ListElement[valueName]}
 * @return Ambigous <multitype:, unknown>
 * @example
 *		$arr = [{a=>1, b=4},{a=2, b=3}]
 * 		a_2hash($arr, 'a', 'b'); result :{1=>4, 2=>3}
 * 		a_2Hash($arr, 'a'); result : {1=>{a=>1, b=4},2=>{a=2, b=3}}
 *
 */
function arr2hash($arr, $keyName, $valueName=null){
	$hash = [];
	foreach ($arr as $e){
		$hash["".$e[$keyName]] = $valueName==null ? $e : $e[$valueName];
	}
	return $hash;
}


/**
 * remove elements from [{k1=>v1, k2=>v2},{k1=v, k2=>v}] while K==V
 * @param:$arr
 * @param:$conditions:
 * @param:$firstOnly:TRUE|FALSE
 * @return:0=OK, 1=Nothing removed
 * @example:
 * 		ds_remove($myArr, array("id"=1)); //remove elements with id=1 from myArr
 * 		ds_remove($myArr, ["a","b"]); //remove elements which inside ["a","b"];
 * 		ds_remove($myArr, "a"); //remove elements which equals "a";
 * 		ds_remove($myArr, function($e){return $e==null;}); //remove elements which equals "";
 *
 **/
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

/**
 * [{k1=>v1, k2=>v2},{k1=v, k2=>v}] find assoc-array from list
 * @param $arr: the target 
 * @param $conditions:
 * @param $firstOnly:TRUE=find the 1st only
 * @param $operator: "eq", "lt", "gt", "le", "ge", "ne"
 * @return $firstOnly==TRUE? array(hash) / array | null
 * 
 * @example : a_find($myArr, array("id"=>10)); //find all elements with id==10 from list $myArr
 * @example : a_find($myArr, array("type"=>"card")); //find all elements with "type"=="card" from list $myArr
 * 
 */
function ds_find($arr, $opts,$firstOnly=false){
	if(empty(Consts::$arr_query_filters))
		Consts::$arr_query_filters = [
		"=" 	=> function($o,$k,$v){return $o[$k]===$v;},
		"!" 	=> function($o,$k,$v){return $o[$k]!==$v;},
		"<" 	=> function($o,$k,$v){return $o[$k]<$v;},
		">" 	=> function($o,$k,$v){return $o[$k]>$v;},
		"<=" 	=> function($o,$k,$v){return $o[$k]<=$v;},
		">=" 	=> function($o,$k,$v){return $o[$k]>=$v;},
		"[]" 	=> function($o,$k,$v){return is_array($v)&&in_array($o[$k],$v);},
		"![]" 	=> function($o,$k,$v){return is_array($v)?!in_array($o[$k],$v):true;},
		"()" 	=> function($o,$k,$v){return is_array($v) && count($v)==2 && $o[$k]>=min($v[0],$v[1]) && $o[$k]<=max($v[0],$v[1]);},
		"!()" 	=> function($o,$k,$v){return !is_array($v) || count($v)<2 || $o[$k]<min($v[0],$v[1]) || $o[$k]>max($v[0],$v[1]);},
		"?"  	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && str_has($o[$k], $v); },
		"!?"  	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !str_has($o[$k], $v); },
		"~" 	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && preg_match("/".$v."/", $o[$k]);},
		"!~"	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !preg_match("/".$v."/", $o[$k]);},
		"~~" 	=> function($o,$k,$v){return !empty($o[$k]) && !empty($v) && preg_match("/".$v."/i", $o[$k]);},
		"!~~"	=> function($o,$k,$v){return empty($o[$k]) || !empty($v) || !preg_match("/".$v."/i", $o[$k]);},
	];
	if(empty($opts))return false; 
	$res = null;
	foreach ($arr as $a){
		$match = true;
		foreach ($opts as $k=>$v){
			$cmd = strstr($k, '@');
			$func = Consts::$arr_query_filters[$cmd];
			if (!$func($a[$k],$v)){
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

/**
 *
 * @param:$arr: target arr
 * @param:$sortKey:use to sort array like [{k1=>v1, k2=>v2},{k1=v, k2=>v}].
 * @param:$sortOrder:1=ASC, 0=DESC
 * @return:sorted array
 * @uses: only numerice | string values can be sorted
 * @example: a_sort($myArr, "id"); //sort myArr by key id with ASC order.
 *
 **/
function ds_sort($arr, $sortKey=null, $sortOrder=1, $comparator=null){
	if(isset($sortKey)){
		if($comparator==null){
			$cfmt = '$av=$a["%s"];if(!isset($av))$av=0;$bv=$b["%s"];if(!isset($bv))$bv=0;if($av==$bv){return 0;} return is_string($av)?strcmp($av,$bv)*%d:($av>$bv)?-1*%d:1*%d;';
			$code = sprintf($cfmt, $sortKey, $sortKey, $sortOrder, $sortOrder,$sortOrder);
			$cmp = create_function('$a, $b', $code);
			usort($arr, $cmp);
		}else
			usort($arr, $comparator);
		return $arr;
	}else{
		asort($arr);
		return $arr;
	}
}

/**
 * Get milliseconds
 */
function ms(){
    list($usec, $sec) = explode(" ", microtime());
    return ((int)((float)$usec*1000) + (int)$sec*1000);
}

/**
 * Write hash to ini file
 */
function fs_put_ini($file, array $options){
	$tmp = '';
	foreach($options as $section => $values){
		$tmp .= "[$section]\n";
		foreach($values as $key => $val){
			if(is_array($val)){
				foreach($val as $k =>$v)
					$tmp .= "{$key}[$k] = \"$v\"\n";
			}else
				$tmp .= "$key = \"$val\"\n";
		}
		$tmp .= "\n";
	}
	file_put_contents($file, $tmp);
	unset($tmp);
}


/**
 * @example $id = 541 , $tokenLength = 100
 * 	path = 541%100=41/541
 */
function fs_archived_path ($id, $tokenLength=1000){
	$arch =  (int)$id % (int)$tokenLength;
	return "$arch/$id";
}

/**
 *Mkdir and subdir automatically
 */
function fs_mkdir($out){
	$folder = (str_has($out,"."))? preg_replace('/[^\/]*\.[^\/]*$/','',$out):$out;
	if(!file_exists($folder))
		mkdir($folder, 0775, TRUE);
}

function fs_xml2arr($xmlString){
	return json_decode(json_encode((array)simplexml_load_string($xmlString, 'SimpleXMLElement', LIBXML_NOCDATA)), TRUE);
}

/**
 * Get Annotations from comment string (multi-line)
 * @param $comm: multi-line comment string.
 * @example:
 * 	fs_annotations("*this is a test function\n@param $a:int\n@param $b:string\n@permission:775\n*");
 *  //the output will be
 *  Array(
 *  	"desc"	=> "this is a test function",
 *   	"param"	=> [
 *   		["attr"=>"$a", "value"=>"int"],
 *   		["attr"=>"$b", "value"=>"string"],	
 *   	],
 *   	"permission"	=> "775"
 *  ) 
 *   
 */
function fs_annotations($comm){
	$comm = explode("\n",preg_replace(['/\/\*\s*/m','/\s*\*\/\s*/m'],'',$comm));
	$anno = [];
	$rows = count($comm);
	$tag = null; $value=[]; $attr= null;
	for($i=0;$i<=$rows;$i++){
		$cm = trim(preg_replace('/^\s*\**\s*/','',$i<$rows?$comm[$i]:''));
		preg_match_all('/^@(?P<tag>[a-zA-Z]+)\s*(?P<attr>[a-zA-Z\d_\$]*)\s*[:=]\s*(?P<value>.*)/i',$cm,$matches);
		//save tag
		if(!empty($matches['tag']) || $i==$rows){
			if(empty($tag))$tag = 'desc';
			if(empty($anno[$tag]))
				$anno[$tag] = [];
			$anno[$tag] []= ['value'=>join('\n', $value),'attr'=>$attr];
			$tag = null; $value=[]; $attr = null;
		}
		//new tag
		if(!empty($matches['tag'])){
			$tag = trim(strtolower($matches['tag'][0]));
			$value []= preg_replace('/^:\s*/','',trim($matches['value'][0]));
			$attr = preg_replace('/^:\s*/','',$matches['attr'][0]);
		}else if(!empty($cm)){
			$value []= $cm;
		}
	}
	foreach ($anno as $key=>$vs){
		if(count($vs)==1) $anno[$key] = $vs[0]['value'];
	}
	return $anno;
}

/**
 * get source tree of php file, auto include
 * @param $phpfile: php file full path
 * @example:
 * 	fs_src_tree("/tmp/test.php");
 * 	//result will be something like this
 * 	Array(
 * 		"annotations"	=> [...] //file comment annotations
 * 		"functions" => [ 	//non-class functions
 * 			"func1" => ["annotations"=>[...], "params"=>[...]],
 * 			"func2" => ["annotations"=>[...], "params"=>[...]],
 * 		],
 * 		"classes"	=> [
 * 			"annotations"=>[...],
 * 			"properties"=>[
 * 				"prop1"	=> [visibility=>public, static=>1],
 * 				"prop2"	=> [visibility=>public, static=>0],
 * 			],
 * 			"methods"=>[
 * 				"method1" => [visibility=>public, static=>1, abstract=>0, annotations=>[...], params=>[...]],
 * 				...
 * 			],
 * 		]
 * 	)
 */
function fs_src_tree($phpfile){
	$src = file_get_contents($phpfile);
	require_once $phpfile;
	//file comment
	preg_match_all('/<\?php\s*\/\*+\s*(?P<comment>.*?)\*\/\s*/sm', $src, $fdef);
	$comment = $fdef["comment"][0];
	//classes
	preg_match_all('/^(abstract)*\s*(class|trait)\s+(?P<cls>[\w\d]+)\s*/mi', $src, $ma);
	$classes = [];
	if(!empty($ma['cls'])){
		foreach ($ma['cls'] as $cls){
			$classes[$cls] =[];
			$cr = new ReflectionClass($cls);
			$classes[$cls]["name"] = $cls;
			//parent
			$parent = $cr->getParentClass();
			if($parent) $classes[$cls]['parent']=$parent->getName();
			//interfaces
			$classes[$cls]['interfaces']=$cr->getInterfaceNames();
			//abstract
			$classes[$cls]['abstract']=$cr->isAbstract();
			//trait
			$classes[$cls]['trait']=$cr->isTrait();
			//class annotations
			$comm = $cr->getDocComment();
			if($comm==$comment) $comment="";
			$classes[$cls]['annotations']=fs_annotations($comm);
			//methods
			$methods = $cr->getMethods();
			foreach ($methods as $mr){
				$args = array_map(function($e){return $e->name;}, $mr->getParameters());
				$anno = fs_annotations($mr->getDocComment());
				$classes[$cls]["methods"][$mr->getName()] = [
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
			//properties
			$props = $cr->getProperties();
			foreach ($props as $pr){
				$classes[$cls]["properties"][$pr->getName()] = [
				'visibility' => $pr->isPrivate()?'private':($pr->isProtected()?'protected':'public'),
				'static' => $pr->isStatic()
				];
			}
		}
	}
	//non-class functions
	preg_match_all('/^function\s+(?P<func>[\w\d_]+)\s*\(/mi', $src, $ma);
	$funcs = [];
	if(!empty($ma['func'])){
		foreach ($ma['func'] as $fn){
			$ref = new ReflectionFunction($fn);
			$args = array_map(function($e){return $e->name;}, $ref->getParameters());
			$comm = $ref->getDocComment();
			if($comm==$comment) $comment="";
			$anno = fs_annotations($comm);
			$funcs[$fn] = ['annotations'=>$anno, 'params'=>$args, 'name'=>$fn];
		}
	}
	return ['annotations' => empty($comment)?[]:fs_annotations($comment),'functions' => $funcs,'classes' => $classes];
}
