<?php
/**
 *	@file: DB.inc	
 *	@author: Soyoes 2014/01/10
 *	@uses: db library
 *	@example: 
 *****************************************************************************/

if (!apc_load_constants("DB_FMTS")){
	apc_define_constants("DB_FMTS", [
	'FORMAT_CREATE_DB' 			 => "CREATE TABLE `%s`.`%s` ( %s PRIMARY KEY (`%s`)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_CREATE_DB_MULTI_KEY' => "CREATE TABLE `%s`.`%s` ( %s CONSTRAINT %s PRIMARY KEY (%s)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_INSERT_DB' 			 => "INSERT %s INTO `%s` (%s) VALUES(%s);",
	'FORMAT_UPDATE_DB'			 => "UPDATE `%s` SET %s WHERE `%s`=%s;"
	]);
}

/**
 * get connection handler of PDO
 * @param opts:use conf.ini as default
 * @return PDO object.
 * */
function db_conn($opts=null){
	$db = REQ::getDB();
	if(!isset($db)){
		$conn_str = Consts::$db_engine.":host=".Consts::$db_host.";port=".Consts::$db_port.";dbname=".Consts::$db_name.";charset=utf8";
		$db = new PDO($conn_str,Consts::$db_user,Consts::$db_pass,
				[PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
		REQ::setDB($db);
	}
	return $db;
}

/**
 * exec query SQL like select ...
 * @param $sql: sql statement with ?.
 * @param $datas: variables inside sql statement.
 * @param $useCache: whether save/load result from APC/Memcache
 * @param $PDOOpt: FETCH_COLUMN, FETCH_ASSOC ...
 * @return array or assoc array.
 * 
 * @example : 
 * 	db_query("select * from tasks");
 * @example : 
 * 	db_query("select * from tasks where `id`=:id",[id=>1]);
 * @example : 
 * 	db_query("select * from tasks where `id`=?",[1]);
 */
function db_query($sql, $datas=[], $useCache = false, $PDOOpt = PDO::FETCH_ASSOC) {
	try {
		$key = $sql;
		$isQeury = str_starts(strtolower($sql), "select");
		if ($useCache && $isQeury) {
			$key = $sql."-".json_encode($datas);
			$value = cache_get($key);
			if (isset ( $value ) && $value != false)
				return $value;
		}
		$db = db_conn();
		$statement = $db->prepare($sql);
		//var_dump($statement);
		if ($statement->execute ($datas) == FALSE) {
			return null;
		}
		$res =$isQeury? $statement->fetchAll($PDOOpt):true;
		if ($useCache && $isQeury && $res != null) {
			cache_set($key, $res);
		}
		return $res;
	} catch ( PDOException $e ) {
		error_log ("DB ERR :". $e->getMessage() );
		error_log ("DB ERR SQL:". $sql );
		return null;
	}
}

/**
 * return number result of "SELECT COUNT"
 */
function db_count($sql=null, $datas=[], $useCache=false){
	try {
		if($useCache){
			$value = cache_get($sql);
			if(isset($value) && $value!=false)
				return $value;
		}
		$db = db_conn();
		$sth = $db->prepare($sql);
		$sth->execute($datas);
		$res = $sth->fetchColumn();
		if($useCache && $res){
			cache_set($sql, $res);
		}
		return intval($res);
	} catch (PDOException $e) {
		error_log($sql);
		return -1;
	}
}

/**
 * change PDO settings 
 * @example : 
 * 	db_attr(PDO::ATTR_TIMEOUT,1000);
 * 
 * */
function db_attr($attr, $val){
	$db = db_conn();
	$db->setAttribute($attr, $val);
}

/**
 * query db with data options
 * @param $table : tablename
 * @param $opts : @see db_make_query
 * 		KEY|KEY@= 	= 	eq, equals
		KEY@!		= 	ne, not equals
		KEY@<		= 	lt, less than
		KEY@>		= 	gt, greater than
		KEY@<=		= 	le, less than or equals
		KEY@>=		= 	ge, greater than or equals
		KEY@[]		= 	in, sql in clause
		KEY@![]		= 	not in, sql not in clause
		KEY@()		= 	between
		KEY@!()		= 	not between
		KEY@?		= 	like, sql like '%YOUR_VALUE%'
		KEY@!?		= 	not like, sql not like '%YOUR_VALUE%'
		KEY@~		= 	regexp match, mysql | postgres
		KEY@!~		= 	regexp not match , mysql | postgres
		KEY@~~		= 	regexp match ignore case, mysql | postgres
		KEY@!~~		= 	regexp not match ignore case, mysql | postgres
 * 
 * @param $withCount: whether result contains total matched amount
 *  
 * @example : db_find("tasks", [
		"ownerId"	=> 3,
		"title@~"	=> "task",	//title regexp 'task'
		"fields" 	=> "id,title",
		"order"		=> "id desc",
		"limit"		=> 10
	]);
 * 
 */
function db_find($table, $opts=[], $withCount=false){
	list($colStr, $optStr,$datas) = db_make_query($table, $opts);
	$sql = "SELECT ".$colStr." FROM ".$table.$optStr;
	error_log("SEARCH: ".$sql);
	$res = db_query($sql, $datas, $opts["useCache"]);
	if($withCount){
		$sql = "SELECT count(*) FROM ".$table.preg_replace("/ORDER BY\s*(.*)\s*([LIMIT .*]*)/", '${3}',$optStr);
		$cnt = db_count($sql, $datas, $opts["useCache"]);
		return ["count"=>$cnt,"result"=>$res];
	}else{
		return $res;
	}
}
/**
 * search with data options, returns 1st matched record.
 * @param $table : tablename
 * @param $opts : @see db_make_query
 *
 * @example : db_find1st("tasks", [
 		"ownerId"	=> 3,
 		"title@~"	=> "task",	//title regexp 'task'
 	]);
 *
 */
function db_find1st($table, $opts=[]){
	$opts["limit"]=1;
	$res = db_find($table,$opts,false,$decodeBson);
	return isset($res)&&$res!=false ? $res[0]:false;
}

/**
 * insert many records to DB
 * @param $datas : hash list
 * @param table:tablename
 * @example:
 * 	db_import("tasks", [
		[
			"ownerId"	=> 3,
			"title"=> "taskD with bson",
			"data" => ["name"=>"ssss","age"=>18]
		],
		[
			"ownerId"	=> 4,
			"title"=> "taskE with bson",
			"data" => ["name"=>"ssss","age"=>18]
		]
	]);
 * 
 * */
function db_import($table, $datas){
	if(!isset($table) || count($datas)==0)
		return false;
	
	$schema = db_schema($table)["schema"];
	
	$cols = [];
	foreach ($datas as $d){
		$cols = array_unique(array_merge($cols,array_keys($d)));
	}
	
	$regName = Consts::$schema_reg;
	$updName = Consts::$schema_upd;
	
	$hasRegStamp = array_key_exists($regName,$schema);
	if($hasRegStamp && !in_array($regName, $cols)) $cols[] = $regName;

	$hasTimestamp = array_key_exists($updName,$schema);
	if($hasTimestamp && !in_array($updName, $cols)) $cols[] = $updName;

	$sql = "INSERT IGNORE INTO ".$table." (`".join("`,`", $cols)."`) VALUES ";
	$time = time();

	foreach ($datas as $d){
		if($hasRegStamp){$d[$regName]=$time;}
		if($hasTimestamp){$d[$updName]=$time;}
		$vals = [];
		foreach ($cols as $c){
			$v = array_key_exists($c, $d) ? $d[$c] : null;
			$vals[]=db_v($v, $schema[$c]);
		}
		$sql.=" (".join(",", $vals)."), ";
	}
	$sql = substr($sql, 0, strlen($sql)-2);
	db_attr(PDO::ATTR_TIMEOUT,1000);
	if(isset($sql_dump_path)){
		$handle = fopen($sql_dump_path, "w+");
		fwrite($handle, $sql);
	}else{
		//error_log("DB::import ".$sql);
		db_query($sql);
	}
}

/**
 * make SQL query from query condition array
 * @param $table:tablename 
 * @param $opts:array or string 
		if opts is Array : reserved keywords in opts  
 		* fields = default = * ||  id,pass,... || [id, pass, name ...] || count(*) 
 		* limit : 0/default=all, 0,20=select 0~20, 21~80= select from 21 get 60 records 
 		* order : default = "" || id desc || updAt desc 
 		* useCache : false | true 
 		* @id : $opts[PK]=$opts["@id"]
 * @param $omit : array, fields in opts to omit.
 * @return array or false
 */
function db_make_query($table, $opts=[], $omit=[]){
	/*Test cases
	 "id"=> 16
	"id@>"=>16,
	"id@<"=>16,
	"id@>="=>16,
	"id@<="=>16,
	"lng@[]"=>[1,3,55],
	"lng@![]"=>[],
	"lng@![]"=>[2,4],
	"lat@()"=>[153, 340],
	"lat@!()"=>[999, 20],
	"title@?"=>"cocoa",
	"title@!?"=>"co%coa",
	"title@~"=>"/^My/",
	"title@~~"=>"^My",
	"title@!~~"=>"'"
	**/
	if(empty(Consts::$db_query_filters))
		Consts::$db_query_filters = [
		"=" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`=:$k";},
		"!" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`!=:$k";},
		"<" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`<:$k";},
		">" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`>:$k";},
		"<=" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`<=:$k";},
		">=" 	=> function($k,$v,&$o){$o[$k]=$v;return "`$k`>=:$k";},
		"[]" 	=> function($k,$v,&$o){if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` IN (".join(",",$vs).")";},
		"![]" 	=> function($k,$v,&$o){if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` NOT IN (".join(",",$vs).")";},
		"()" 	=> function($k,$v,&$o){if(count($v)!=2)return false; return "(`$k` BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";},
		"!()" 	=> function($k,$v,&$o){if(count($v)!=2)return false; return "(`$k` NOT BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";},
		"?"  	=> function($k,$v,&$o){if(!str_has($v,"%"))$v="%$v%";return "`$k` LIKE '".$v."'";},
		"!?"  	=> function($k,$v,&$o){if(!str_has($v,"%"))$v="%$v%";return "`$k` NOT LIKE '".$v."'";},
		"~" 	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "`$k` $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
		//Regexp only mysql or regexp
		/*	@example db_find("users", ["title@~" => "^My"]) == select * from users where title regexp '^My'
		 For MySQL : these are available
		^ , $ , . , [...] , [^...] , p1|p2|p3 , * , + , {n} , {m,n}
		*/
		"!~"	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "`$k` NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
		"~~"	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "LOWER(`$k`) $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
		"!~~"	=> function($k,$v,&$o){$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "LOWER(`$k`) NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
		//Regexp with ignorecase option only mysql or regexp
	];
	if(!isset($table))return false;
	$colStr =  is_array($opts['fields'])? (count($opts['fields'])==0? "*" :join(",", $opts['fields']))
		 : (isset($opts['fields'])? $opts['fields']:'*');
	$optStr = "";
	$schemaDef = db_schema($table);
	$schema = $schemaDef["schema"];
	$pk = $schemaDef["general"]["pk"];
	$eg = Consts::$db_engine;
	$data = [];
	if(is_hash($opts)){
		foreach ($opts as $k => $v){
			if(in_array($k,$omit))continue;
			if($k=="@id")$k=$pk;
			if(is_callable($v)){ /*$k = key=value, $v = function($k,$v){return "$k > '$v'";} */
				list($key,$value)=explode("=", $k);
				$val = $v($key, $value);
				$optStr.= !empty($optStr) ? " AND $val ": " WHERE $val ";
			}else{
				list($k,$cmd) = explode("@",$k);
				if(array_key_exists($k, $schema)){
					$cmd = !isset($cmd)||$cmd=="" ? "=":$cmd;
					$cmd = strpbrk($cmd, 'begilmnqt') ? Consts::$query_filter_names[$cmd]:$cmd;
					error_log($cmd);
					$vStr =  Consts::$db_query_filters[$cmd]($k, $v, $data);
					if($vStr)
						$optStr.= !empty($optStr) ? " AND ". $vStr : " WHERE ".$vStr;
				}	
			}
		}
		if(!in_array("order",$omit) && !empty($opts["order"]))
			$optStr .= " ORDER BY ".$opts["order"];
		if(!in_array("limit",$omit) && !empty($opts["limit"]))
			$optStr .= " LIMIT ".$opts["limit"];
	}else {
		$optStr = !empty($opts)? " WHERE ". $opts : "";
	}
	return [$colStr,$optStr,$data];
}

function db_exists($table, $id){
	if(!isset($table) || !isset($id))
		return false;
	$pk = db_schema($table)["general"]["pk"];
	$entity = db_count("select count(*) from $table where `$pk`=:$pk",[$pk=>$id]);
	return $entity>0;
}

/**
 * @param $table:tablename
 * @param $opts:query options
 *
 * @example : db_update("tasks",[groupId=>3]); //delete from ... where groupId=3
 * */
function db_delete($table, $opts){
	if(empty($opts))return false;
	list($cs,$optStr,$data) = db_make_query($table, $opts,["order","limit","fields"]);
	$sql = "DELETE FROM $table ".$optStr;
	return db_query($sql,$data);
}

/**
 * @param $table:tablename
 * @param $data:hash, new value to update 
 * @param $opts:query options
 * 
 * @example : db_update("tasks",[groupId=>3],["id@>"=>38]); //set groupId=3 where id>38
 * */
function db_update($table, $data, $opts=[]){
	if(!isset($table) || empty($data) || !is_hash($data))
		return false;
	$vStrs = [];
	$schema = db_schema($table)["schema"];
	foreach($data as $k=>$v){
		$vStrs[]="`$k`=".db_v($v, $schema[$k]);
	} 
	$vStrs = join(",",$vStrs);
	list($cs,$optStr,$data) = db_make_query($table, $opts,["order","limit","fields"]);
	$sql = "UPDATE $table SET $vStrs ".$optStr;
	return db_query($sql,$data);
}

function db_migrate($table){
	$dbn = Consts::$db_name;
	$sql = sprintf("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '%s' AND table_name = '%s';", $dbn, $table);
	$res = db_count($sql,[], false);
	if ($res<=0 ){//schema doesn't exist
		$schema_def = db_schema($table);
		$schema = $schema_def["schema"];
		$pk = $schema_def["general"]["pk"];
		$colStmt = "";
		foreach ($schema as $col => $type){
			$colStmt .= "`".$col."` ".$type.", ";
		}
		$incStmt = "";
		$sql = "";
		if (str_has($pk, "|")){
			$parts = explode("|", $pk);
			$pkName = $parts[0];
			$keys = $parts[1];
			$sql = sprintf(FORMAT_CREATE_DB_MULTI_KEY, $dbn, $table, $colStmt, $pkName, $keys, $incStmt);
		}else{
			$sql = sprintf(FORMAT_CREATE_DB, $dbn, $table, $colStmt, $pk, $incStmt);
		}
		$res = db_query($sql);
	}
	echo "Created ".$table."</br>\n";
}


/**
 * insert / update a single record
 * @params $tabel:table name
 * @params $data: assoc array , one data record
 * 		insert => new record data, update => changes
 * @params $returnId : whether return new id.
 * @params $bson : compress json field to bson
 * @example :
 	db_save("tasks", [
		"ownerId"	=> 3,
		"title"=> "taskD with bson",
		"data" => ["name"=>"ssss","age"=>18]
	],true,true) //data will be saved as bson.
 * 
 * */
function db_save($table, $data, $returnId=false, $bson=false){
	if(!isset($table) || !is_hash($data) || empty($data))return false;
	$regName = Consts::$schema_reg;
	$updName = Consts::$schema_upd;
	
	$schema_def = db_schema($table);
	$schema = $schema_def["schema"];
	$pk = $schema_def["general"]["pk"];
	$id = isset($data[$pk]) ? $data[$pk] : null;
	if(array_key_exists($updName,$schema))
		$data[$updName] = Dates::format();
	$sql = "";
	$isUpdate = isset($id) && db_exists($table, $id);
	$qdatas = [];
	if ($isUpdate){	//update
		cache_del($table."_".$id);
		foreach ($data as $col => $val){
			if($col==$pk || !isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			$colStmt .= "`$col`=:$col ";
			$qdatas[$col]=db_v($val, $schema[$col], $bson);
		}
		$sql = sprintf(FORMAT_UPDATE_DB, $table, $colStmt, $pk, db_v($id));
	}else{						//insert
		if(array_key_exists($regName,$schema) && !isset($data[$regName]))
			$data[$regName] = Dates::format();
		foreach ($data as $col => $val){
			if(!isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			if(!empty($valStmt))$valStmt .= ",";
			$colStmt .= "`$col`";
			$valStmt .=":$col";
			$qdatas[$col]=db_v($val, $schema[$col], $bson);
		}
		//$ignore = $ignoreErrors ? "IGNORE" : "";
		$sql = sprintf(FORMAT_INSERT_DB, $ignore, $table, $colStmt, $valStmt);
	}
	try {
		if($returnId==true && !$isUpdate) {
			echo ($sql."\n");
			var_dump($qdatas);
			$res = db_trans([$sql, "SELECT LAST_INSERT_ID() as 'last_id'"],[$qdatas]);
			$data["id"] = $res[0]["last_id"];
		}else{
			db_query($sql,$qdatas);
		}
		return $data;
	} catch (PDOException $e) {
		error_log($sql);
		return false;
	}
}

/**
 * load schema configuration data from SCHEMA_DIR/$schemaName.ini using APC cache.
 * */
function db_schema($schemaName){
	$schemaDef = cache_get("SCHEMA_$schemaName", function($key){
		$schemaname = str_replace("SCHEMA_","",$key);
		$schemaDef = @parse_ini_file(APP_DIR.__SLASH__."conf".__SLASH__."schemas".__SLASH__.$schemaname.".ini", true);
		if(!$schemaDef){
			//error_log("schema=".LIBER_DIR."common/schemas/".$schemaname.".ini");
			$schemaDef = @parse_ini_file(LIBER_DIR."common/schemas/".str_replace("liber_","",$schemaname).".ini", true);
		}
		return $schemaDef;
	},false);
	return $schemaDef;
}

/**TODO test
 * @param $querys: function or array
 * 	@example function
 * 		db_trans(function($dbh){ //must return int;
 * 			$sth = $dbh->exec("DROP TABLE fruit");
 *			$sth = $dbh->exec("UPDATE dessert SET name = 'hamburger'");
 *			$dbh->rollBack();
 *			return 2; //MUST return int !!!
 * 		});
 * 	@example array
 * 		db_trans(["DROP TABLE fruit","UPDATE dessert SET name = 'hamburger'","@rollback"])
 * 
 * 		$rs = db_trans(["update tasks set members='dds' where id=35", "select * from tasks where id=35"]);
		echo json_encode($rs);
 * */
function db_trans($querys,$datas){
	if(!isset($querys))
		return false;
	$db = db_conn();
	$mod = $db->getAttribute(PDO::ATTR_ERRMODE);
	$db->setAttribute( PDO::ATTR_AUTOCOMMIT, 0 );
	$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	$db->beginTransaction();
	$cnt = 0;
	$res = true;
	try{
		if(is_callable($querys)){
			$cnt = $querys($db);
		}else if(is_array($querys)){
			$i=0;
			foreach($querys as $q){
				$i++;
				if($q==="@rollback"){
					$db->rollBack();$cnt--;
				}else{
					$statement = $db->prepare($q);
					$data = isset($datas[$i-1])?$datas[$i-1]:[];
					if ($statement->execute($data) == false) {
						continue;
					}
					if(str_starts(strtolower($q), "select")){
						$res = $statement->fetchAll(PDO::FETCH_ASSOC);//FIXME PDO option
					}
					$cnt++;
				}
			}
		}	
	}catch(Exception $e){
		error_log("DB Transaction ERR:",$e->getMessage());
		//while(0 < $cnt--){
		$db->rollBack();
		//}
	}
	if($cnt>0)
		$db->commit();
	$db->setAttribute( PDO::ATTR_AUTOCOMMIT, 1 );
	$db->setAttribute(PDO::ATTR_ERRMODE, $mod);
	return $res;
}
function bson_enc($arr){
	$str = json_encode($arr);
	$str = str_replace("\\", "", $str);
	return str2hex($str);
}

function bson_dec($bson){
	if(isset($bson)){
		$json = hex2str($bson);
		return json_decode($json,true);
	}
	return false;
}

/**
 * change php value to db value
 * @param $v : php value
 * @param $typeDef : type def in schemas/xxx.ini file. 
 * 			example : "int not null", "bigint", "varchar",  "text"....
 * @param $bsonText : compress text/mediumtext to BSON (binary json)
 * 
 * */
function db_v($v, $typeDef="", $bsonText=false){
	if(!isset($v))
		return "NULL";
	if(is_bool($v))
		return $v ? 1 : 0;
	if (is_array($v)){
		return $bsonText&&(isset($typeDef)&&preg_match("/text/i", $typeDef))? "'".bson_enc($v)."'"
				: "'".mysql_escape_string(json_encode($v))."'";
	}
	if(is_string($v)){
		if(preg_match("/bigint/i", $typeDef) && str_has($v, "-"))
			return strtotime($v);
		if(preg_match("/(int|byte)/i", $typeDef))
			return (int)$v;
		return "'".mysql_escape_string($v)."'";
	} 
	return $v;
}
