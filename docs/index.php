<?php

/**
 * TODO add anchor if there are many classes
 * TODO quick guide of liberPHP
 * TODO quick guide of PHPDoc syntax...
 * 
 * */

$FILES=null;

function parse_file($file){
	$src = file_get_contents($file);
	//file docs
	preg_match_all('/<\?php\s*\/\*+\s*(?P<comment>.*?)\*\/\s*/sm', $src, $fdef);
	$fdoc = $fdef["comment"][0];
	$fds = array_map(function($e){return preg_replace("/^\s*\**\s*/","",$e);}, explode("\n",$fdoc));
	$fdoc = join("\n",$fds);
	$src = preg_replace('/<\?php\s*\/\*+\s*(?P<comment>.*?)\*\/\s*/sm',"",$src);
	
	$segs = preg_split("/\s+class\s+/",$src);
	$classes = [];
	$segIdx = 0;
	//var_dump(count($segs));
	foreach($segs as $seg){
		$methods = [];
		$clsName = null;
		
		if($segIdx>0){
			//find classes
			preg_match_all('/class\s+(?P<name>[_a-zA-Z0-9]+)\s*(?P<ext>[_a-zA-Z0-9\s]*)\{(?P<body>.*)\}/sm', "class ".$seg, $cdef);
			if(!empty($cdef["name"])){
				$clsName = $cdef["name"][0];
				$cls =  [
					"name" => $clsName,
					"ext" =>  $cdef["ext"][0]
				];
				if(!empty($cls["ext"])){
					preg_match_all('/implements\s+(?P<cls>[_a-zA-Z0-9]+)\s*/', $cls["ext"], $itfs);
					preg_match_all('/extends\s+(?P<cls>[_a-zA-Z0-9]+)\s*/', $cls["ext"], $prts);
					if(!empty($itfs["cls"])){
						$cls["interface"] = ((strpos($itfs["cls"][0], ',') !== FALSE))?explode(",",$itfs["cls"][0]):[$itfs["cls"][0]];
					}
					if(!empty($prts["cls"]))$cls["parent"]=$prts["cls"][0];
				}
				$classes[] =$cls;
				preg_match_all("/\s*function\s+(?P<mname>[_a-zA-Z0-9]+)\s*\(/s",$cdef["body"][0],$mdef);
				//var_dump($mdef["mname"]);echo "<br>";
				if(!empty($mdef["mname"])){
					foreach($mdef["mname"] as $mname){
						$methods[]=$mname;
					}
				}
			}
		}
		
		//funcs with comment
		preg_match_all('/\/\*\*\s*(?P<comment>.*?)\*\/\s+(?P<sec1>(public|private|protected|static)*)\s*(?P<sec2>(public|private|protected|static)*)\s*function\s+(?P<name>[_a-zA-Z0-9]+)\s*\((?P<params>.*?)\)/s', $seg, $def);
		$idx = 0;
		for($i=0;$i<count($def["name"]);$i++){
			$f = [
				"name" => $def["name"][$i],
				"params_str" => $def["params"][$i],
				"comment" => $def["comment"][$i],
				"attr1"=>$def["sec1"][$i], "attr2"=>$def["sec2"][$i]
			];
			if(isset($clsName) && in_array($def["name"][$i], $methods)) $f["class"] = $clsName;
			$f = parse_comm($def["comment"][$i],$f);
			$funcs[$def["name"][$i]]=$f;
		}
		//funcs no comment
		preg_match_all('/(?P<sec1>(public|private|protected|static)*)\s*(?P<sec2>(public|private|protected|static)*)\s*function\s+(?P<name>[_a-zA-Z0-9]+)\((?P<params>.*)\)/', $seg, $ndef);
		for($i=0;$i<count($ndef["name"]);$i++){
			$fn = $ndef["name"][$i];
			$f = ["name" => $fn,"params_str" => $ndef["params"][$i], "attr1"=>$ndef["sec1"][$i], "attr2"=>$ndef["sec2"][$i]];
			//echo $fn."::".$ndef["sec1"][$i]." -- ".$ndef["sec2"][$i]."<br>";
			if(isset($clsName) && in_array($fn, $methods)) $f["class"] = $clsName;
			if(!isset($funcs[$fn]))
				$funcs[$fn]=$f;
		}
		$segIdx++;
	}
	return ["file"=>$fdoc, "funcs"=>$funcs, "classes"=>$classes];
}

function parse_comm($comm,&$d){
	$comm = explode("\n",$comm);
	//echo $d["name"]."<br>";
	$d = isset($d)?$d:[];
	$d["desc"] = "";
	$d["params"] = [];
	$d["example"] = [];
	$lastTag=null;$lastTagIdx=0;$lastField=null;
	foreach($comm as $cm){
		$cm = trim(preg_replace('/^\s*\**\s*/',"",$cm));
		//var_dump($comm);
		//echo "$cm<hr>";
		preg_match_all('/@(?P<tag>[a-zA-Z]+)[\s:]+(?P<value>.*)/i',$cm,$matches);
		if(!empty($matches["tag"])){
			$tag = trim(strtolower($matches["tag"][0]));
			$value = trim($matches["value"][0]);
			$lastTagIdx = $tag==$lastTag?$lastTagIdx+1:0;
			$lastTag = null;
			switch($tag){
				case "param":
					$ps = preg_split("/\s*:\s*/",$value);
					$d["params"][$ps[0]] = $ps[1];
					$lastField = $ps[0];
					break;
				case "example":
					$d[$tag][$lastTagIdx] = !empty($value) ? preg_replace("/^\s*:\s*/","",$value)."\n":"";
					break;
				default:
					$d[$tag] = $value;
					break;
			}
			$lastTag = $tag;
		}else if($cm!=""){
			if($lastTag=="example"){
				$d[$lastTag][$lastTagIdx] .= $cm."\n";
			}else if($lastTag=="param"){
				$d["params"][$lastField] .= "\n".$cm;
			}else{
				$d["desc"].=$cm."\n";
			}
		}
	}
	return $d;
}

