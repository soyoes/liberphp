<?php
/**
 *	@file: DOM.inc
 *	@author: Soyoes 2014/05/17
 *	@uses: DOM Render like LiberJS
 *	@notice : this is a little bit SLOWER than Render.inc
 *	@example:
 *	DOM::init();
 * 	_ul(['id'=>'list'],[
 * 		_li('text1')->css(['color'=>'yellow','border'=>'1px solid black;']),
 * 		_li('text2'),
 * 		_li('text3'),
 * 	],_div(['id'=>'frame']));
 * 	DOM::render_html();
 *****************************************************************************/

class DOM{
	static $html,$body,$head;
	static $tagIdx = 1;
	static $single_tags = ['br','hr','img','input'];
	static $double_tags = ['a','bold','body','div','li','ul','html','head',
	'ol','dl','dt','dd','p','span','article','section','aside','footer','header','nav',
	'table','caption','tbody','thead','tfoot','colgroup','col','tr','td','th',
	'form','fieldset','legend','ipt','label','textarea',
	'b','h1','h2','h3','h4','h5','h6','cite','pre','s','u','i','mark','q','small','sub','sup','abbr','bdo','time',
	'em','dfn','code','samp','strong','kbd','var',
	'map','area','figure','figcaption',
	'button','progress','address','base',
	'canvas','embed','audio','video','source','progr'
	//input -> ipt, b->bold, progress->progr

	];
	static function init(){
		self::$html = _html();
		self::$head = _head(self::$html);
		self::$body = _body(self::$html);
	}
	static function add($node){
		if(!$node->parent)
			$node->appendTo(self::$body);
	}
	static function nextIdx(){
		return self::$tagIdx++;
	}
	static function render_html(){
		$out = "<!DOCTYPE html>";
		echo $out;
		echo self::$html;
	}
}

class NODE{
	var $name;
	var $attrs=[];
	var $styles=[];
	var $parent;
	var $nodes=[];
	var $content="";
	var $single=false;
	var $idx = 0;
	private function __construct(){}
	static function create(){
		$num = func_num_args();
		if($num<1)return;
		$args = func_get_args();
		$node = new NODE();
		$node->idx = DOM::nextIdx();
		for ($i = 0; $i < $num; $i++) {
			if($i==0) $node->name = $args[$i];
			else if(is_string($args[$i])){//innerHTML
				$node->content = $args[$i];
			}else if(is_hash($args[$i])){//attr
				$node->attr($args[$i]);
			}else if(is_array($args[$i])){//subs
				foreach($args[$i] as $arg)
					$node->append($arg);
			}else if(is_bool($args[$i])){//single tag or double tag
				$node->single = $args[$i];
			}else if(get_class($args[$i])=="NODE"){//element -> parent
				$args[$i]->append($node);
			}
		}
		DOM::add($node);
		return $node;
	}
	function append($dom){$dom->appendTo($this);}
	function appendTo($dom){
		if($this->parent){
			$nodes = [];
			foreach ($this->parent->nodes as $node){
				if($node->idx!=$this->idx) $nodes[]=$node;
			}
			$this->parent->nodes = $nodes;
		}
		$this->parent=$dom;
		$dom->nodes[]=$this;
	}
	function attr($attrs){
		$num = func_num_args();
		if($num==1 && is_hash($attrs)){
			foreach ($attrs as $k => $v)
			if($k=="style") $this->css($v);
			else $this->attrs[$k] = $v;
		}else if($num==2){
			$args = func_get_args();
			$this->attrs[$args[0]] = $args[1];
		}
		return $this;
	}
	function css($styles){
		$num = func_num_args();
		if($num==1){
			if(is_string($styles))
				parse_str(preg_replace(['/;$/','/;/','/:/'],['','&','='],$styles),$styles);
			if(is_hash($styles))
			foreach ($styles as $k => $v)
				$this->styles[$k] = str_replace(';','',$v);
		}else if($num==2){
			$args = func_get_args();
			$this->styles[$args[0]] = $args[1];
		}
		return $this;
	}
	public function __toString(){
		$name = $this->name;
		$attrs = "";
		foreach ($this->attrs as $k => $v)
			$attrs.=$k.'="'.$v.'" ';
		if(!empty($this->styles))
			$attrs.= 'style="'.preg_replace(['/&/','/=/','/\+/'],[';',':',' '],http_build_query($this->styles)).'"';
		$content = $this->content;
		foreach ($this->nodes as $node)
			$content.=$node;
		return $this->single?"<$name $attrs/>":"<$name $attrs>$content</$name>";
	}
}
foreach (DOM::$single_tags as $tag)
	eval('function _'.$tag.'(){$args = func_get_args();array_push($args,true);array_unshift($args,"'.$tag.'");return call_user_func_array(["NODE","create"],$args);}');
foreach (DOM::$double_tags as $tag)
	eval('function _'.$tag.'(){$args = func_get_args();array_push($args,false);array_unshift($args,"'.$tag.'");return call_user_func_array(["NODE","create"],$args);}');

