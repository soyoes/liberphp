<?php
/**
 *	@file: Model.inc
 *	@author: Soyoes 2014/01/10
 *	@uses: model class
 *	@example:
 *****************************************************************************/

class Model extends Observable implements Observer{
	
	private $db;
	private $pk;		//primary key
	private $table;		//table name
	public 	$schema;	//array
	private $restricts;	//array migration only
	private $indexes;	//array migration only
	
	private function __construct(){}
	
	/*
	 * 	get a new model instance 
	 * 	@function $schemaName : null -> binds nothing, tablename -> binds a certain table name
	 *  @return Model
	 */
	public static function factory($schemaName){
		try{
			if(!isset($schemaName))
				throw Exception("Model must have a schema name");
			$model = new Model();
	        $db = null;
	        if(Strings::contains($schemaName, '.')){
	        	list($db, $schemaName) = explode('.', $schemaName);
	        	$model->db = $db;
	        } 
	        $model->addObserver($model);
			if(isset($schemaName) && $db==null)
				$model->loadSchema($schemaName);
			return $model;
		}catch(Exception $e){
			throw new Exception($schemaName.'Model not found');
		}
	}
	
	/**
	 * @param $schemaname	schema def file name(without .ini) under conf/schemas
	 */
	public function loadSchema($schemaname){
		try{
			$schemaDef = db_schema($schemaname);
			$general = $schemaDef["general"];
			error_log(json_encode($general));
			if(isset($general["db"])) $this->db = $general["db"];
			$this->pk	 	= $general["pk"];
			$this->table	= $general["name"];
			if(isset($general["index"])) $this->indexes = $general["index"];
			$this->schema 	= $schemaDef["schema"];
			$this->isTemp = isset($schemaDef["temp"]);
		}catch(Exception $e){
			throw new Exception($schemaName.' schema file not found');
		}
	}
	
	/**
	 * @param hash $data		: {tag:'mytag',regAt=1023434555}
	 * @param string $condition : tag='mytag'
	 */
	public function addIfNotExist($data,$condition){
		$cols = "`".join("`,`",array_keys($data))."`";
		$values = "'".(join("','",array_values($data)))."'";
		$table = $this->table;
		$sql = "insert into $table ($cols) SELECT $values from $table where not exists (select id from $table where $condition) limit 1;";
		db_exec($sql);
	}
	
	/**
	 * insert many records to DB
	 * @param $datas : hash list
	 * @param table:tablename
	 * */
	public function import($datas, $sql_dump_path=null){
		return db_import($this->table, $datas, $sql_dump_path);
	}

	/**
	 * search by conditions
	 * @param array or string $opts
	 * 		if opts is Array : reserved keywords in opts
	 * 		* fields = default = * ||  id,pass,... || [id, pass, name ...]
	 * 		* limit : 0/default=all, 0,20=select 0~20, 21~80= select from 21 get 60 records 
	 * 		* order : default = "" || id desc || updAt desc
	 * 		* useCache : false | true
	 * @return array or false
	 */
	public function find($opts=array(),$withCount=false){
		return db_find($this->table,$opts,$withCount);
	}
	
	
	/**
	 * get a single record from db by pk
	 * @param string $id		:pk value
	 * @param boolean $useCache	:true = use memcache
	 * @param string $cols :	cols to fetch
	 * @return Entity 
	 */
	public function fetch($id, $useCache=false, $cols="*",$pkName=null){
		try {
			if($pkName==null)	
				$pkName = $this->pk;
			if(!isset($this->table))
				return null;
			$pk = is_string($id) ? "'". mysql_escape_string($id). "'":$id;
			if($cols==null) $cols ="*";
			$sql = "SELECT ".$cols." FROM ".$this->table." WHERE `".$pkName."`=".$pk." LIMIT 1";
			$res = db_query($sql,$useCache);
		    $data = $res[0];
		    return $data;
		} catch (PDOException $e) {
			error_log($sql);
		    return null;
		}		
	}
	public function find1st($opts=array()){
		return db_find1st($this->table,$opts);
	}
	
	public function exists($pk){
		return db_exists($this-table,$pk);
	}
	
	public function del($optsOrPk){
		if(!isset($this->table) || !isset($this->pk) || !isset($optsOrPk))
			return false;
		if(is_array($optsOrPk)){
			$ops = db_make_query($this->table, $optsOrPk, ["order","fields"]);
			$sql = "DELETE FROM ".$this->table." ".$ops[1];
		}else{
			$pk = is_string($optsOrPk) ? '"'. mysql_escape_string($optsOrPk). '"' : $optsOrPk;
			$sql = "DELETE FROM ".$this->table." WHERE `".$this->pk."`=".$pk;
		}
		error_log($sql);
		return db_exec($sql);		
	}
	
	/**
	 * insert or update by checking pk
	 * @param boolean $useCache
	 */
	public function save($data=null, $returnId=false){
		$o = $this;
		if (isset($data)){
			foreach ($data as $k => $v){
				$this->set($k, $v);
			}
		}
		$id = isset($this->data[$this->pk]) ? $this->data[$this->pk] : null;
		$isUpdate = isset($id);
		$d = $isUpdate?$this->changes:$this->data;
		if($isUpdate)$d[$this->pk] = $id;
		return db_save($this->table, $d, $returnId);
	}
	
	public function __set($key, $value){
		$this->$key = $value;
	}
	
	public function __get($key){
        return $this->$key;
	}

	/*
	 * FIXME
	 * $m->upd
	 * */
	public function upd($conditions, $changes){
		$constr = array();
		foreach ($conditions as $k=>$v){
			$v = (is_string($v)) ? "'$v'":$v; 
			$constr[]= "`$k`=$v";
		}
		$constr = join(" AND ", $constr);

		$chstr = array();
		foreach ($changes as $k=>$v){
			$v = (is_string($v)) ? "'$v'":$v; 
			$chstr[]= "$k=$v";
		}
		$chstr = join(", ", $chstr);
		
		$sql ="update ".$this->table." set $chstr where $constr";
		db_exec($sql);
	}
	
	/* (non-PHPdoc)
	 * Delegate method of Observer !!
	 * @see Observer::update()
	 */
	public function update(Observable $obj, $args){
		if(isset($this->schema[$args["key"]])){
			$v = $args["val"];
			if(is_string($v) && Strings::contains($this->schema[$args['key']], "int"))
				$v = (int) $v;
			$this->changes[$args['key']] = $v;
		}
	}
	
	public function get($keyPath){
		return Arrays::get($this->data, $keyPath);
	}
	
	public function setDB($db){
		$this->db= $db;
	}
	
	
	/**
	 * increase counter
	 * @param str $condition : tag="mytag"
	 * @param str $col :	tag
	 * @param mixed $value
	 */
	public function inc($condition,$col,$value=1){
		$sql = "update ".$this->table." set `$col`=`$col`+1 where $condition";
		db_exec($sql);
	} 
	
	public function replace($col, $from, $to){
		$table = $this->table;
		$sql = "UPDATE `$table` SET $col=REPLACE ($col,'$from','$to');";
		db_exec($sql);
	}
	
}
