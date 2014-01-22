<?php
class Files {
	
	static function writeINI($file, array $options){
	    $tmp = '';
	    foreach($options as $section => $values){
	        $tmp .= "[$section]\n";
	        foreach($values as $key => $val){
	            if(is_array($val)){
	                foreach($val as $k =>$v){
	                    $tmp .= "{$key}[$k] = \"$v\"\n";
	                }
	            }
	            else
	                $tmp .= "$key = \"$val\"\n";
	        }
	        $tmp .= "\n";
	    }
	    file_put_contents($file, $tmp);
	    unset($tmp);
	}
	
	
	/**
	 * 
	 * @example
	 * $dir_path : 
	 * $condition : "*.inc";
	 * 
	 * */
	static function ls($dir_path,$condition){
		//get all image files with a .jpg extension.
		$files = glob($dir_path . $condition);
		return $files;
	}


	/**
	* @example id = 541 , range = 100
	* 	path = 541%100=41/541
	*
	*/
	static function archivePath ($id, $range){
		$arch =  (int)$id % (int)$range;
		return "$arch/$id";
	}

	static function mkdir($out){
		$parts = explode(".",$out);
		$path = explode("/", $parts[0]);
		unset($path[count($path)-1]);
		$folder = join("/", $path);
		if(!file_exists($folder)){
			mkdir($folder, 0775, TRUE);
		}
	}

}