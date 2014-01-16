<?php 

class Strings {

	/*
	 * get string between strA and strB
	 * for instance
	 * 
	 * $str = "/Akaka?";
	 * between($str, "/", "?") => "Akaka"
	 * 
	 * 
	 * */
	static function between($string, $start, $end){
		$string = " ". $string;
		$ini = strpos($string,$start);
		if ($ini == 0) return "";
		$ini += strlen($start);
		$len = strpos($string, $end, $ini) - $ini;
		return substr($string, $ini, $len);
	}
	
	static function startsWith($haystack,$needle,$case=true) {
	    if($case){return (strcmp(substr($haystack, 0, strlen($needle)),$needle)===0);}
	    return (strcasecmp(substr($haystack, 0, strlen($needle)),$needle)===0);
	}
	
	static function endsWith($haystack,$needle,$case=true) {
	    if($case){return (strcmp(substr($haystack, strlen($haystack) - strlen($needle)),$needle)===0);}
	    return (strcasecmp(substr($haystack, strlen($haystack) - strlen($needle)),$needle)===0);
	}
	
	/**
	 * plural -> singular
	 * @param unknown_type $word
	 * @return string 
	 */
	static function toSingular($word){
	    $rules = array( 
	        'ss' => false, 
	        'os' => 'o', 
	        'ies' => 'y', 
	        'xes' => 'x', 
	        'oes' => 'o', 
	        'ies' => 'y', 
	        'ves' => 'f', 
	        's' => '');
	    foreach(array_keys($rules) as $key){
	        if(substr($word, (strlen($key) * -1)) != $key) 
	            continue;
	        if($key === false) 
	            return $word;
	        return substr($word, 0, strlen($word) - strlen($key)) . $rules[$key]; 
	    }
	    return $word;
	}
	
	static function substrFrom($haystack, $needle){
		return Strings::contains($haystack, $needle)? substr($haystack, strpos($haystack, $needle)+strlen($needle)) : $haystack;
	}
	static function substrTo($haystack, $needle){
		return Strings::contains($haystack, $needle)? substr($haystack,0,strpos($haystack, $needle)) : $haystack;
	}

	static function contains($haystack, $needle){
		if(!is_string($haystack)||!is_string($needle))
			return false;
	 	$pos = strpos($haystack, $needle);
        if($pos === false) {
            return false;
        }else {
            return true;
        }
	}

	static function hex2str($hex){
		$string='';
		for ($i=0; $i < strlen($hex)-1; $i+=2){
		   $string .= chr(hexdec($hex[$i].$hex[$i+1]));
		}
		return $string;
	}
	
	static function str2hex($string){
	    $hex='';
	    for ($i=0; $i < strlen($string); $i++){
	        $hex .= dechex(ord($string[$i]));
	    }
    	return $hex;
	}

	static function validEmail($email){
		return false != filter_var( $email, FILTER_VALIDATE_EMAIL );
	}
	
	static function validIP($str){
		return false!=filter_var( $email, FILTER_VALIDATE_IP);
	}
	
	static function validURL($str){
		return false != filter_var( $str, FILTER_VALIDATE_URL );
	}
	
	static function ufirst($arr){
		if(!is_array($arr))return false;
		return array_map(create_function('$e', 'return ucfirst($e);'), $arr);
	}
	
	static function shortcutUrl($url){
		$api = "http://api.bit.ly/v3/shorten?login=o_5ua138bdv2&apiKey=R_91759670f9f0052f1976ac3855b17614&longUrl=$url&format=txt";
		$short = file_get_contents($api);
		return $short;
	}
	
}

?>