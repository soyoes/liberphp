<?php
/**
 *	@file: Render.inc
*	@author: Soyoes 2014/01/10
*	@uses: html template render class
*	@example:
*****************************************************************************/


/**
 * TODO solve <script> <style> without {ignore}
 * 
 * TODO add custome tags like <liber::pages>
 * 
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
		$key = 'template-'. REQ::getTemplateType() . "-" .$template;
		$s = cache_get($key, function($f){
			$key_prefix = 'template-'. REQ::getTemplateType() . "-";
			return file_get_contents(Render::$path.str_replace($key_prefix,'',$f));
		},false);
		list($before, $after) = explode('__CONTENTS__', $s);
		if(!(REQ::isTestMode() && REQ::stackSize()>1)) 
			REQ::write($before, "html");
		//set variables
		if(!empty($data))
		foreach ($data as $k=>$v)
			$this->data[$k] = $v;
		extract($this->data);
		//$output="--";
		if(is_string($file)){
			$r = $this->render_file($file,$data);
			if($r) {
				ob_start(); include($r);
				$output = ob_get_contents();
				ob_end_clean();
			};
		}else if(is_array($file))
			foreach ($file as $f){
				$r = $this->render_file($f,$data);
				if($r) {
					ob_start(); include($r);
					$output = ob_get_contents();
					ob_end_clean();
				};
				flush();
			}
		REQ::write($output, "html");
		//if($after) {include $after;flush();}
		if(!(REQ::isTestMode() && REQ::stackSize()>1)) 
			REQ::write($after, "html");
		
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
		$prefix  = 'template-' . REQ::getTemplateType() . '-';
		$outpath = self::$output_path. $prefix .str_replace(self::$ext,'.php',$file);
		if(!file_exists($outpath)
			||(Consts::$mode=="Developing" && filemtime($filepath) > filemtime($outpath))){
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
		//CustomTag get user tags from delegate
		$delegate_methods = get_class_methods('RenderDelegate');
		$custom_tags = [];
		foreach ($delegate_methods as $m) 
			if(str_starts($m,'tag_'))
				$custom_tags []= preg_replace('/^tag_/','',$m);
		
		//CustomTag make regex compression
		$tags_regexp = (!empty($custom_tags)) ?
			"(%|=|if|elseif|else|break|ignore|for|var|include|".join('|',$custom_tags).")":
			"(%|=|if|elseif|else|break|ignore|for|var|include)";
		
		while($code = array_shift($rows)){
			$matched = false;
			preg_match_all('/\{(?P<close>\/*)(?P<tag>'.$tags_regexp.'{0,1})\s*(?P<val>.*)\}/', $code, $matches);
			
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
				}else if(!empty($custom_tags)&&in_array($tag, $custom_tags)){
					//custom tags
					$code = call_user_func(["RenderDelegate","tag_".$tag],$val,$this->data);
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
								default :
									if(preg_match('/^\d+$/', $parts[1])&&preg_match('/^\d+$/', $parts[2])){
										$phpcode .= '<?php for($'.$parts[0].'='.$parts[1].';$'.$parts[0].'<'.$parts[2].';$'.$parts[0].'++) { ?>';
									}else
										$phpcode .= '<?php foreach('.$parts[0].' as '.$parts[1].'=>'.$parts[2].') { ?>';break;
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
						default:
							break;
					}//end switch
				}//end else
			}
		}
		return $phpcode;
	}
	
	/**
	 * 
	 * @param page: current page number
	 * @param total: total records
	 * @param opts: assoc-array (paginate options), perPage|items
	 * @return : array of page index numbers. (starts with 1)
	 * @example : 
	 * 		Render::paginate(11,1000);
	 		// result :{pages:[1,0,7,8,9,10,11,12,13,14,15,0,100], cursor:6}
	 		// 0 means nothing/blank/...
	 * */
	static function paginate($page,$total,$opts=['perPage'=>20]){
		$pp = ($opts['perPage']>0)? $opts['perPage']: 20;
		$pi = $opts['items']?$opts['items']:9; 
		$ptt = ceil($total/$pp); $half=floor($pi/2);
		$pages = []; $begin = max(1,$page-$half); $end = min($ptt, $page+$half);
		$cursor = 0;
		for($i=$begin;$i<=$end;$i++){
			$pages[]=$i;
			if($i<$page) $cursor++;
		}
		if($begin>2) {
			$b = $begin;
			while(count($pages)<$pi-2 && $b>2){
				array_unshift($pages,--$b);$cursor++;
			}
			if(count($pages)>$pi-2)$pages =[1,0]+$pages;
			else{$pages = array_merge([1,0],$pages); $cursor+=2;}
		}
		if($begin==2) {array_unshift($pages, 1); $cursor++;}
		if($ptt-$end>2) {
			while(count($pages)>$pi-2){
				$p = array_pop($pages);
				if($p<$page)$cursor--;
			}
			if(count($pages)<$pi-2){
				$e = $end;
				while(count($pages)<$pi-2 && $e<$ptt-2){array_push($pages, ++$e);}
			}
			$pages = array_merge($pages,[0,$ptt]);
		}
		if($ptt-$end==2) $pages []= $ptt;
		return ['pages'=>$pages,'cursor'=>$cursor];
	}
	
}

