<?php


/**
Query clause:
k=a
k!=a
k<a
k>a
k<=a
k>=a
k(a,b)			//between
k!(a,b)			//not between
k[a,b,c...]		//in
k![a,b,c...]	//not in
k/keyword/		//like
k!/keyword$/	//not like
date().!(:date1,:date2)
sql func
	YEAR()
	MONTH()
	DAY()
	NOW()
	DATE()
	TIME()
	UNIX_TIMESTAMP()
	GROUP_CONCAT()
	CONCAT()


@example w[where]:
q = (a>1&b>2)|(a<1&b>c)
q = a.c1<1&b.cc2>c&a.c2(2014-10-01,2014-11-01)&date()>a.c3

@example s[select]
c = a.id,b.id[bid],c.name[cname],count(1)[cnt] # a.id,b.id 'bid',c.name 'cname',count(1) 'cnt'

@example g[group]
g = a.id 	# group by a.id

@example o[order]
o = a.name!,b.id 	# order by a.name desc, b.id asc

 */





/**
 * parse ql to sql, and return db resonpse
 * @param  hash $opt 
 *         string f    [SQL FROM CLAUSE]
 *         string s    [SQL SELECT CLAUSE]
 * 		   string w    [SQL WHERE CLAUSE]
 * 		   string g    [SQL GROUP BY CLAUSE]
 * 		   string o    [SQL ORDER BY CLAUSE]
 * 		   string l    [SQL LIMIT CLAUSE]
 * @param  array  $data [SQL statement data]
 * @return array result or error
 * 
 */
function ql_query($opt,$data=[]){
	$f = !empty($opt['f'])?ql_parse_f($opt['f']):'';
	$s = !empty($opt['s'])?ql_parse_s($opt['s']):'*';
	$o = !empty($opt['o'])?ql_parse_o($opt['o']):'';
	$g = !empty($opt['g'])?ql_parse_g($opt['g']):'';
	$w = $opt['w'];
	$l = !empty($opt['l'])?$opt['l']:'';
	if(empty($f))return false;
	if(empty($w) && !empty($data)){
		$nd = [];
		$w = ql_parse_w(ql_build_w($od,$nd));
		$data = $nd;
	}else{
		$w = ql_parse_w(ql_filter(preg_replace(['/#.*/','/[\t\s]+/'],'',$w),$data));
		//error_log("data=".json_encode($data));exit;
	}
	$sql = "SELECT $s FROM $f ".
		(empty($w)?'':"WHERE $w ").
		(empty($g)?'':"GROUP BY $g ").
		(empty($o)?'':"ORDER BY $o ").
		(empty($l)?'':"LIMIT $l");
	try{
		$res = db_query($sql, $data);
		return $res;
	}catch(Exception $e){
		return [
			'error' => $e->getMessage(),
			'sql' => $sql
		];
	}
}

/**
 * remove undefined fields from [w]here clause, with user parameters
 * @param  [str]  $w 	  : remove fields which are not exist in data from ql where clause
 * @param  [hash] $data   : param list from client
 * @return [str]  where clause for ql
 */
