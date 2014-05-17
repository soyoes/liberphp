<?php
/**
 *	@file: SEORender.inc
 *	@author: Soyoes 2014/05/18
 *	@uses: render html from data, to support SEO bots.
 *  Data could be readed from javascript 
 *****************************************************************************/

class SEORender{
	/**
	 * render html code with data. for search engine bots 
	 * assoc array will be rendered as <div data-key="{$KEY_COMES_FROM_PARENT}"><div data-key="{$KEY}">{$VALUE}</div>...</div>
	 * array(list) will be rendered as <ol data-key="{$KEY_COMES_FROM_PARENT}"><li data-idx=0>{$ELEMENT}</li>...</ol>  
	 * 
	 * @param $data: data to render
	 * @param markupOpts: an assoc array to specify how to export markup tags such as  H1, H3, A(internal links)
	 * 
	 * @example :
	 $json =<<< EOF
	{"datas":[{"id":"16","title":"task 1","author":{"name":"Jerry","id":13}},{"id":"24","title":"task 2","author":{"name":"Mary","id":36}},{"id":"30","title":"task 3","author":{"name":"Tom","id":44}}],"page":1}
	EOF;
	$code = SEO::data2html(json_decode($json,true), ["datas"=>["title"=>":title","link"=>"/task/:id","label"=>":title"],"@ROOT"=>["title"=>"TASKS LIST"]]);
	echo $code; 
	 * 
	 * 
	 */
	static function data2html ($data, $markupOpts=[], $key=null){
		$code = "";
		if($key==null)$key="@ROOT";
		$mo = empty($markupOpts[$key])?[]:$markupOpts[$key];
		if(is_hash($data)){
			//$code = "<div ".'data-key="'.$key.'">';
			if(!empty($mo["title"])){
				$tag = $key=="@ROOT"?"h1":"h3";
				$title = $mo["title"];
				if(str_has($title,":")){
					preg_match_all('/:(?<field>[\w\d_]+)/',$title,$matches);
					foreach ($matches["field"] as $fd)
						$title = preg_replace('/:('.$fd.')/',$data[$fd],$title);
				}
				$code.="<$tag data-markup>".$title."</$tag>";
			}
			foreach ($data as $k=>$v){
				if(is_hash($v)){
					$v = self::data2html($v,$markupOpts,str_replace('@ROOT.','',$key.".".$k));
					$code.='<div data-key="'.$k.'">'.$v.'</div>';
				}else if(is_array($v))
					$code .= self::data2html($v,$markupOpts,str_replace('@ROOT.','',$key.".".$k));
				else
					$code.='<div data-key="'.$k.'">'.$v.'</div>';
			}
			if(!empty($mo["link"])){
				preg_match_all('/:(?<field>[\w\d_]+)/',$mo["link"],$matches);
				$url = $mo["link"];
				foreach ($matches["field"] as $fd) 
					$url = preg_replace('/:('.$fd.')/',$data[$fd],$url);
				$label = $url;
				if(!empty($mo["label"])){
					$label = $mo["label"];
					preg_match_all('/:(?<field>[\w\d_]+)/',$mo["label"],$matches);
					foreach ($matches["field"] as $fd)
						$label = preg_replace('/:('.$fd.')/',$data[$fd], $label);
				}
				$code.='<a data-markup href="'.$url.'">'.$label."</a>";
			}
			//$code .="</div>";
		}else if(is_array($data)){
			$code = "<ol ".'data-key="'.$key.'">';
			$idx = 0;
			foreach ($data as $d){
				$v = (is_array($d))? self::data2html($d,$markupOpts,$key):$d;
				$code.='<li data-idx="'.($idx++).'">'.$v.'</li>';
			}
			$code .="</ol>";
		}else{
			$code .= $data;
		}
		return $code;
	}
	
}
