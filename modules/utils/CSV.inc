<?php
/**
 * CSV export
 */

class CSV{
	
	/**
	 * @param $file: file name without path , ex: report.csv
	 * @param $data: data list [{},{}...]
	 * @param $titles: {colName1:label1, colName2:label2...}
	 */
	public static function export($file,$data,$titles=null){
		header("Content-type: application/x-msexcel-csv");
		if(!str_has($file, ".csv"))
			$file .= ".csv";
		header('Content-Transfer-Encoding: binary');
        header( 'Content-Disposition: attachment;filename='.$file);

        $fp = fopen('php://output', 'w');
        if($titles == null){
        	$cols = array();
        	foreach ($data as $d){
        		$cols = array_unique(array_merge($cols, array_keys($d)));
        	}
        	$titles=array();
        	foreach ($cols as $col)
        		$titles[$col] = $col;
        }
        //print titles
        foreach ($titles as &$t) {
     	   $t = mb_convert_encoding($t, 'SJIS-win', 'UTF-8');
    	}
    	fputcsv($fp, array_values($titles));
		foreach ($data as &$d){
			$row = [];
			foreach ($titles as $col=>$label){
				$v = hash_get($d, $col);
				$row[] = mb_convert_encoding($v, 'SJIS', 'UTF-8');;
			}
			fputcsv($fp, $row);
		}
        fclose($fp);
	}

}