function parse_dir($path){
	exec ("find $path | grep .inc | grep -v .svn | grep -v .git ", $fs);
	$files = [];
	foreach($fs as $f){
		$f = preg_replace("/^\//","",str_replace($path,"",$f));
		$files[]=$f;
	}
	global $FILES;
	$FILES = $files;
	return $files;
}

function render_tree($path){
	$files = parse_dir($path);
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

function render_content($d){
	global $FILES;
	$html = "<h2>".str_replace("/","::",$_GET["f"])."</h2>";
	
	$html.= "<input list='files' id='search' autocomplete='on'/><datalist id='files'>".join("",array_map(function($e){return "<option value='$e'>";},$FILES))."</datalist>";
	list($fdoc,$funcs,$classes) = [$d["file"], $d["funcs"], $d["classes"]];
	$fdoc = preg_replace(["/\n/","/\t/"],["<br>","&nbsp;&nbsp;&nbsp;&nbsp;"],htmlspecialchars($fdoc));
	$html.= "<section>".$fdoc."<section><hr>";
	
	foreach($classes as $cls){
		$extstr = isset($cls["parent"])?"&nbsp;extends&nbsp;<a href='?f=".$cls["parent"]."'>".$cls["parent"]."</a>&nbsp;":" ";
		$intstr = isset($cls["interface"])? join("&nbsp;,&nbsp;",array_map(function($e){return "&nbsp;implements&nbsp;<a href='?f=$e'>$e</a>";},$cls["interface"])) : ""; 
		$html.= "<section class='cls'>";
		$html.= "<div><b>class</b><dfn>".$cls["name"]."</dfn><var>$extstr $intstr</var></li>";
		$html.= "</section>";
	}
	
	foreach($funcs as $fn=>$f){
		$fstyle = !isset($f["comment"])? "class='nodoc'":"";
		$fclass = isset($f["class"])? $f["class"]."::":"";
		$fattrStyle = trim($f["attr1"]." ".$f["attr2"]);
		if($fattrStyle=="")$fattrStyle="public";
		$html.= "<section class='func'>";
		$html.= "<div class='$fattrStyle'><b>function</b><dfn $fstyle>$fclass$fn</dfn>(".htmlspecialchars($f["params_str"]).")</div>";
		$html.= "<ul><li><label>@desc : </label><var>".str_replace("\n","<br>",htmlspecialchars($f["desc"]))."</var></li>";
		$html.= "<li><label>@author : </label><var>".htmlspecialchars($f["author"])."</var></li>";
		if(!empty($f["params"])){
			foreach($f["params"] as $p=>$v)
				$html.= "<li><label>@param $p : </label><var>".str_replace("\n","<br>",htmlspecialchars($v))."</var></li>";
		}
		$html.= "<li><label>@return : </label><var>".htmlspecialchars($f["return"])."</var></li>";
		if(!empty($f["example"])){
			foreach($f["example"] as $exp)
				$html.= "<li><label>@example:</label><br><code>".preg_replace(['/\n/','/\t/'],["<br>","<samp></samp>"],htmlspecialchars($exp))."</code></li>";
		}
		$html.= "</ul>";
		$html.= "</section>";
	}
	return $html;
}

function search_class_file($cls){
	global $FILES;
	foreach($FILES as $df){
		//echo $df.", ".$cls."<br>";
		if($df==$cls.".inc" || (strpos($df, "/".$cls.".inc") !== FALSE)){
			return $df;
		}
	}
	return false;
}

function render($path, $f=null){
	$html = <<<EOT
<!DOCTYPE html>
<html>
	<head>
		<title>LiberPHP2 Document</title>
		<meta http-equiv="X-UA-Compatible" content="IE=edge"> 
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
		<meta name="description" content="LiberPHP2 Document Preference">
		<link rel="stylesheet" href="css/docs.css" type="text/css" />
		<script></script>
		<script src="http://liberjs.org/liber.js"></script>
		<script src="js/docs.js"></script>
	</head>
	<body>
		<div id="wrapper">
		<nav>
			<h1>LiberPHP2</h1>
			<a href="index.html">About</a>
			<a href="tutorial.html">Tutorial</a>
			<a href="">Document</a>
		</nav>
		___TREE___
		<article>
		___CONTENT___
			<footer> @Author : soyoes 2014</footer>
		</article>
		</div>
	</body>
</html>
EOT;
	$html = str_replace("___TREE___",render_tree($path),$html);
	$content = (isset($f))? render_content(parse_file($path."/".((strpos($f, '.') == FALSE) ? search_class_file($f):$f))) : "";
	echo str_replace("___CONTENT___",$content,$html);
	exit;
}

//$d = parse_file('./modules/DB.inc');
//$d = parse_file('../modules/DB.inc');
render("../modules/", $_GET["f"]);