function ql_filter($w, &$data){
	preg_match_all('/:(?<v>[a-z0-9_]+)/i',$w,$mvs);
	$allvs = $mvs['v'];
	$ks = array_map(function($e){return explode('@', $e)[0];},array_keys($data));
	//solve =|!=|<|<=|>|>= expressions
	preg_match_all('/(?<pre>[&|]*)[a-z0-9_\.]+[=<>!]+:(?<v>[a-z]+[a-z0-9_]+)(?<sur>[&|]*)/i', $w, $m);
	if(!empty($m['v'])){
		$i=0;
		foreach ($m['v'] as $k) {
			$pre=preg_match('/[&|]/',$m['pre'][$i])?'\\'.$m['pre'][$i]:'';
			$sur=empty($pre)&&preg_match('/[&|]/',$m['sur'][$i])?'\\'.$m['sur'][$i]:'';
			if(!in_array($k, $ks))
				$w = preg_replace('/'.$pre.'[a-z0-9_.]+[=<>!]+:'.$k.$sur.'/i', '', $w);
			$i++;
		}
	}
	//error_log("w1=".$w);
	
	//solve single var inside ()/[] between/in
	preg_match_all('/(?<pre>[&|]*)(?<name>[a-z0-9_\.]+)(?<n>!*)[\(\[]:(?<a>[a-z0-9_]+)[\]\)](?<sur>[&|]*)/i', $w, $m);
	if(!empty($m['a'])){
		$i = 0;
		foreach ($m['a'] as $a) {
			// error_log("a=$a");
			// error_log("name=".$m['name'][$i]);
			// error_log("n=".$m['n'][$i]);
			if(!empty($data[$a])){
				//split arg to sequenced idx
				$vs = explode(',',$data[$a]);
				$j = 0;
				$rep = [];//replacement str
				foreach ($vs as $v){
					$data[$a.'_'.$j]=$v;
					$rep[]=':'.$a.'_'.$j;
					$allvs[]=$a.'_'.$j++;
				}
				$rep = join(',',$rep);
				// error_log("rep=$rep");
				unset($data[$a]);
				$ks = array_map(function($e){return explode('@', $e)[0];},array_keys($data));
				unset($allvs[array_search($a, $allvs)]);

				//replace regexp from single var to multi vars
				$w = preg_replace('/('.$m['name'][$i].$m['n'][$i].')([\(\[]):(?<a>[a-z0-9_]+)([\]\)])/i','$1$2'.$rep.'$4',$w);
			}else{//remove this part
				$pre=preg_match('/[&|]/',$m['pre'][$i])?'\\'.$m['pre'][$i]:'';// &| surround name
				$sur=empty($pre)&&preg_match('/[&|]/',$m['sur'][$i])?'\\'.$m['sur'][$i]:'';// &| surround name
				$w = preg_replace('/'.$pre.'[a-z0-9_\.]+!*[\(\[]'.$a.'[\]\)]'.$sur.'/i', '', $w);
			}
			$i++;
		}
	}
	// error_log("w2=".$w);


	//solve (not)between,(not)in
	preg_match_all('/(?<pre>[&|]*)[a-z0-9_\.]+!*[\(\[](?<v>[:a-z0-9_\.,]+)[\]\)](?<sur>[&|]*)/i', $w, $m);
	if(!empty($m['v'])){
		$i=0;
		foreach ($m['v'] as $v) {
			$pre=preg_match('/[&|]/',$m['pre'][$i])?'\\'.$m['pre'][$i]:'';// &| surround name
			$sur=empty($pre)&&preg_match('/[&|]/',$m['sur'][$i])?'\\'.$m['sur'][$i]:'';// &| surround name
			$vs = array_filter(explode(',', $v),function($e){return $e[0]==':';});
			if(empty($vs))continue;
			$vs = array_map(function($e){return substr($e, 1);}, $vs);
			if(count(array_intersect($vs, $ks)) != count($vs))//data.fields does not contains all vars
				$w = preg_replace('/'.$pre.'[a-z0-9_\.]+!*[\(\[]'.$v.'[\]\)]'.$sur.'/i', '', $w);
			$i++;
		}
	}
	// error_log("w3=".$w);


	//solve like
	preg_match_all('/(?<pre>[&|]*)[a-z0-9_\.]+!*\/:(?<v>[^\/]+)\/(?<sur>[&|]*)/i', $w, $m);
	if(!empty($m['v'])){
		$i=0;
		foreach ($m['v'] as $k) {
			$pre=preg_match('/[&|]/',$m['pre'][$i])?'\\'.$m['pre'][$i]:'';
			$sur=empty($pre)&&preg_match('/[&|]/',$m['sur'][$i])?'\\'.$m['sur'][$i]:'';
			if(!in_array($k, $ks))
				$w = preg_replace('/'.$pre.'[a-z0-9_\.]+!*\/:'.$k.'\/'.$sur.'/i', '', $w);
			$i++;
		}
	}

	//solve regexp
	preg_match_all('/(?<pre>[&|]*)[a-z0-9_\.]+!*\{:(?<v>[^\}]+)\}(?<sur>[&|]*)/i', $w, $m);
	if(!empty($m['v'])){
		$i=0;
		foreach ($m['v'] as $k) {
			$pre=preg_match('/[&|]/',$m['pre'][$i])?'\\'.$m['pre'][$i]:'';
			$sur=empty($pre)&&preg_match('/[&|]/',$m['sur'][$i])?'\\'.$m['sur'][$i]:'';
			if(!in_array($k, $ks))
				$w = preg_replace('/'.$pre.'[a-z0-9_\.]+!*\{:'.$k.'\}'.$sur.'/i', '', $w);
			$i++;
		}
	}
	// error_log("w4=".$w);

	//remove empty braces and useless &|
	$w = preg_replace(['/\([&|]/','/[&|]\)/','/^[&|]/','/[&|]$/'], ['(',')','',''], $w);
	preg_match_all('/(?<pre>[&|]*)(?<bra>\(\))(?<sur>[&|]*)/i', $w, $m);
	if(!empty($m['bra'])){
		$i=0;
		foreach ($m['bra'] as $k) {
			$pre=preg_match('/[&|]/',$m['pre'][$i])?'\\'.$m['pre'][$i]:'';
			$sur=empty($pre)&&preg_match('/[&|]/',$m['sur'][$i])?'\\'.$m['sur'][$i]:'';
			$w = preg_replace('/'.$pre.'\(\)'.$sur.'/i', '', $w);
			$i++;
		}
	}
	//error_log("w5=".$w);

	//remove useless data from data
	$aks = array_filter($ks, function($k) use($allvs,&$data){if(in_array($k, $allvs))return true;else unset($data[$k]);return false;});


	// error_log("w6=".$w);
	// exit;
	return $w;
}

