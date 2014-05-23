<?php
class Docs{
	
	static $files = [];

	
	static function render_file($filename){
		$tree = fs_src_tree($filename);
		$out = ["<div class='file'>"];
		if(!empty($tree['annotations']))
			$out[]=self::render_annotations($tree['annotations']);
		$li = $tree['classes'];
		if(!empty($li))
			foreach ($li as $c)
				$out[]=self::render_class($c);
		$li = $tree['functions'];
		if(!empty($li))
			foreach ($li as $f)
				$out[]=self::render_function($f);
		$out[]="</div>";
		return join('',$out);	
	}
	
	private static function render_annotations($anno){
		$out = [];
		if(!empty($anno))
			$out[]="<table class='comment'>";
		foreach ($anno as $k => $v) {
			
			if(empty($v))continue;
			$style = ($k=="example")?"example":"";
			$out[]="<tr><th>$k</th><td class='$style'>";
			if(is_array($v)){
				foreach ($v as $el) {
					//error_log(json_encode($el));
					$attr = $el["attr"];
					$vstr = str_replace('\n','<br>',preg_replace(['/</','/>/'],['&lt;','&gt;'],$el['value']));
					if($k=="param"&&!empty($attr))
						$out[]="<div><label class='param'>$attr :</label><span>$vstr</span></div>";
					else if($k=="example")
						$out[]="<code>$vstr</code>";
					else 
						$out[]="<span>$vstr</span>";
				}
			}else {
				$v = str_replace('\n','<br>',preg_replace(['/</','/>/'],['&lt;','&gt;'],$v));
				$out[]=$k=='example'?'<code>'.$v.'</code>':$v;
			}
			$out[]="</td></tr>";
		}
		$out[]="</table>";
		return join('',$out);
	}
	
	private static function render_class($cls){
		$out = [];
		if(empty($cls))return;
		$name = $cls['name']; 
		$out []= '<section class="cls">';
		
		$abstr = empty($cls["abstract"]) ? '':'<b>abstract</b> ';
		$trait = empty($cls["trait"])||$cls["trait"]===false ? "<b>class<b> $name ":"<b>trait</b> $name ";
		$parent = empty($cls["parent"])? '':"<b>extends</b> <a href='?f=".$cls["parent"]."'>".$cls["parent"]."</a>";
		$def = $abstr.$trait.$clsName.$parent;
		if(!empty($cls["interfaces"])){
			$intrs = explode(',',$cls['interfaces']);
			$def.=' <b>implements</b> ';
			$intrsStr = "";
			foreach ($intrs  as $intr){
				$intrsStr []= '<a href="?f='.$intr.'">'.$intr.'</a>';
			}
			$def.=join(', ', $intrsStr);
		}
		$out []= '<h2>'.$def.'</h2>';
		
		if($cls['annotations']){
			$out[]=self::render_annotations($cls['annotations']);
		}
		
		//properties
		if(!empty($cls["properties"])){
			$out[]='<table class="properties"><thead><tr><td colspan="3"><h3>Properties</h3></td></tr><tr><th>Name</th><th>Visibility</th><th>Static</th></tr></thead>';
			foreach ($cls["properties"] as $prop=>$attr){
				$out[]="<tr><th class='".($attr["static"]?"static":"")."'>$prop</th><td class='visibility ".$attr["visibility"]."'>".$attr["visibility"]."</td><td>".($attr["static"]?"YES":"")."</td></tr>";
			}
			$out[]='</table>';
		}
		
		//methods
		if(!empty($cls['methods'])){
			foreach ($cls['methods'] as $prop=>$attr)
				$out[]=self::render_function($attr);
		}
		$out []= '</section>';
		
		return join('',$out);
	}
	
	private static function render_function($func){
		$out = [];
		$isMember = !empty($func["classname"]);
		$cn = !$isMember ? 'func ':'func method '; 
		$fn = !$isMember ? $func["name"]:$func["classname"]."::".$func["name"];
		$sta = $isMember&&$func['static']? 'static ':'';
		$cn.=$sta.$func["visibility"];
		$fk = !$isMember ? '<b>function</b>' : "<b>$sta".$func["visibility"]." function</b>";
		$params = join(', ',$func['params']); 
		$out[]="<div class='$cn'><h3>$fk <dfn>$fn</dfn>(<span>$params</span>)</h3>";
		if(!empty($func["annotations"]))
			$out []= self::render_annotations($func["annotations"]);
		$out []= "</div>";
		return join('',$out);
	}
	
	public static function render_folder($path){
		$files = self::parse_dir($path);
		$html = "<dl>";
		$lastT = null;
		foreach($files as $f){
			$ps = explode("/",$f);
			$lstyle = (isset($_GET["f"])&&$f==$_GET["f"]) ? "on":"";
			if(preg_match("/.inc$/",$ps[0])){
				$html.="<dt class='$lstyle'><a href='?f=$f'>$f</a></dt>";
			}else{
				if($lastT!=$ps[0]){
					$html.="<dt>$ps[0]</dt>";
					$lastT = $ps[0];
				}
				$html.="<dd class='$lstyle'><a href='?f=$f'>$ps[1]</a></dd>";
			}
		}
		$html.="</dl>";
		return $html;
	}
	
	private static function search_class_file($cls){
		foreach(self::$files as $df){
			if($df==$cls.".inc" || (strpos($df, "/".$cls.".inc") !== FALSE)){
				return $df;
			}
		}
		return false;
	}
	
	
	public static function parse_dir($path){
		exec ("find $path | grep .inc | grep -v .svn | grep -v .git ", $fs);
		$files = [];
		foreach($fs as $f){
			$f = preg_replace("/^\//","",str_replace($path,"",$f));
			$files[]=$f;
		}
		self::$files = $files;
		return $files;
	}
	
	
	public static function render($src, $path, $f=null){
		$html = str_replace("___TREE___",self::render_folder($path),$src);
		$content = (isset($f))? self::render_file($path."/".((strpos($f, '.') == FALSE) ? self::search_class_file($f):$f)) : "";
		echo str_replace("___CONTENT___",$content,$html);
		exit;
	}
	
	
}
