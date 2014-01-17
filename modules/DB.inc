<?php
/**
 *	@file: DB.inc	
 *	@author: Soyoes 2014/01/10
 *	@uses: db library
 *	@example: 
 *****************************************************************************/

/*TODO
 
 test db_find
 test db_find1st
 test db_save
 test db_delete
 test db_update
 
 test db_trans
 test db_import
 
 test model::factory
 test model::save
 
 * 
 * 
 * 
 */

if (!apc_load_constants("DB_FMTS")){
	apc_define_constants("DB_FMTS", [
	'FORMAT_CREATE_DB' 			 => "CREATE TABLE `%s`.`%s` ( %s PRIMARY KEY (`%s`)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_CREATE_DB_MULTI_KEY' => "CREATE TABLE `%s`.`%s` ( %s CONSTRAINT %s PRIMARY KEY (%s)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_INSERT_DB' 			 => "INSERT %s INTO `%s` (%s) VALUES(%s);",
	'FORMAT_UPDATE_DB'			 => "UPDATE `%s` SET %s WHERE `%s`=%s;"
	]);
}

$QUERY_REGEXP_OP = ["mysql"=>"REGEXP","postgres"=>"~"];

/*
 * Test cases
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
 * */
$QUERY_FILTERS = [
	"=" 	=> function($k,$v){return "`$k`=".db_v($v);},
	"!" 	=> function($k,$v){return "`$k`!=".db_v($v);},
	"<" 	=> function($k,$v){return "`$k`<".db_v($v);},
	">" 	=> function($k,$v){return "`$k`>".db_v($v);},
	"<=" 	=> function($k,$v){return "`$k`<=".db_v($v);},
	">=" 	=> function($k,$v){return "`$k`>=".db_v($v);},
	"[]" 	=> function($k,$v){if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` IN (".join(",",$vs).")";},
	"![]" 	=> function($k,$v){if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` NOT IN (".join(",",$vs).")";},
	"()" 	=> function($k,$v){if(count($v)!=2)return false; return "(`$k` BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";},
	"!()" 	=> function($k,$v){if(count($v)!=2)return false; return "(`$k` NOT BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";},
	"?"  	=> function($k,$v){if(!Strings::contains($v,"%"))$v="%$v%";return "`$k` LIKE '".$v."'";},
	"!?"  	=> function($k,$v){if(!Strings::contains($v,"%"))$v="%$v%";return "`$k` NOT LIKE '".$v."'";},
	"~" 	=> function($k,$v){global $QUERY_REGEXP_OP;$op = $QUERY_REGEXP_OP[strtolower(DB_ENGINE)];if(!isset($op))return false;return "`$k` $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";}, 
			//Regexp only mysql or regexp
			/*	@example db_find("users", ["title@~" => "^My"]) == select * from users where title regexp '^My'
			 	For MySQL : these are available
			 	^ , $ , . , [...] , [^...] , p1|p2|p3 , * , + , {n} , {m,n}
			 */
	"!~"	=> function($k,$v){global $QUERY_REGEXP_OP;$op = $QUERY_REGEXP_OP[strtolower(DB_ENGINE)];if(!isset($op))return false;return "`$k` NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
	"~~"	=> function($k,$v){global $QUERY_REGEXP_OP;$op = $QUERY_REGEXP_OP[strtolower(DB_ENGINE)];if(!isset($op))return false;return "LOWER(`$k`) $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
	"!~~"	=> function($k,$v){global $QUERY_REGEXP_OP;$op = $QUERY_REGEXP_OP[strtolower(DB_ENGINE)];if(!isset($op))return false;return "LOWER(`$k`) NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";},
			//Regexp with ignorecase option only mysql or regexp
];

/**
 * get connection handler of PDO
 * @param opts:use conf.ini as default
 * @return PDO object.
 * */
function db_conn($opts=null){
	global $db;
	if(!isset($db)){
		$host = (strcmp(DB_HOST, "localhost")==0) ? "127.0.0.1" : DB_HOST;
		$conn_str = strtolower(DB_ENGINE).":host=".$host.";port=".DB_PORT.";dbname=".DB_DB.";charset=utf8";
		$db = new PDO($conn_str,DB_USER,DB_PASS,
				[PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
	}
	return $db;
}

/**
 * exec query SQL like select ...
 * @param $sql: sql statement.
 * @param $useCache: whether save/load result from APC/Memcache
 * @param $PDOOpt: FETCH_COLUMN, FETCH_ASSOC ...
 * @return array or assoc array.
 */
function db_query($sql, $useCache = false, $PDOOpt = PDO::FETCH_ASSOC) {
	try {
		if ($useCache) {
			$value = Cache::get($sql);
			if (isset ( $value ) && $value != false)
				return $value;
		}
		$db = db_conn();
		$statement = $db->prepare($sql);
		//var_dump($statement);
		if ($statement->execute () == FALSE) {
			return null;
		}
		$res = $statement->fetchAll($PDOOpt);
		if ($useCache && $res != null) {
			Cache::set($key, $res);
		}
		return $res;
	} catch ( PDOException $e ) {
		error_log ("DB ERR :". $e->getMessage() );
		error_log ("DB ERR SQL:". $sql );
		return null;
	}
}

/**
* exec $sql like update, insert ...
* return true->success/ false->failed
*/
function db_exec($sql){
	try {
		$db = db_conn();
		return $db->exec($sql);
	} catch (PDOException $e) {
		error_log("DB_ERR Exec :",$sql);
		return false;
	}
}

/**
 * return number result of "SELECT COUNT"
 */
function db_count($sql=null, $useCache=false){
	try {
		if($useCache){
			$value = Cache::get($sql);
			if(isset($value) && $value!=false)
				return $value;
		}
		
		$db = db_conn();
		$sth = $db->prepare($sql);
		$sth->execute();
		$res = $sth->fetchColumn();
		if($useCache && $res){
			Cache::set($sql, $res);
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
	list($colStr, $optStr) = db_make_query($table, $opts);
	$sql = "SELECT ".$colStr." FROM ".$table.$optStr;
	error_log("SEARCH: ".$sql);
	$res = db_query($sql, $opts["useCache"]);
	if($withCount){
		$sql = "SELECT count(*) FROM ".$table.preg_replace("/ORDER BY\s*(.*)\s*([LIMIT .*]*)/", '${3}',$optStr);
		$cnt = db_count($sql, $opts["useCache"]);
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
	
	$regName = defined(DB_REGAT_NAME)?DB_REGAT_NAME:"regAt";
	$updName = defined(DB_UPDAT_NAME)?DB_UPDAT_NAME:"updAt";
	
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
		db_exec($sql);
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
 * @param $omit : array, fields in opts to omit.
 * @return array or false
 */
function db_make_query($table, $opts=[], $omit=[]){
	if(!isset($table))return false;
	$colStr =  is_array($opts['fields'])? (count($opts['fields'])==0? "*" :join(",", $opts['fields']))
		 : (isset($opts['fields'])? $opts['fields']:'*');
	$optStr = "";
	$schema = db_schema($table)["schema"];
	$eg = strtolower(DB_ENGINE); 
	global $QUERY_FILTERS;
	if(Arrays::isHash($opts)){
		foreach ($opts as $k => $v){
			if(in_array($k,$omit))continue;
			if(is_callable($v)){ /*$k = key=value, $v = function($k,$v){return "$k > '$v'";} */
				list($key,$value)=explode("=", $k);
				$val = $v($key, $value);
				$optStr.= !empty($optStr) ? " AND $val ": " WHERE $val ";
			}else{
				list($k,$cmd) = explode("@",$k); 
				if(array_key_exists($k, $schema)){
					$cmd = !isset($cmd)||$cmd=="" ? "=":$cmd;
					$vStr =  $QUERY_FILTERS[$cmd]($k, $v);
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
	return [$colStr,$optStr];
}

function db_exists($table, $id){
	if(!isset($table) || !isset($id))
		return false;
	$pk = db_schema($table)["general"]["pk"];
	$entity = db_count("select count(*) from $table where `$pk`=".db_v($id));
	return $entity>0;
}

/**
 * @param $table:tablename
 * @param $opts:query options
 *
 * @example : db_update("tasks",[groupId=>3]); //delete from ... where groupId=3
 * */
function db_delete($table, $opts){
	if(!isset($table) || Arrays::isEmpty($opts))
		return false;
	$sql = "DELETE FROM $table ".db_make_query($table, $opts,["order","limit","fields"])[1];
	return db_exec($sql);
}

/**
 * @param $table:tablename
 * @param $data:hash, new value to update 
 * @param $opts:query options
 * 
 * @example : db_update("tasks",[groupId=>3],["id@>"=>38]); //set groupId=3 where id>38
 * */
function db_update($table, $data, $opts=[]){
	if(!isset($table) || Arrays::isEmpty($data) || !Arrays::isHash($data))
		return false;
	$vStrs = [];
	$schema = db_schema($table)["schema"];
	foreach($data as $k=>$v){
		$vStrs[]="`$k`=".db_v($v, $schema[$k]);
	} 
	$vStrs = join(",",$vStrs);
	$sql = "UPDATE $table SET $vStrs ".db_make_query($table, $opts, ["order","limit","fields"])[1];
	return db_exec($sql);
}

function db_migrate($table){
	$sql = sprintf("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '%s' AND table_name = '%s';", DB_DB, $table);
	$res = db_count($sql, false);
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
		if (Strings::contains($pk, "|")){
			$parts = explode("|", $pk);
			$pkName = $parts[0];
			$keys = $parts[1];
			$sql = sprintf(FORMAT_CREATE_DB_MULTI_KEY, DB_DB, $table, $colStmt, $pkName, $keys, $incStmt);
		}else{
			$sql = sprintf(FORMAT_CREATE_DB, DB_DB, $table, $colStmt, $pk, $incStmt);
		}
		$res = db_exec($sql);
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
	if(!isset($table) || !Arrays::isHash($data) || Arrays::isEmpty($data))return false;
	$regName = defined(DB_REGAT_NAME)?DB_REGAT_NAME:"regAt";
	$updName = defined(DB_UPDAT_NAME)?DB_UPDAT_NAME:"updAt";
	$schema_def = db_schema($table);
	$schema = $schema_def["schema"];
	$pk = $schema_def["general"]["pk"];
	$id = isset($data[$pk]) ? $data[$pk] : null;
	if(array_key_exists($updName,$schema))
		$data[$updName] = Dates::format();
	$sql = "";
	$isUpdate = isset($id) && db_exists($table, $id);
	if ($isUpdate){	//update
		Cache::del($table."_".$id);
		foreach ($data as $col => $val){
			if($col==$pk || !isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			$colStmt .= "`$col`=".db_v($val, $schema[$col], $bson);
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
			$valStmt .= db_v($val, $schema[$col], $bson);
		}
		//$ignore = $ignoreErrors ? "IGNORE" : "";
		$sql = sprintf(FORMAT_INSERT_DB, $ignore, $table, $colStmt, $valStmt);
	}
	try {
		if($returnId==true && !$isUpdate) {
			$res = db_trans([$sql, "SELECT LAST_INSERT_ID() as 'last_id'"]);
			$data["id"] = $res[0]["last_id"];
		}else{
			db_exec($sql);
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
	$schemaDef = Cache::get("SCHEMA_$schemaName", function($key){
		$schemaname = str_replace("SCHEMA_","",$key);
		$schemaDef = @parse_ini_file(SCHEMA_DIR.$schemaname.".ini", true);
		if(!$schemaDef){
			//error_log("schema=".LIBER_DIR."common/schemas/".$schemaname.".ini");
			$schemaDef = @parse_ini_file(LIBER_DIR."common/schemas/".str_replace("liber_","",$schemaname).".ini", true);
		}
		return $schemaDef;
	});
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
function db_trans($querys){
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
			foreach($querys as $q){
				if($q==="@rollback"){
					$db->rollBack();$cnt--;
				}else{
					if(Strings::startsWith(strtolower($q), "select")){
						$statement = $db->prepare($q);
						if ($statement->execute() == FALSE) {
							continue;
						}
						$res = $statement->fetchAll(PDO::FETCH_ASSOC);//FIXME PDO option
					}else {
						$r = $db->exec($q);
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
	return Strings::str2hex($str);
}

function bson_dec($bson){
	if(isset($bson)){
		$json = Strings::hex2str($bson);
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
		if(preg_match("/bigint/i", $typeDef) && Strings::contains($v, "-"))
			return strtotime($v);
		if(preg_match("/(int|byte)/i", $typeDef))
			return (int)$v;
		return "'".mysql_escape_string($v)."'";
	} 
	return $v;
}