/**
 * convert user parameters(assoc-array) to ql [w]here clause.
 * @param  [type] $o [description]
 * @return ql
 * @example 1th
 	[
		{a:1,b:2},
		{a:1,b@gt:10}
 	]
 	=> (a=1&b=2)|(a=1&b>10)

 * @example 2nd
	{a:1,b@l:abc,c@in:1,3}
	=> (a=1&b/abc/&c[1,3])

 */
function ql_build_w($o, &$data){
	$qns = [
		'eq' 	=> '=$',
		'ne' 	=> '!=$',
		'lt' 	=> '<$',
		'gt'	=> '>$',
		'le' 	=> '<=$',
		'ge'	=> '>=$',
		'in'	=> '[$]',
		'nin' 	=> '![$]',
		'bt' 	=> '($)',
		'nb' 	=> '!($)',
		'l' 	=> '/$/',
		'nl' 	=> '!/$/',
		'm' 	=> '{$}',
		'nm' 	=> '!{$}',
	];
	$ws = [];
	if(is_hash($o)){
		foreach ($o as $k=>$v){
			list($c, $r)=explode('@', $k);
			$cn = str_replace('.', '_', $c);
			$i=0;
			while(isset($data[$cn])) //a key may appears more than 1 time
				$cn.="_".(++$i);
			$data[$cn] = $v;
			$ws []= !empty($r)? $c.str_replace('$', ":$cn", $qns[$r]):"$c=:$cn";
		}
		return implode('&', $ws);
	}else if(is_array($o)){
		foreach ($o as $e){
			$w=ql_build_w($e,$data);
			if(!empty($w))
				$ws []= '('.$w.')';
		}
		return implode('|', $ws);
	}
	return '';
}


/**
 * build SQL WHERE clause,with ql [w]here clause
 * @param  [string] $q
 * @return a piece of $sql
 * @example
	$w = "a.c1<1&b.cc2>c&a.c2!(2014-10-01,2014-11-01)&d![11,2,34]&date()/^abc$/&e/def/";
	//result : a.`c1`<1 AND b.`cc2`>'c' AND (a.`c2` NOT BETWEEN '2014-10-01' AND '2014-11-01') AND d NOT IN ('11','2','34') AND date() LIKE 'abc' AND e LIKE '%def%'
	$w = "a.c1<:c1&b.cc2>:c2&a.c2!(:v3,:v4)&d![:v5]&e/:def/&f/:abc$/&k=null";
	//result : a.`c1`<:c1 AND b.`cc2`>:c2 AND (a.`c2` NOT BETWEEN :v3 AND :v4) AND d NOT IN (:v5) AND e LIKE :def AND f LIKE '%:abc' AND k IS NULL
	$w = "a.c1<:c1&b.cc2>:c2|a.c2!(:v3,:v4)";
	//result : a.`c1`<:c1 AND b.`cc2`>:c2 OR (a.`c2` NOT BETWEEN :v3 AND :v4)
	$sql = ql_parse_w($w);
	echo $sql."<br>";
 */
