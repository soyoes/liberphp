<?php
/**
 * TODO solve <script> <style>
 * TODO text locale
 * TODO CSS
 * 
 * OK{ (condition) ? var1 : var2 }
 * OK{ $a : $b }
 * 
 */

class Render {
	static $path;
	private static $output_path;
	private static $ext='.html';
	
	private $layout = '_layout';
	private $data = [];
	private $contents = [];
	
	private function __construct(){}
	
	static function factory($path){
		self::$path = $path.__SLASH__;
		self::$output_path = APP_DIR.__SLASH__.'tmp'.__SLASH__;
		$render = new Render();
		return $render;
	}
	
	function assign($key, $value){
		$this->data[$key] = $value;	
	}
	
	/**
	 * print to screen
	 * 
	 */
	function render($file,$data=[],$layout=null){
		$template = isset($layout)? $layout : $this->layout;
		$s = cache_get('template-'.$template, function($f){
			return file_get_contents(Render::$path.str_replace('template-','',$f));
		},false);
		list($before, $after) = explode('__CONTENTS__', $s);
		echo $before;
		//set variables
		if(!empty($data))
		foreach ($data as $k=>$v)
			$this->data[$k] = $v;
		extract($this->data);
		if(is_string($file)){
			$r = $this->render_file($file,$data);
			if($r) include($r);
		}else if(is_array($file))
			foreach ($file as $f){
				$r = $this->render_file($f,$data);
				if($r) include($r);
				flush();
			}
		//if($after) {include $after;flush();}
		echo $after;
		unset($this->data);
		unset($data);
	}
	
	/**
	 * get phpcode from file
	 * 
	 */
	function render_file($file,$data=[]){
		$filepath = self::$path.$file;
		if(!file_exists($filepath)) return false;
		$outpath = self::$output_path.'template-'.str_replace(self::$ext,'.php',$file);
		if(!file_exists($outpath)
			||(Consts::$template_check && filemtime($filepath) > filemtime($outpath))){
			$code = $this->compile($filepath);
			if(isset($code) && $code!=""){
				file_put_contents($outpath,$code);
				unset($code);
			}
		}
		return $outpath;
	}

	function compile($file){
		$rows = preg_split('/(\{[^\{^\}]*\})/', file_get_contents($file), -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY ) ;
		$phpcode = '';
		$indent = 0;
		$ignore = false;
		while($code = array_shift($rows)){
			$matched = false;
			preg_match_all('/\{(?P<close>\/*)(?P<tag>(%|=|if|elseif|else|break|ignore|for|var|include){0,1})\s*(?P<val>.*)\}/', $code, $matches);
			
			if(empty($matches[0])){
				$phpcode .= $code;
			}else{
				list($close, $tag, $val) = [$matches['close'][0]=="/"?true:false, $matches['tag'][0], 
						preg_replace('/\.([a-zA-Z0-9_]+)/', "['$1']",(trim($matches['val'][0])))];
				
				if($tag=='' || $tag=='=')$tag='echo';
				if($tag=='%')$tag='text';
				if($close){
					if($tag=='if'||$tag=='for')$indent --;
					if($tag=='ignore'){
						$ignore = false;
					}else{
						$phpcode .= '<?php } ?>';
					}
				}else if($ignore){
					$phpcode .= $code;
				}else{
					switch($tag){
						case 'for':
							$parts = preg_split('/\s*,\s*/',$val,-1,PREG_SPLIT_NO_EMPTY );
							$len = count($parts);
							$indent ++;
							switch($len){
								case 1:$phpcode .= '<?php foreach('.$parts[0].' as $key=>$value) { ?>';break;
								case 2:$phpcode .= '<?php foreach('.$parts[0].' as $key=>'.$parts[1].') { ?>';break;
								default :$phpcode .= '<?php foreach('.$parts[0].' as '.$parts[1].'=>'.$parts[2].') { ?>';break;
							}
							break;
						case 'if':
							$indent ++;
							$phpcode .= '<?php if('.$val.'){ ?>';break;
						case 'elseif':
							$phpcode .= '<?php }else if('.$val.'){ ?>';break;
						case 'else':
							$phpcode .= '<?php }else{ ?>';break;
						case 'break':
							$phpcode .= '<?php break; ?>';break;
						case 'echo':
							$phpcode .= '<?= '.$val.' ?>';break;
						case 'text':
							$phpcode .= '<?= T("'.$val.'"); ?>';break;
						case 'var':
							$phpcode .= '<?php '.$val.'; ?>';break;
						case 'include':
							$val = preg_replace_all('/\'"/',"",$val);
							$phpcode .= '<?php include_template("'.$val.'"); ?>';break;
						case 'ignore':
							$ignore = true;
							break;
						default:break;
					}//end switch
				}//end else
			}
		}
		return $phpcode;
	}
	
}