function ql_parse_w($q){
	$sql = preg_replace([
			'/&&/','/\|\|/',//binary & |
			'/&/','/\|/',//and or
			'/\!=null/', //is not null
			'/=null/', //is null
			'/([\da-z_\.]+)(\!*)\((:[^,]+),(:[^\)]+)\)/i', //between with statement 2 vars
			'/([\da-z_\.]+)(\!*)\(([^,]+),([^\)]+)\)/i', //between
			'/([\da-z_\.]+)(\!*)\(:([0-9a-z_]+)\)/i', //between with statement 1 var
			'/(\!*)\[([^\]]+)\]/', //in
			'/(\!*)\/(:[^\/\^\$]+)\//', //like with variable
			'/(\!*)\/([^\/]+)\//', //like
			'/%\^/', //like ^
			'/\$%/', //like $
			'/(\!*)\{(:[^\}\^\$]+)\}/', //regexp with variable
			'/(\!*)\{([^\}]+)\}/', //regexp
			'/\!(BETWEEN|IN|LIKE)\s/', //not
			'/(?<=^|[=<>\s\(])([a-z0-9_]+)\.([a-z0-9_]+)(?=[=<>\s\)]*|$)/i',//wrapper on tablename.colname
			'/@binAND\s(:[a-zA-Z0-9\._]+)/','/@binOR\s(:[a-zA-Z0-9\._]+)/',
			],[
			' @binAND ',' @binOR ',//binary & |
			' AND ',' OR ',//and or
			' IS NOT NULL ', //is not null
			' IS NULL ', //is null
			' ($1 $2BETWEEN $3 AND $4) ', //between with statement variable
			' ($1 $2BETWEEN \'$3\' AND \'$4\') ', //between
			' ($1 $2BETWEEN :$3) ', //between with statement 1 var
			' $1IN ($2) ', //in
			' $1LIKE $2 ', //like
			' $1LIKE \'%$2%\' ', //like
			'', 				//like ^
			'', 				//like $
			'$1 REGEXP $2',//regexp var
			'$1 REGEXP \'$2\'',//regexp
			'NOT $1 ',//not
			'$1.`$2`',//wrapper on tablename.colname
			'& b\'$1\'','| b\'$1\'',
			],$q);
	//solve single quota of k=v
	$sql = preg_replace_callback('/(?<k>\b[a-z0-9_`]+)(?<o>\s*[><=]\s*)(?<v>[^\s]+)(?=\s|$)/',function($m){
		// echo "v=$m[v]<br>";
		// echo "r=".preg_match('/^[a-z\d`_]+\.[a-z\d`_]+$/i', $m['v'])."<br>";
		if(preg_match('/^[\d\.]+$/',$m['v']) || $m['v'][0]==':' || preg_match('/^[a-z\d`_]+\.[a-z\d`_]+$/i', $m['v'])){
			return $m['k'].$m['o'].$m['v']." ";
		}else {
			return $m['k'].$m['o']."'".$m['v']."' ";
		}
	}, $sql);

	//solve single quota in IN clause
	$sql = preg_replace_callback('/(?<=IN\s)\((?<v>[^:][^\)]+)/',function($m){
        return "('".implode("','", explode(',', $m['v']))."'";
    },$sql);

	return $sql;
}

/**
 * parse FROM table, fields string to sql string
 * @param string $t :
 * @return string
 * @example
	$f = "members[a]>(posts[b]){a.id=b.member_id&b.del_flg=0}";
	$sql = ql_parse_f($f);
	echo $sql."<br>";
	//result = `members` a LEFT JOIN `posts` b ON a.id=b.`member_id` AND b.`del_flg`=0
 */
function ql_parse_f($t){
	//extract subquery first
	$t = preg_replace_callback('/(?<=\{)([^\}]+)(?=\})/', function($m){
		return ql_parse_w($m[0]);
	}, $t);
	$sql = preg_replace([
		'/{/', // ON 
		'/([a-z_]+[\w\d_]*)\[([a-z_]+[\w\d_]*)\]/', //tablename alias
		//'/([:\.])(left|right|inner)\(/',//left|right|inner join
		'/>\(/',//left join
		'/<\(/',//right join
		'/\^\(/',//inner join
		'/[\)\}]/'//remove last 
		],[
		' ON ',
		'`$1` $2',//tablename alias
		' LEFT JOIN ',//left join
		' RIGHT JOIN ',//right join
		' INNER JOIN ',//inner join
		''
		], $t);
	return $sql;
}


/**
 * parse select columns clause
 * @param string $t :
 * @return string
 * @example
	s=m.id,m.name,p.title[pt],u.name[username],count(1)
 */
function ql_parse_s($t){
	$sql = preg_replace([
		'/([\.\(])([a-z_]+[\w\d_]*)\b([^\.])/i',
		'/\[([\w\d_]+)\]/i', //tablename alias
		],[
		'$1`$2`$3',
		' \'$1\'',
		], $t);
	return $sql;
}


/**
 * parse order !=>desc
 * @param string $t :
 * @return string
 * @example
	o=m.id! => m.id desc
 */
function ql_parse_o($t){
	$sql = preg_replace([
		'/\!/'
		],[
		' DESC',
		], $t);
	return $sql;
}

/**
 * parse group by
 * @param string $t :
 * @return string
 * @example
	g=m.id => m.`id`
 */
function ql_parse_g($t){
	$sql = preg_replace([
		// '/(?<=\.)([a-z\d_]+)/i',
		'/([a-z\d_]+)(?!\.)/i',
		],[
		// '`$1`',
		'`$1`',
		], $t);
	return $sql;
}




